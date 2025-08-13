#!/usr/bin/env python3

import os
import argparse
import struct
import binascii

import bitstring

class BDF_Font_Error(Exception):
    pass

def main():
    args = parse_args()

    with open(args.input_font, "r") as f:
        font_data = f.readlines()

    bdf = BDF_Font(font_data)

    fnt = bdf.to_fnt()
    with open(args.output_font, "wb") as f:
        f.write(fnt)

    if args.char:
        bdf.print_char(args.char[0].encode("utf8"))



class BDF_Font(object):
    def __init__(self, font_lines):
        """
        Expects BDF font data as an array of lines / strings
        """
        self.font_lines = font_lines
        self.chars = {} # dict, keys are ints, e.g. 0x20 is space char

        # NB I didn't work from a standard, just a font file example.
        # Format is pretty simple so I'm just making best guesses.

        self.parse_header()

        i = 0
        while i < self.num_chars:
            self.parse_char()
            i += 1

        if self.num_chars != len(self.chars):
            raise BDF_Font_Error("Unexpected number of chars parsed.\nExpected, parsed: %d, %d" %
                                 self.num_chars, len(self.chars))

    def parse_header(self):
        if not self.font_lines[0].startswith("STARTFONT"):
            raise BDF_Font_Error("Missing STARTFONT magic")

        for i, line in enumerate(self.font_lines):
            if line.startswith("STARTPROPERTIES"):
                break
        self.num_properties = int(self.font_lines[i].split()[-1])
        i += self.num_properties + 2

        if not self.font_lines[i].startswith("CHARS"):
            raise BDF_Font_Error("Missing CHARS line")
        self.num_chars = int(self.font_lines[i].split()[-1])
        self.line_index = i
        
    def parse_char(self):
        """ using self.line_index, find the next char, add it,
            update self.line_index """
        # skip to next char
        while not self.font_lines[self.line_index].startswith("STARTCHAR"):
            self.line_index += 1
        # find "encoding", we use this as the key.  These seem to be UTF16,
        # bfnt uses UTF8
        while not self.font_lines[self.line_index].startswith("ENCODING"):
            self.line_index += 1
        encoding = int(self.font_lines[self.line_index].split()[-1])
        encoding = struct.pack("<H", encoding).decode("utf16").encode("utf8")
        self.chars[encoding] = []

        # skip to bitmap part of the char
        while not self.font_lines[self.line_index].startswith("BITMAP"):
            self.line_index += 1
        self.line_index += 1
        hex_chars = set("0123456789abcdefABCDEF")
        while not self.font_lines[self.line_index].startswith("ENDCHAR"):
            line_stripped = self.font_lines[self.line_index].strip()
            line = set(line_stripped)
            if not line:
                raise BDF_Font_Error("Empty BITMAP data line")
            if not line.issubset(hex_chars):
                raise BDF_Font_Error("BITMAP data line didn't look like hex chars: " +
                                     self.font_lines[self.line_index])
            # a2b_hex() will error if the input would produce output that
            # is not aligned to a byte boundary, we use this property elsewhere
            self.chars[encoding].append(binascii.a2b_hex(line_stripped))
            self.line_index += 1

    def print_char(self, char):
        """ takes a utf8 encoded char, prints that char from the font,
            or prints nothing if the char doesn't exist in font
        """
        if char not in self.chars:
            return

        rows = self.chars[char]
        bit_rows = []
        for r in rows:
            bit_rows.append(bitstring.BitArray(r))

        b_string = ""
        for row in bit_rows:
            for bit in row:
                if bit:
                    b_string += "*"
                else:
                    b_string += " "
            b_string += "\n"
        print(b_string)

    def to_fnt(self):
        """
        Return the bytes of an equivalent FNT font;
        serialise the BDF to FNT.

        Not all BDF can convert to FNT that meets ML
        expectations; ML code assumes all FNT will
        have some ASCII chars available.  We raise if
        we can't produce a FNT ML is happy with.
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

        # ML src/bmp.c, bfnt_find_char(), assumes fonts will have
        # these chars, sorted and as the first chars in the font
        required_chars = ""
        for c in range(0x20, 0x7b): # most of printable ASCII
            required_chars += chr(c)

        sorted_keys = sorted(self.chars)
        # handle the case where a font starts with a char lower than space,
        # but still has the chars we want.  We will ignore the earlier chars
        try:
            space_index = sorted_keys.index(bytes(" ", "utf8"))
        except ValueError:
            raise BDF_Font_Error("Input font missing required space character")
        first_chars = ""
        for c in sorted_keys[space_index:space_index + len(required_chars)]:
            first_chars += c.decode("utf8")
        if first_chars != required_chars:
            required_chars = set(required_chars)
            have_chars = {c.decode("utf8") for c in self.chars.keys()}
            missing_chars = required_chars.difference(have_chars)
            missing_chars = sorted(list(missing_chars))
            raise BDF_Font_Error("Input font missing required ASCII chars: %s" %
                                 missing_chars)

        char_index = b""
        char_offsets = b""
        char_count = 0
        chardata_bytes = b""
        for c in sorted_keys[space_index:]:
            bitmap = self.chars[c] # list of rows of bytes
            if len(c) > 4:
                # don't know how to encode into FNT, too long, skip it
                continue
            pad_bytes = b"\x00" * (4 - len(c))
            char_index += c + pad_bytes
            char_offsets += struct.pack("<I", len(chardata_bytes))
            char_count += 1

            # some of these it would be better to work out
            # from BBX in the BDF data
            char_width = len(bitmap[0]) * 8
            char_height = len(bitmap)
            display_width = char_width
            x_offset = 0
            y_offset = 0

            chardata_bytes += struct.pack("<H", char_width)
            chardata_bytes += struct.pack("<H", char_height)
            chardata_bytes += struct.pack("<H", display_width)
            chardata_bytes += struct.pack("<H", x_offset)
            chardata_bytes += struct.pack("<H", y_offset)

            # char data must be padded to byte boundary, but
            # we used binascii.a2b_hex() earlier which errors
            # if this is not already true in the input
            for row in bitmap:
                chardata_bytes += row
        print(char_count)
        print(len(chardata_bytes))

        # We now have all the character data, allowing us to generate
        # the FNT header.

        # FNT format is something like this:
        #
        # u32 magic: FNT\0
        # u16 unk_01 // seen 0xffe2 or 0xffd8.  Often, but not always, the inverse of the font_width
        # u16 font_width
        # u32 charmap_offset
        # u32 charmap_size # in bytes, char_count is this /4
        # u32 bitmap_size
        # char[16] name
        # u32[char_count] char_index # holds utf8, you match the char you want
                                     # in this array, and use the index to get
                                     # the offset from the next array
        # u32[char_count] char_offsets # offset to data for the char
        
        header = b"FNT\0"
        header += struct.pack("<H", 0xffe2)
        header += struct.pack("<H", char_width) # this uses the last chars width, should maybe use max?
        header += struct.pack("<I", 0x24)
        header += struct.pack("<I", char_count * 4)
        header += struct.pack("<I", len(chardata_bytes))
        font_name = "TestFont".ljust(16, "\x00")
        header += bytes(font_name, "utf8")[0:15] + b"\x00" # could mangle the encoded chars,
                                                           # but safe length and terminated
        header += char_index
        header += char_offsets
        return header + chardata_bytes

    def __repr__(self):
        s = ""
        return s

    def print_info(self):
        """
        Prints verbose info about the font
        """
        s = str(self)
        print(s)


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
    description = '''Converts a BDF font to FNT format
    '''

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("input_font",
                        help="BDF format font file")

    parser.add_argument("output_font",
                        help="FNT format font file to create")

    parser.add_argument("-i", "--info",
                        help="print summary info about the font",
                        default=False,
                        action="store_true")

    parser.add_argument("-c", "--char",
                        help="print given character from the font")

    args = parser.parse_args()
    if not os.path.isfile(args.input_font):
        print("font file didn't exist: '%s'" % args.input_font)
        exit()

    return args


if __name__ == "__main__":
    main()
