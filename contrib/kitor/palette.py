#!/usr/bin/env python3

"""
* kitor, 15aug2021

GUI viewer for Canon indexed palettes found in D8 roms. Probably applicable to D6+, may require modifications.
Limitation: does not render alpha channel right now.

Navigate with left/right arrow.

R180 contains GUI resources at 0xF0B40000, located in ROM1 (0xFB000000)

Resources start with header:

uint32_t count;
uint32_t unk1;

followed by list of resource offsets:
uint32_t offset;
size_t   rsc_size;

Offset base is same as GUI resources base.

Palette resources start with uint32_t 0x0 (unknown meaning). Then header follows:
uint16_t count;
uint16_t unkn;
uint32_t unk2;

followed by list of palette offsets and lengths (item entry count):
uint32_t count;
uint32_t offset;

This time offset is counted from list start address.

Palette entries have following format:
uint16_t index;
uint32_t ARGB;
Reason for index is unknown, all observed palettes have indexes incrementing from zero.
"""

import sys, argparse
from tkinter import *
from struct import unpack
from pprint import pprint


class Globals:
    file = None

    rsc_base = 0
    palette_base = 0
    palette_max = 0
    palette_id = 0
    palettes = []

    @classmethod
    def inc(cls):
        cls.palette_id += 1
        if cls.palette_id >= cls.palette_max:
            cls.palette_id = 0

    @classmethod
    def dec(cls):
        cls.palette_id -= 1
        if cls.palette_id < 0:
            cls.palette_id = cls.palette_max


class Application(Frame):
    """
    GUI class
    """

    def __init__(self, master=None):
        super().__init__(master)

        self.master = master
        self.cell_w = 20

        self.canvas = Canvas(self.master)
        self.canvas.pack()

        self.master.bind("<KeyRelease>", self.keyRelease)

        # load first palette
        self.loadPalette()

    def renderPalette(self, palette):
        """
        Render palette

        :param palette: List of hex string colors
        """
        # clear and resize canvas
        self.canvas.delete("all")
        ww = 16 * self.cell_w
        wh = (len(palette) / 16) * self.cell_w
        self.canvas.configure(width=ww, height=wh)

        # render color cells
        for i in range(0, len(palette)):
            x = (i % 16) * self.cell_w
            y = (int(i / 16)) * self.cell_w
            cx = x + self.cell_w
            cy = y + self.cell_w

            r = self.canvas.create_rectangle(x, y, cx, cy, fill=palette[i])

    def keyRelease(self, e):
        """
        key release keyrpess handler

        :param e: keypress event
        """
        if e.keycode == 100:  # left
            Globals.dec()
        if e.keycode == 102:  # right
            Globals.inc()

        self.loadPalette()

    def loadPalette(self):
        """
        Loads new palette and calls render
        """
        palette = loadPaletteResource(Globals.palette_id)
        self.master.title("Palette ID: {}".format(Globals.palette_id))
        self.renderPalette(palette)
        return


"""
shameless copy from @indy find_fnt.py
"""


def getLongLE(d, a):
    return unpack("<L", (d)[a : a + 4])[0]


def getShortLE(d, a):
    return unpack("<H", (d)[a : a + 2])[0]


def getResourceOffset(id):
    """
    Parses ROM1 to find file of given GUI resource ID

    :param id: Resource ID
    :return:   File offset
    """
    with open(Globals.file, "rb") as rom_file:
        rom_file.seek(Globals.rsc_base, 0)
        rsc_data = rom_file.read(4)
        types = getLongLE(rsc_data, 0)
        if id > types:
            print("Rsc ID {} > total {}".format(id, types))
            sys.exit(1)

        rom_file.read(4)  # skip unkn entry
        rscPtrs = []
        for i in range(0, types):
            rsc_off = rom_file.read(4)
            rsc_size = rom_file.read(4)
            rscPtrs.append(
                {"off": getLongLE(rsc_off, 0), "size": getLongLE(rsc_size, 0)}
            )

        pprint(rscPtrs)
        rsc_off = rscPtrs[args.id]["off"]
        print("Selected resource: {}, off: {}".format(id, rsc_off))
        return rsc_off


def loadPaletteList(off):
    """
    Parses palette resource header into palette list

    :param id: Resource ID
    :return:   File offset
    """
    if len(Globals.palettes) > 0:
        print("Palettes already loaded!")
        return

    with open(Globals.file, "rb") as rom_file:
        rom_file.seek(Globals.rsc_base + off, 0)
        # decode palette entry
        rom_file.read(4)  # skip header
        count = getShortLE(rom_file.read(2), 0)
        rom_file.read(2)  # skip uknown entry
        rom_file.read(4)

        Globals.palette_base = rom_file.tell()

        palettes = []

        for i in range(0, count):
            rsc_count = getLongLE(rom_file.read(4), 0)
            rsc_off = getLongLE(rom_file.read(4), 0)
            palettes.append({"off": Globals.palette_base + rsc_off, "count": rsc_count})

        print("Found {} palettes".format(count))
        return palettes


def loadPaletteResource(id):
    """
    Parses palette data into colors list

    :param id: Palette ID
    :return:   List of colors
    """
    print("Selected ID {}".format(id))
    if id > len(Globals.palettes):
        print("Palette ID > max_id")
        sys.exit(1)
    with open(Globals.file, "rb") as rom_file:
        rom_file.seek(Globals.palettes[id]["off"])

        palette = []
        for i in range(0, Globals.palettes[id]["count"]):
            entry_id = getShortLE(rom_file.read(2), 0)
            rgba = getLongLE(rom_file.read(4), 0)
            # cut opacity data
            rgb = (rgba & 0xFFFFFF00) >> 8
            # swap color bytes
            color = rgb & 0xFF00
            color = color | (rgb & 0xFF) << 16
            color = color | (rgb & 0xFF0000) >> 16
            palette.append("#%06x" % color)

        return palette


parser = argparse.ArgumentParser(description="Draw D8 indexed palettes")

file_args = parser.add_argument_group("Input file")
file_args.add_argument("file", default="ROM1.bin", help="ROM dump to analyze")
file_args.add_argument(
    "--address", "-a", default=0xF0000000, help="Load (memory) address of file."
)

file_args.add_argument("--assets", default=0xF0B40000, help="Address of GUI resources")
file_args.add_argument("--id", default=2, help="Resource ID")

file_args.add_argument("--pid", default=0, help="Palette ID")

args = parser.parse_args()

Globals.file = args.file
Globals.rsc_base = args.assets - args.address
Globals.palette_id = int(args.pid)

offset = getResourceOffset(int(args.id))
Globals.palettes = loadPaletteList(offset)
Globals.palette_max = len(Globals.palettes) - 1

root = Tk()
app = Application(master=root)
app.mainloop()
