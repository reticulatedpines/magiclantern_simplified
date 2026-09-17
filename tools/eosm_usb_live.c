// SPDX-License-Identifier: GPL-2.0-only
// Live EOS M USB viewer. Close the window or press Esc to end the PTP session.
// Build: clang eosm_usb_live.c -I/opt/homebrew/include/libusb-1.0 -I/usr/local/include/SDL2 -L/opt/homebrew/lib -L/usr/local/lib -lusb-1.0 -lSDL2 -o eosm_usb_live
#define EOSM_PREVIEW_TEST
#include "eosm_usb_preview.c"
#include <SDL.h>
#include <signal.h>

static volatile sig_atomic_t stop_requested = 0;
static void stop_on_signal(int unused) { (void)unused; stop_requested = 1; }

int main(void) {
    signal(SIGINT, stop_on_signal);
    signal(SIGTERM, stop_on_signal);
    libusb_context *ctx = NULL;
    libusb_device_handle *camera = NULL;
    struct libusb_config_descriptor *config = NULL;
    int iface = -1, claimed = 0, session_open = 0, status = 1;
    uint8_t in = 0, out = 0;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    uint8_t *data = NULL;
    uint32_t length = 0, txid = 1;

    if (libusb_init(&ctx)) return 1;
    camera = libusb_open_device_with_vid_pid(ctx, CAMERA_VENDOR, CAMERA_PRODUCT);
    if (!camera) { fprintf(stderr, "EOS M not accessible over USB\n"); goto cleanup; }
    if (libusb_get_active_config_descriptor(libusb_get_device(camera), &config)) goto cleanup;
    for (int i = 0; i < config->bNumInterfaces && iface < 0; i++) {
        const struct libusb_interface *it = &config->interface[i];
        for (int a = 0; a < it->num_altsetting; a++) {
            const struct libusb_interface_descriptor *alt = &it->altsetting[a];
            uint8_t ep_in = 0, ep_out = 0;
            for (int e = 0; e < alt->bNumEndpoints; e++) {
                const struct libusb_endpoint_descriptor *ep = &alt->endpoint[e];
                if ((ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_BULK) continue;
                if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN) ep_in = ep->bEndpointAddress;
                else ep_out = ep->bEndpointAddress;
            }
            if (alt->bInterfaceClass == 6 && ep_in && ep_out) {
                iface = alt->bInterfaceNumber; in = ep_in; out = ep_out; break;
            }
        }
    }
    if (iface < 0 || libusb_claim_interface(camera, iface)) {
        fprintf(stderr, "Could not claim PTP interface; close Photos/Image Capture.\n");
        goto cleanup;
    }
    claimed = 1;
    if (ptp(camera, out, in, 0x1002, txid++, 1, 1, &data, &length)) goto cleanup;
    session_open = 1;
    uint32_t protocol[5] = {0};
    int protocol_params = 0;
    uint32_t version_param = 0;
    if (ptp_command(camera, out, in, PREVIEW_OPCODE, txid++, 1,
                    &version_param, &data, &length, protocol, &protocol_params)) goto cleanup;
    if (protocol_params < 2 || protocol[0] != 1 || protocol[1] != 1) {
        fprintf(stderr, "Unsupported preview protocol.\n");
        goto cleanup;
    }
    free(data); data = NULL;
    if (SDL_Init(SDL_INIT_VIDEO)) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); goto cleanup; }
    window = SDL_CreateWindow("EOS M USB LiveView", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              720, 480, SDL_WINDOW_RESIZABLE);
    if (!window) { fprintf(stderr, "SDL window: %s\n", SDL_GetError()); goto cleanup; }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) { fprintf(stderr, "SDL renderer: %s\n", SDL_GetError()); goto cleanup; }
    unsigned texture_width = 0, texture_height = 0, frames = 0;
    uint32_t fps_start = SDL_GetTicks();
    while (!stop_requested) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                stop_requested = 1;
        }
        if (stop_requested) break;
        if (ptp(camera, out, in, PREVIEW_OPCODE, txid++, 1, 1, &data, &length)) {
            fprintf(stderr, "Frame request failed; is the camera still in LiveView?\n");
            break;
        }
        unsigned width = 0, height = 0;
        uint8_t *rgb = decode_frame_rgb(data, length, &width, &height);
        free(data); data = NULL;
        if (!rgb) { fprintf(stderr, "Invalid frame data\n"); break; }
        if (!texture || width != texture_width || height != texture_height) {
            if (texture) SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                        SDL_TEXTUREACCESS_STREAMING, width, height);
            texture_width = width; texture_height = height;
        }
        if (!texture || SDL_UpdateTexture(texture, NULL, rgb, width * 3)) {
            fprintf(stderr, "SDL texture: %s\n", SDL_GetError());
            free(rgb); break;
        }
        free(rgb);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        frames++;
        uint32_t now = SDL_GetTicks();
        if (now - fps_start >= 1000) {
            char title[100];
            snprintf(title, sizeof(title), "EOS M USB LiveView — %u×%u — %.1f fps",
                     width, height, 1000.0 * frames / (now - fps_start));
            SDL_SetWindowTitle(window, title);
            fps_start = now; frames = 0;
        }
    }
    status = 0;
cleanup:
    if (session_open) {
        free(data); data = NULL;
        ptp(camera, out, in, 0x1003, txid++, 0, 0, &data, &length);
    }
    free(data);
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    if (claimed) libusb_release_interface(camera, iface);
    if (config) libusb_free_config_descriptor(config);
    if (camera) libusb_close(camera);
    libusb_exit(ctx);
    return status;
}
