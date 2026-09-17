# Experimental EOS M USB LiveView (read-only)

This is an opt-in PTP preview for the original EOS M, tested on Canon firmware 2.0.2 with Magic Lantern running from the SD card. It returns a single copy of the current LiveView YUV buffer per request. It does not start LiveView, alter exposure, release the shutter, or expose arbitrary memory operations.

## Build the camera side

Build from `platform/EOSM.202` with `CONFIG_PTP=y CONFIG_PTP_PREVIEW=y`. Both flags are off by default. Install the resulting `autoexec.bin` and matching `ML` folder on a camera card that is already bootable with Magic Lantern; do not install `ML-SETUP.FIR` merely to try this experiment. Use a backup and a disposable card. The handler currently rejects non-EOS-M builds at compile time.

## Build the host side

The host examples use libusb 1.0. The continuous viewer also uses SDL2. For example, on a Mac with Homebrew libraries (adjust include/library paths as needed):

```sh
clang tools/eosm_usb_preview.c -I/opt/homebrew/include/libusb-1.0 -L/opt/homebrew/lib -lusb-1.0 -o eosm_usb_preview
clang tools/eosm_usb_live.c -I/opt/homebrew/include/libusb-1.0 -I/usr/local/include/SDL2 -L/opt/homebrew/lib -L/usr/local/lib -lusb-1.0 -lSDL2 -o eosm_usb_live
```

Switch the camera to LiveView, connect its USB cable, and close any other PTP client (such as Photos or Image Capture). Run `./eosm_usb_live` for a window that updates until Esc or close; or `./eosm_usb_preview frame.ppm` for one frame. Only one program can claim the camera's PTP interface at a time.

## Protocol

Vendor operation `0x9997`, one input parameter:

- `0`: response parameters `1` (protocol version) and `1` (signed-chroma UYVY422 format).
- `1`: a data phase beginning with `MLVF`, followed by little-endian 16-bit width/height, little-endian 32-bit pitch/payload size, then the copied UYVY422 buffer; response `0x2001` on success. LiveView must already be active.

The current bounds are width ≤ 1024, height ≤ 600, even width, pitch between `width * 2` and 2048, and payload ≤ 1,228,800 bytes. The DIGIC 5 chroma bytes are signed. The host examples convert to RGB using Magic Lantern's YUV coefficients. Frames may tear if Canon swaps the source buffer while it is copied. Neither latency nor throughput is guaranteed.

## Verification and limits

Verified on one physical EOS M / firmware 2.0.2: version response, 720×480 still frame, and an updating live window at about 13 fps. The host RGB decoder also passed a synthetic colour test. Other camera models and firmware versions were not tested. This is an experimental feature, disabled by default; a custom ML build can still hang the camera. Test with a nonessential card and be ready to power off and restore a known-good build.
