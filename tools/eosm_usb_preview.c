// SPDX-License-Identifier: GPL-2.0-only
// Host receiver for the experimental EOS M Magic Lantern PTP preview handler.
// Build: clang eosm_usb_preview.c -I/opt/homebrew/include/libusb-1.0 -L/opt/homebrew/lib -lusb-1.0 -o eosm_usb_preview
// Run after installing the matching experimental autoexec.bin: ./eosm_usb_preview frame.ppm
#include <libusb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAMERA_VENDOR 0x04a9
#define CAMERA_PRODUCT 0x323d
#define PREVIEW_OPCODE 0x9997
#define MAX_PTP_DATA (2 * 1024 * 1024)

static void put16(uint8_t *p, uint16_t v) { p[0] = v; p[1] = v >> 8; }
static void put32(uint8_t *p, uint32_t v) {
    p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24;
}
static uint16_t get16(const uint8_t *p) { return (uint16_t)p[0] | (uint16_t)p[1] << 8; }
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int ptp_command(libusb_device_handle *camera, uint8_t out, uint8_t in,
                       uint16_t opcode, uint32_t txid, int nparams,
                       const uint32_t *params, uint8_t **data,
                       uint32_t *data_size, uint32_t *response,
                       int *response_count) {
    uint8_t command[32] = {0}, packet[16384];
    int transferred = 0, err;
    if (nparams < 0 || nparams > 5) return -1;
    *data = NULL;
    *data_size = 0;
    if (response_count) *response_count = 0;
    put32(command, 12 + nparams * 4);
    put16(command + 4, 1);
    put16(command + 6, opcode);
    put32(command + 8, txid);
    for (int i = 0; i < nparams; i++) put32(command + 12 + i * 4, params[i]);
    err = libusb_bulk_transfer(camera, out, command, 12 + nparams * 4,
                               &transferred, 5000);
    if (err || transferred != 12 + nparams * 4) {
        fprintf(stderr, "PTP send: %s\n", libusb_error_name(err));
        return -1;
    }
    for (int phase = 0; phase < 2; phase++) {
        err = libusb_bulk_transfer(camera, in, packet, sizeof(packet), &transferred, 30000);
        if (err || transferred < 12) {
            fprintf(stderr, "PTP receive: %s (%d bytes)\n", libusb_error_name(err), transferred);
            return -1;
        }
        uint32_t total = get32(packet), reply_txid = get32(packet + 8);
        uint16_t type = get16(packet + 4), code = get16(packet + 6);
        if (total < 12 || reply_txid != txid) return -1;
        if (type == 3) {
            if (code != 0x2001) {
                fprintf(stderr, "PTP opcode 0x%04x returned 0x%04x\n", opcode, code);
                return -1;
            }
            if (response && response_count) {
                if (total > 32 || transferred < (int)total || ((total - 12) & 3)) return -1;
                *response_count = (int)(total - 12) / 4;
                for (int i = 0; i < *response_count; i++)
                    response[i] = get32(packet + 12 + i * 4);
            }
            return 0;
        }
        if (type != 2 || code != opcode || phase != 0 || total > MAX_PTP_DATA + 12)
            return -1;
        *data_size = total - 12;
        *data = malloc(*data_size);
        if (!*data) return -1;
        uint32_t received = transferred - 12;
        if (received > *data_size) return -1;
        memcpy(*data, packet + 12, received);
        while (received < *data_size) {
            int want = *data_size - received < sizeof(packet)
                       ? (int)(*data_size - received) : (int)sizeof(packet);
            err = libusb_bulk_transfer(camera, in, *data + received, want, &transferred, 10000);
            if (err || transferred <= 0) {
                fprintf(stderr, "PTP frame transfer: %s\n", libusb_error_name(err));
                return -1;
            }
            received += transferred;
        }
    }
    return -1;
}

static int ptp(libusb_device_handle *camera, uint8_t out, uint8_t in,
               uint16_t opcode, uint32_t txid, int has_param, uint32_t param,
               uint8_t **data, uint32_t *data_size) {
    return ptp_command(camera, out, in, opcode, txid, has_param, &param,
                       data, data_size, NULL, NULL);
}

static uint8_t clip(int v) { return v < 0 ? 0 : v > 255 ? 255 : (uint8_t)v; }

static uint8_t *decode_frame_rgb(const uint8_t *data, uint32_t length,
                                 unsigned *width_out, unsigned *height_out) {
    if (length < 16 || memcmp(data, "MLVF", 4)) return NULL;
    unsigned width = get16(data + 4), height = get16(data + 6);
    uint32_t pitch = get32(data + 8), bytes = get32(data + 12);
    if (!width || width > 1024 || (width & 1) || !height || height > 600 ||
        pitch < width * 2 || pitch > 2048 || bytes != pitch * height ||
        length != bytes + 16) return NULL;
    uint8_t *rgb = malloc(width * height * 3);
    if (!rgb) return NULL;
    // Magic Lantern's DIGIC 5 YUV422 uses signed U and V (not +128 offset).
    // These coefficients match its Rec.601 conversion in src/imgconv.c.
    for (unsigned y = 0; y < height; y++) {
        const uint8_t *row = data + 16 + y * pitch;
        for (unsigned x = 0; x < width; x += 2) {
            int u = (int8_t)row[x * 2], v = (int8_t)row[x * 2 + 2];
            for (int i = 0; i < 2; i++) {
                int luma = row[x * 2 + 1 + 2 * i];
                size_t offset = ((size_t)y * width + x + i) * 3;
                rgb[offset] = clip(luma + (1437 * v >> 10));
                rgb[offset + 1] = clip(luma + ((-352 * u - 731 * v) >> 10));
                rgb[offset + 2] = clip(luma + (1812 * u >> 10));
            }
        }
    }
    *width_out = width;
    *height_out = height;
    return rgb;
}

static int save_ppm(const char *path, const uint8_t *data, uint32_t length) {
    unsigned width, height;
    uint8_t *rgb = decode_frame_rgb(data, length, &width, &height);
    if (!rgb) return -1;
    FILE *file = fopen(path, "wb");
    if (!file) { perror(path); free(rgb); return -1; }
    fprintf(file, "P6\n%u %u\n255\n", width, height);
    int okay = fwrite(rgb, 1, width * height * 3, file) == width * height * 3;
    if (fclose(file)) okay = 0;
    free(rgb);
    if (!okay) return -1;
    printf("Saved %ux%u preview to %s\n", width, height, path);
    return 0;
}

#ifndef EOSM_PREVIEW_TEST
int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "Usage: %s output.ppm\n", argv[0]); return 2; }
    libusb_context *ctx = NULL;
    if (libusb_init(&ctx)) return 1;
    libusb_device_handle *camera = libusb_open_device_with_vid_pid(ctx, CAMERA_VENDOR, CAMERA_PRODUCT);
    if (!camera) { fprintf(stderr, "EOS M not accessible\n"); libusb_exit(ctx); return 1; }
    struct libusb_config_descriptor *config = NULL;
    if (libusb_get_active_config_descriptor(libusb_get_device(camera), &config)) goto fail;
    int iface = -1;
    uint8_t in = 0, out = 0;
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
    if (iface < 0 || libusb_claim_interface(camera, iface)) goto fail;
    uint8_t *data = NULL;
    uint32_t length = 0;
    int success = ptp(camera, out, in, 0x1002, 1, 1, 1, &data, &length) == 0;
    if (success) {
        success = ptp(camera, out, in, PREVIEW_OPCODE, 2, 1, 0, &data, &length) == 0;
        free(data); data = NULL;
        if (success) {
            success = ptp(camera, out, in, PREVIEW_OPCODE, 3, 1, 1, &data, &length) == 0;
            if (success) success = save_ppm(argv[1], data, length) == 0;
            free(data); data = NULL;
        }
        ptp(camera, out, in, 0x1003, 4, 0, 0, &data, &length);
    }
    libusb_release_interface(camera, iface);
    libusb_free_config_descriptor(config);
    libusb_close(camera);
    libusb_exit(ctx);
    return success ? 0 : 1;
fail:
    fprintf(stderr, "Could not claim the camera PTP interface (close Photos/Image Capture).\n");
    if (config) libusb_free_config_descriptor(config);
    libusb_close(camera);
    libusb_exit(ctx);
    return 1;
}
#endif
