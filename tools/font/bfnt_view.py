#!/usr/bin/env python3

import os
import argparse
import struct
import binascii

import bitstring

def main():
    args = parse_args()

    with open(args.fontfile, "rb") as f:
        font_data = f.read()

    font = Font(font_data)
    if args.info:
        font.print_info()
    if args.char:
        font.print_char(args.char)


class FontError(Exception):
    pass


class Font(object):
    def __init__(self, font_data):
        """
        Expects a byte array of a Canon FNT bitmap font file.
        """
        self.font_data = font_data

        # FNT format is something like this:
        #
        # u32 magic: FNT\0
        # u16 unk_01
        # u16 font_width
        # u32 charmap_offset
        # u32 charmap_size # in bytes, char_count is this /4
        # u32 bitmap_size
        # char[16] name
        # u32[char_count] char_index # holds utf8, you match the char you want
                                     # in this array, and use the index to get
                                     # the offset from the next array
        # u32[char_count] char_offsets # offset to data for the char

        # parse header info
        self.magic, self.unk_01, self.font_width = struct.unpack("<iHH", font_data[0:8])
        self.index_offset, self.index_size = struct.unpack("<II", font_data[8:16])
        self.num_chars = self.index_size // 4
        self.bitmap_size, = struct.unpack("<I", font_data[16:20])
        name_bytes = font_data[20:36]
        self.name = "".join([chr(b) for b in name_bytes if b != b'\x00'])

        # get the data sections
        start = self.index_offset
        end = start + self.index_size
        char_index_raw = self.font_data[start:end]
        self.char_index = []
        i = 0
        while i < self.index_size:
            self.char_index.append(struct.unpack("<I", char_index_raw[i:i+4])[0])
            i += 4

        start = end
        end = start + self.index_size
        char_offsets_raw = self.font_data[start:end]
        self.char_offsets = []
        i = 0
        while i < self.index_size:
            self.char_offsets.append(struct.unpack("<I", char_offsets_raw[i:i+4])[0])
            i += 4
        # last item, we don't have the next item to use as end offset,
        # assume it is the same width as the prior item
        self.char_offsets.append(self.char_offsets[-1] + (self.char_offsets[-1] - self.char_offsets[-2]))

        start = end
        end = start + self.bitmap_size
        self.bitmap_data = self.font_data[start:end]

    def __repr__(self):
        s = ""
        magic_bytes = self.magic.to_bytes(4, "little")
        magic_str = "".join([chr(b) for b in magic_bytes])
        s += "magic:\t %s\n" % magic_str
        s += "unk_01:\t %s\n" % hex(self.unk_01)
        s += "font_width:\t %d\n" % self.font_width
        s += "charmap_offset:\t %s\n" % hex(self.index_offset)
        s += "charmap_size:\t %s\n" % hex(self.index_size)
        s += "bitmap_size:\t %s\n" % hex(self.bitmap_size)
        s += "num chars:\t %d\n" % self.num_chars
        s += "name:\t %s\n" % self.name
        return s

    def print_info(self):
        """
        Prints verbose info about the font
        """
        s = str(self)
        s += "\nchar map:\n"
        s += hex_dump(self.font_data[self.index_offset:self.index_offset + self.index_size],
                      initial_offset=self.index_offset,
                      utf_col=True)
        s += "\nchar index:\n"
        index_start = self.index_offset + self.index_size
        s += hex_dump(self.font_data[index_start:index_start + self.index_size],
                      initial_offset=index_start)
        print(s)

    def print_char(self, c):
        """
        Show the bitmap for a char, by index
        """
        # Each char is like this:
        #
        # u16 width # in bits
        # u16 height # in bits
        # u16 display_width # in bits
        # u16 x_offset # in bits
        # u16 y_offset # in bits
        # 
        # Each row in the output bitmap is aligned to a byte boundary,
        # so if width is 20, each row is packed into 3 bytes;
        # bits 8 + 8 + 4 are used, last 4 are padding.

        if c > self.num_chars - 1:
            return

        start = self.char_offsets[c]
        end = self.char_offsets[c + 1]
        #print(hex_dump(self.bitmap_data[start:end]))

        c_width, c_height, disp_width, x_off, y_off = struct.unpack("<HHHHH", self.bitmap_data[start:start + 10])
        padding_bits = 0
        padding = 0
        if c_width % 8:
            padding_bits = 8 - c_width % 8
            padding = 1
        bytes_per_row = c_width // 8 + padding

        rows = []
        #print(hex_dump(self.bitmap_data[start:end]))
        i = start + 10
        while i < end:
            rows.append(bitstring.BitArray(self.bitmap_data[i:i + bytes_per_row]))
            i += bytes_per_row

        b_string = ""
        for r in rows:
            for b in r[0: bytes_per_row * 8 - padding_bits]:
                if b:
                    b_string += "*"
                else:
                    b_string += " "
            b_string += "\n"
        print(b_string)
        print(struct.pack("<I", self.char_index[c]).decode("utf8"))


def hex_dump(byte_data, initial_offset=0, utf_col=False):
    """ Convenience function, returns a pretty-printed string
        of the supplied bytes """
    row_len = 16
    i = 0
    row = byte_data[i:i + row_len]
    ret_s = ""
    while(row):
        row_str = ""
        for b in row:
            if b > 0x1f and b < 0x7f:
                row_str += chr(b)
            else:
                row_str += "."
        row_utf = ""
        if utf_col:
            row_utf = " : "
            row_utf += row[0:4].decode("utf8") + u" " + row[4:8].decode("utf8") + u" "
            row_utf += row[8:12].decode("utf8") + u" " + row[12:16].decode("utf8")
        ret_s += "%08x : " % (i + initial_offset) + binascii.hexlify(row, " ").decode("utf8") + \
                 " : " + row_str + row_utf + "\n"
        i += row_len
        row = byte_data[i:i + row_len]
    return ret_s


def parse_args():
    description = '''Simple viewer for FNT format bitmap fonts
    '''

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("fontfile",
                        help="font file (maybe extracted from ROM with contrib/indy/find_fnt.py)")

    parser.add_argument("-i", "--info",
                        help="print summary info about the font",
                        default=False,
                        action="store_true")

    parser.add_argument("-c", "--char",
                        help="print character at given index, default: don't print",
                        default=0,
                        type=int)

    args = parser.parse_args()
    if not os.path.isfile(args.fontfile):
        print("font file didn't exist: '%s'" % args.fontfile)
        exit()

    return args


if __name__ == "__main__":
    main()
