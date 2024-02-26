#!/usr/bin/env python3

import os
import argparse
import functools

import cv2
import numpy as np

import width_guesser
import convert_14_to_16

MAX_ASPECT = 60
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
    
    bufs = [{"top_4": best_rgb[:4],
             "desc": "24bpp RGB",
             "bgr_func": functools.partial(bgr_from_rgb, buf_8)},
            {"top_4": best_yuv[:4],
             "desc": "16bpp YUV",
             "bgr_func": functools.partial(bgr_from_yuv, buf_8)},
            {"top_4": best_bayer[:4],
             "desc": "42bpp Bayer",
             "bgr_func": functools.partial(bgr_from_bayer, buf_16)},
            ]

    for b in bufs:
        print("Top 4 guesses for %s, w*h: %s" % (b["desc"], b["top_4"]))

    bufs = [b for b in bufs if b["top_4"]]

    # Allow user to select between decodings and w*h guesses
    # with wasd
    buf_i = 0
    dims_i = 0
    while True:
        b = bufs[buf_i]
        w_h = bufs[buf_i]["top_4"][dims_i]

        bgr = b["bgr_func"](w_h[0])

        if w_h[0] > MAX_DISPLAY_WIDTH:
            ratio = w_h[1] / w_h[0]
            bgr = cv2.resize(bgr, (MAX_DISPLAY_WIDTH, int(MAX_DISPLAY_WIDTH * ratio)))

        print("Showing " + b["desc"]
              + ": %d * %d" % (w_h[0], w_h[1]))
        cv2.imshow("frame", bgr)

        # wasd keys control what is displayed,
        # anything else exits
        key = chr(cv2.waitKey(0) & 0xff)
        if key not in ['w', 'a', 's', 'd']:
            exit(0)
        else:
            if key == 'w':
                buf_i += 1
                if buf_i == len(bufs):
                    buf_i = 0
            elif key == 's':
                buf_i -= 1
                if buf_i < 0:
                    buf_i = len(bufs) - 1
            elif key == 'd':
                dims_i += 1
                if dims_i == len(b["top_4"]) - 1:
                    dims_i = 0
            elif key == 'a':
                dims_i -= 1
                if dims_i < 0:
                    dims_i = len(b["top_4"]) - 1
            

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
    buf2 = buf2 * 4

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

WASD controls.  WS cycle through decoding modes,
AD through guessed resolutions.  Any other key
to exit.
"""

    parser = argparse.ArgumentParser(description=description,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("filename",
                        help="image buffer dump")

    args = parser.parse_args()
    if not os.path.isfile(args.filename):
        print("file didn't exist: '%s'" % args.filename)
        exit(-1)

    return args


if __name__ == "__main__":
    main()
