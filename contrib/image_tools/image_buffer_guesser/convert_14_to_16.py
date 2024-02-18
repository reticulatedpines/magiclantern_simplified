#!/usr/bin/env python3

import os
import argparse
import numpy as np


def main():
    args = parse_args()

    # in raw.h we have the sensor encoding, in bits:
    # 14-bit encoding:
    # hi          lo
    # aaaaaaaa aaaaaabb
    # bbbbbbbb bbbbcccc
    # cccccccc ccdddddd
    # dddddddd eeeeeeee
    # eeeeeeff ffffffff
    # ffffgggg gggggggg
    # gghhhhhh hhhhhhhh
    #
    # However, the above is 16 bit big endian.
    # Linear bytes look like:
    # aaaaaabb aaaaaaaa
    # bbbbcccc bbbbbbbb
    # ccdddddd cccccccc
    # eeeeeeee dddddddd
    # ffffffff eeeeeeff
    # gggggggg ffffgggg
    # hhhhhhhh gghhhhhh
        
    # we want to pad each 14 bits with 2 leading 0 bits, to convert
    # to 16 bit little endian data, which is more convenient for other
    # utils to deal with.

    with open(args.infile, "rb") as f:
        image_bytes = f.read()

    image_bytes_16 = convert_14_to_16(image_bytes)

    with open(args.outfile, "wb") as f:
        f.write(image_bytes_16)


def convert_14_to_16(buf):
    image_bytes = np.frombuffer(buf, dtype=np.ubyte)

    image_bytes.dtype = np.uint16
    image_bytes = image_bytes.byteswap()
    image_bytes.dtype = np.uint8

    bits_14 = np.unpackbits(image_bytes[:])
    # ensure divisible by 14 for later reshape
    bits_14 = extend_np_array(bits_14, 14)
    
    bits_0 = np.zeros(bits_14.size // 14 * 2, dtype=np.uint8)

    # Expand 14 bit to 16: reshape arrays into 14 bit and 2 bit wide columns,
    # concat the 2 bits to the left of the 14
    bits_14 = bits_14.reshape(-1, 14)
    bits_0 = bits_0.reshape(-1, 2)

    bits_16 = np.concatenate((bits_0, bits_14), axis=1)

    packed = np.packbits(bits_16)
    packed.dtype = np.uint16
    packed_LE = packed.byteswap()
    return packed_LE


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
    Canon sensors produce 14 bit data, OpenCV can't process this
    directly to debayer.  Extend each 14 bits into 16.
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("infile",
                        help="image data file")
    parser.add_argument("outfile",
                        help="output file")

    args = parser.parse_args()
    if not os.path.isfile(args.infile):
        print("file didn't exist: '%s'" % args.infile)
        exit()

    return args


if __name__ == "__main__":
    main()
