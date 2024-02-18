#!/usr/bin/env python3

# Adapted from:
# https://stackoverflow.com/questions/60867212/numpy-array-two-unknown-dimensions-png-files/60870168#60870168

import os
import argparse
import itertools
import numpy as np

MAX_ASPECT = 20

def main():
    args = parse_args()

#    image_bytes = np.fromfile(args.filename, dtype=np.ubyte)
    image_bytes = np.fromfile(args.filename, dtype=np.uint16)

    top_20_dims = get_size_guesses(image_bytes)
    # filter out outlandish aspect ratios
    best_dims = [x for x in top_20_dims if ((max(x[0], x[1]) // min(x[0], x[1]) < MAX_ASPECT))]

    print("Best guesses: %s" % best_dims)


def get_size_guesses(buf, planes=1):
    """
    Returns the top 20 guesses on image dimensions,
    as a list of (w, h) tuples.

    The planes arg is useful if you have some guess
    about image format.  E.g. YUV is 2 planes, RGB is 3.

    If the length of buf can't be converted into that
    number of planes, the last item in buf is repeatedly
    appended to a copy of buf to create a workable length buffer.

    Guesses are for the dimensions of a single plane,
    all planes being the same size.
    """
    buf_len = len(buf)
    r = buf_len % planes
    if r:
        buf2 = np.append(buf, [buf[-1]] * (planes - r))
    else:
        buf2 = buf

    planed_buf = buf2.reshape(-1, planes)
    planed_len = len(planed_buf)

    prime_factors = list(compute_prime_factors(planed_len))
    combinations = itertools.chain.from_iterable(
                        itertools.combinations(prime_factors, r=i)
                            for i in range(1, len(prime_factors)))
    row_dims = sorted({np.prod(x) for x in combinations})
    scores = [test_row_dimensions(buf2, r, planes) for r in row_dims]
    top_20_dims = [(planed_len // row_dims[s], row_dims[s])
                        for s in np.argsort(scores)[:20]]
    return top_20_dims
    

def compute_prime_factors(n):
    i = 2
    while i <= n:
        if n % i == 0:
            n //= i
            yield i
        else:
            i += 1


def test_row_dimensions(image, r, planes=1):
    c = len(image) // r
    if r < 10:
        return 0.0
    test = image.reshape(r, c // planes, planes)

    # for Bayer data, the pattern confuses the test,
    # which checks columns.  Skipping rows helps.
    if planes == 1 and r > 2:
        test = test[::2]

    return np.mean((test[1:] - test[:-1]) ** 2)


def parse_args():
    description = """
    Given a buffer of image data (no image headers, etc, just the data),
    attempts to automatically find the width of the image.
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("filename",
                        help="image data file")

    args = parser.parse_args()
    if not os.path.isfile(args.filename):
        print("file didn't exist: '%s'" % args.filename)
        exit()

    return args


if __name__ == "__main__":
    main()
