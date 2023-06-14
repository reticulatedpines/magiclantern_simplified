#!/usr/bin/env python3

import os
import argparse

import cv2
import numpy as np

def main():
    args = parse_args()

    with open(args.filename, "rb") as f:
        yuv_buf = f.read()
#    yuv_buf = np.frombuffer(yuv_buf, dtype=np.uint8)
    yuv_buf = np.fromfile(args.filename, dtype=np.uint8)

    yuv_buf = yuv_buf.reshape(480, 736, 1)
    bgr = cv2.cvtColor(yuv_buf, cv2.COLOR_GRAY2BGR)

    cv2.imshow("frame", bgr)
    cv2.waitKey()


def parse_args():
    description = """
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("filename",
                        help="raw yuv data file")

    args = parser.parse_args()
    if not os.path.isfile(args.filename):
        print("yuv file didn't exist: '%s'" % args.filename)
        exit()

    return args


if __name__ == "__main__":
    main()
