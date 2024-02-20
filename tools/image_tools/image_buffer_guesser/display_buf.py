#!/usr/bin/env python3

import os
import argparse

import cv2
import numpy as np

import width_guesser
import convert_14_to_16

MAX_ASPECT = 30
MAX_DISPLAY_WIDTH = 1920


def main():
    args = parse_args()

    # RGB and YUV display attempts use 8 bit, with 3 or 2 planes,
    # 14 bit Bayer we convert from 8 (containing packed 14 bit),
    # to 16 bit.
    buf_8 = np.fromfile(args.filename, dtype=np.uint8)
    buf_16 = convert_14_to_16.convert_14_to_16(buf_8)

    # width guess each buffer
    top_20_rgb = width_guesser.get_size_guesses(buf_8, planes=3)
    top_20_yuv = width_guesser.get_size_guesses(buf_8, planes=2)
    top_20_bayer = width_guesser.get_size_guesses(buf_16, planes=1)

    # filter out outlandish aspect ratios
    best_rgb = [x for x in top_20_rgb if ((max(x[0], x[1]) // min(x[0], x[1]) < MAX_ASPECT))]
    best_yuv = [x for x in top_20_yuv if ((max(x[0], x[1]) // min(x[0], x[1]) < MAX_ASPECT))]
    best_bayer = [x for x in top_20_bayer if ((max(x[0], x[1]) // min(x[0], x[1]) < MAX_ASPECT))]
    
    print("Top 4 guesses for RGB w*h: %s" % best_rgb[:4])
    buf_rgb_w_h = None
    if (best_rgb):
        buf_rgb_w_h = best_rgb[0] # (w, h) tuple

    print("Top 4 guesses for YUV w*h: %s" % best_yuv[:4])
    buf_yuv_w_h = None
    if (best_yuv):
        buf_yuv_w_h = best_yuv[0]

    print("Top 4 guesses for Bayer w*h: %s" % best_bayer[:4])
    buf_bayer_w_h = None
    if (best_bayer):
        buf_bayer_w_h = best_bayer[0]

    # convert each to BGR
    rgb = None
    if (buf_rgb_w_h):
        rgb = bgr_from_rgb(buf_8, buf_rgb_w_h[0])
        if buf_rgb_w_h[0] > MAX_DISPLAY_WIDTH:
            ratio = buf_rgb_w_h[1] / buf_rgb_w_h[0]
            rgb = cv2.resize(rgb, (MAX_DISPLAY_WIDTH, int(MAX_DISPLAY_WIDTH * ratio)))

    yuv = None
    if (buf_yuv_w_h):
        yuv = bgr_from_yuv(buf_8, buf_yuv_w_h[0])
        if buf_yuv_w_h[0] > MAX_DISPLAY_WIDTH:
            ratio = buf_yuv_w_h[1] / buf_yuv_w_h[0]
            yuv = cv2.resize(yuv, (MAX_DISPLAY_WIDTH, int(MAX_DISPLAY_WIDTH * ratio)))

    bayer = None
    if (buf_bayer_w_h):
        bayer = bgr_from_bayer(buf_16, buf_bayer_w_h[0])
        if buf_bayer_w_h[0] > MAX_DISPLAY_WIDTH:
            ratio = buf_bayer_w_h[1] / buf_bayer_w_h[0]
            bayer = cv2.resize(bayer, (MAX_DISPLAY_WIDTH, int(MAX_DISPLAY_WIDTH * ratio)))

    # if multiple plausible width guesses,
    # cycle on key press?

    if (rgb is not None):
        print("Showing 24bpp RGB")
        cv2.imshow("frame", rgb)
        cv2.waitKey()

    if (yuv is not None):
        print("Showing 16bpp YUV")
        cv2.imshow("frame", yuv)
        cv2.waitKey()

    if (bayer is not None):
        print("Showing 42bpp Bayer")
        cv2.imshow("frame", bayer)
        cv2.waitKey()


def bgr_from_bayer(buf, width):
    # expects a buf composed of uint16s
    buf2 = buf.reshape(-1, width, 1)
    buf2 = buf2 * 4 # raws often appear dark, and we have two spare bits of 0s to shift into

    return cv2.cvtColor(buf2, cv2.COLOR_BayerRGGB2BGR)


def bgr_from_yuv(buf, width):
    # expects a buf composed of bytes

    # we reshape into 2 planes later, ensure buf is compatible length
    buf2 = extend_np_array(buf, 2)

    # Canon YUV buffers (maybe only on modern cams?)
    # have an offset applied to the UV channels,
    # we must fix this up since OpenCV doesn't have an option.

    # Reshape to allow easy swapping / manipulation of each byte of each u32
    buf2 = buf2.reshape(-1, 4) # 4 cols
    buf2 = buf2.swapaxes(0, 1) # 4 rows, each all one byte from the u32s

    # conditionally modify the UV rows only
    uv_selector = np.array([True, False, True, False])
    uv_only = buf2[uv_selector]
    uv_only = (uv_only - 128)

    # replace original UV rows with modified
    buf2[[0, 2]] = uv_only[[0, 1]]

    # Reshape for display
    buf2 = buf2.swapaxes(0, 1)
    buf2 = buf2.reshape(-1, width, 2)

    return cv2.cvtColor(buf2, cv2.COLOR_YUV2BGR_UYVY)


def bgr_from_rgb(buf, width):
    # we reshape into 3 planes later, ensure buf is compatible length
    buf2 = extend_np_array(buf, 3)

    buf2 = buf2.reshape(-1, width, 3)
    return cv2.cvtColor(buf2, cv2.COLOR_RGB2BGR)


def extend_np_array(buf, factor):
    """
    Return a copy of buf, extended using the last element,
    so that factor is a factor of the length of the copy.

    That is, make the length of buf divisible by factor.
    """
    r = len(buf) % factor
    if r:
        buf2 = buf
        return np.append(buf2, [buf2[-1]] * (factor - r))
    else:
        return buf


def parse_args():
    description = """
    Attempts to guess dimensions of dumped data,
    displaying as RGB, YUV and Bayer.

    Useful for quickly checking unknown data
    that you hope is an image.
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("filename",
                        help="image buffer dump")

    args = parser.parse_args()
    if not os.path.isfile(args.filename):
        print("file didn't exist: '%s'" % args.filename)
        exit()

    return args


if __name__ == "__main__":
    main()
