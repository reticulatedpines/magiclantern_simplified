#!/usr/bin/env python3

# Based on a1ex's v7mmap.py: https://a1ex.magiclantern.fm/bleeding-edge/R5/v7mmap.py
# Also see srsa's post about MMU: https://www.magiclantern.fm/forum/index.php?topic=25399.0
#
# Fundamentally based upon "ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition",
# I believe this is the relevant pseudo-code:
# https://developer.arm.com/documentation/ddi0406/b/System-Level-Architecture/Virtual-Memory-System-Architecture--VMSA-/Pseudocode-details-of-VMSA-memory-system-operations/Translation-table-walk

import os
import sys
import argparse
import functools
import struct
from collections import namedtuple

PageAttributes = namedtuple("PageAttributes", "virt_address phys_address size access_permissions texcb xn")

def main():
    args = parse_args()

    data = open(args.datafile, "rb").read()

    # What is TTBR?  This means Translation Table Base Register.
    # There are three,  TTBR0, TTBR1 and TTBCR.
    # TTBCR controls whether TTBR0 or TTBR1 is used to find the tables.
    #
    # The virtual address space can be split into two regions,
    # with TTBR0 defining the mappings for the lower addresses,
    # TTBR1 the upper.
    #
    # TTBR0 is saved/restored on context switches, TTBR1 is not.
    #
    # Canon sometimes sets these different per core.
    # On Digic 7, 8 and X dual-cores, we see Canon map a page of early
    # mem so each core sees VA 0x1000:0x2000 mapped to different
    # physical mem.  This is used for per core data.
    #
    # This also means VA 0 is unmapped and will fault on access,
    # which catches null pointer derefs.
    #
    # https://developer.arm.com/documentation/ddi0406/b/System-Level-Architecture/Virtual-Memory-System-Architecture--VMSA-/Translation-tables?lang=en

    ttbr1 = args.ttbr1 & 0xffffff00

    # Notes from srsa on standard Canon MMU tables:
    #
    # below translation table manipulation functions assume the Canon fw table arrangement:
    # - 0x4000 bytes of L1 table for both cores, describing addresses from 32MB upwards
    # - 0x400 bytes of L2 table describing address range 0...1024kB, core0
    # - 0x400 bytes of L2 table describing address range 0...1024kB, core1
    # - 0x80 bytes of L1 table describing address range 0...32MB, core0
    # - 0x80 bytes of L1 table describing address range 0...32MB, core1

    # So far, all Canon tables seen have TTBR1 tables immediately
    # before TTBR0 tables, and size 0x4800
    ttbr0_offset = 0x4800

    for cpu_id in [0, 1]:
        print("CPU%d" % cpu_id)

        last_offset = None
        vstart = 0
        pstart = 0
        size = 0
        last_ap = None
        last_xn = None
        last_texcb = None

        ttbr0 = ttbr1 + ttbr0_offset + (0x80 * cpu_id)
        # address, phys_addr, page_size, access_permissions, texcb, xn
        for a, p, s, ap, texcb, xn in walk_ttbrs(data, args.data_base_address,
                                                 [ttbr0, ttbr1],
                                                 verbose=args.verbose):
            offset = p - a
            s = 1024
            if offset == last_offset and ap == last_ap \
               and xn == last_xn and texcb == last_texcb \
               and a == vstart + size:
                size += s
            else:
                if last_offset is not None:
                    print("%08X-%08X -> %08X-%08X (%+5X) %-4s %s %s" %
                                (vstart, vstart+size-1, pstart, pstart+size-1,
                                last_offset, decode_texcb(last_texcb), decode_ap(last_ap),
                                decode_xn(last_xn)))
                vstart, pstart, size = a, p, s
                last_offset = offset
                last_ap = ap
                last_xn = xn
                last_texcb = texcb

        print("%08X-%08X -> %08X-%08X (%+5X) %-4s %s %s" % 
                    (vstart, vstart+size-1, pstart, pstart+size-1,
                    last_offset, decode_texcb(last_texcb), decode_ap(last_ap),
                    decode_xn(last_xn)))
        print("")


def parse_args():
    description = """
    For cams that have MMU tables in rom (all dual core D78X?),
    dump the VA -> PA mappings
    """

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("datafile",
                        help="path to ROM or mem dump to attempt MMU table parsing")
    parser.add_argument("-b", "--data-base-address",
                        help="Virtual address of the start of datafile, default: %(default)x",
                        default=0xe0000000,
                        type=functools.partial(int, base=0))
    parser.add_argument("--ttbr1",
                        help="Value of TTBR1: default: %(default)x",
                        default=0xe0000000,
                        type=functools.partial(int, base=0))
    parser.add_argument("-v", "--verbose",
                        default=False,
                        action="store_true")

    args = parser.parse_args()
    args.datafile = os.path.realpath(args.datafile)
    if not os.path.isfile(args.datafile):
        print("File didn't exist: '%s'" % args.datafile)
        sys.exit(-1)

    return args


def getLongLE(d, address):
    assert address > 0, "Expecting positive address, address was: 0x%x.  " \
                        "Was --data-base-address too high?" % address
    assert address < len(d), "Expecting address < len, was --data-base-address too low?"
    return struct.unpack('<L',d[address:address+4])[0]


def getByte(d, address):
    return ord(d[address])


def getString(d, address):
    return d[address : d.index('\0', address)]


def extract32(value, start, length):
    assert(start >= 0 and length > 0 and length <= 32 - start);
    return (value >> start) & (0xFFFFFFFF >> (32 - length));


def walk_ttbrs(data, data_base_address, ttbrs, verbose=False):
    # we assume "short-descriptor translation table format",
    # which may not be future proof.
    #
    # See ARM manual, B3.5.1 Short-descriptor translation table format descriptors

    for i, ttbr in enumerate(ttbrs):
        print("TTBR%d: %08X" % (i, ttbr))
        print("===============")
        if not ttbr:
            continue
        for address in range(0, 1<<32, 1024):
            table = 0
            if address >> (32-7):
                # TODO this shouldn't hard-code 7,
                # the ttbr0/1 split can vary (I think??)
                table = 1
            if table != i:
                continue

            base_mask = 0xffffff80
            if i == 1:
                base_mask = 0xffffc000

            entry_address = (ttbr & base_mask) | ((address >> 18) & 0x3ffc)
            #~ print hex(ttbr), hex(ttbr & base_mask), hex((address >> 18) & 0x3ffc), hex(entry_address)
            desc = getLongLE(data, entry_address - data_base_address)
            page_attributes = parse_descriptor(data, data_base_address, desc,
                                               address, entry_address, verbose=verbose)
            if not page_attributes:
                continue

            yield page_attributes


def parse_descriptor(data, rombase, desc, address, entry_address, verbose=False):
    # last two bits hold the type, see B3.5.1
    desc_type = desc & 3
    if verbose:
        print(("%08X [%08X]: %08X %s"
               % (address, entry_address, desc, bin(desc_type|4)[3:])), end=' ')

    #~ print hex(address), hex(entry_address), hex(desc), type
    if desc_type == 1: # Page table
        page_attributes = parse_page_table(data, rombase, desc, address, verbose=verbose)
    elif desc_type == 2: # Section or Supersection
        page_attributes = parse_section(data, rombase, desc, address, verbose=verbose)
    else: # 0, explicit fault, or 3, fault if implementation does not support PXN
        if verbose:
            print("Fault")
        return None

    return page_attributes


def parse_section(data, rombase, desc, address, verbose=False):
    """
    Parses the given descriptor, which should be type 2 / Section or Supersection.

    Returns the attributes for the page it resolves to.
    """
    # bit 18 determines section or supersection
    if desc & (1 << 18): # supersection
        # supersection address is encoded split over the word,
        # and may use 40-bit physical addresses (LPAE)
        page_addr = (desc & 0xff000000)
        page_addr |= extract32(desc, 20, 4) << 32;
        page_addr |= extract32(desc, 5, 4) << 36;
        phys_addr = page_addr | (address & 0x00ffffff);
        page_size = 0x1000000; # 16MB pages
        acc = desc & 0xffffff
        if verbose:
            if page_addr > 0xffffffff:
                print("INFO: >32-bit address found")
            print("Supersection", end=' ')
            print(" -> 16M page at %X -> %X" % (page_addr, phys_addr))
    else: # section
        page_addr = (desc & 0xfff00000)
        page_size = 0x100000; # 1MB pages
        phys_addr = page_addr | (address & 0x000fffff);
        acc = desc & 0xfffff
        if verbose:
            print("Section", end=' ')
            print(" -> 1M page at %X -> %X" % (page_addr, phys_addr))
    ap = ((desc >> 10) & 3) | ((desc >> 13) & 4);
    texcb = ((desc >> 2) & 3) | (desc >> 10) & 0x1C
    xn = (desc >> 4) & 1
    return PageAttributes(address, phys_addr, page_size, ap, texcb, xn)


def parse_page_table(data, rombase, desc, address, verbose=False):
    """
    Parses the given descriptor, which should be type 1 / page table.

    Returns the attributes for the page it resolves to, or None
    if it maps to Fault.
    """
    domain = (desc >> 5) & 0x0f
    sbz = desc & 0x1E
    assert sbz == 0
    table_addr = (desc & 0xfffffc00)
    l2_entry = table_addr | ((address >> 10) & 0x3fc);
    desc = getLongLE(data, l2_entry - rombase)
    if verbose:
        print("L2 table=%08X domain=%X entry=%X"
              % (table_addr, domain, l2_entry), end=' ')

    ap = ((desc >> 4) & 3) | ((desc >> 7) & 4);
    typ = (desc & 3)
    if typ == 0:
        if verbose:
            print(" -> fault")
        return None
    elif typ == 1:
        page_addr = (desc & 0xffff0000)
        page_size = 0x10000;
        phys_addr = page_addr | (address & 0xffff);
        acc = desc & 0xffff
        xn = desc & (1 << 15);
        texcb = ((desc >> 2) & 3) | (desc >> 10) & 0x1C
        if verbose:
            print(" -> 64K page at %X -> %X" % (page_addr, phys_addr))
    elif typ == 2 or typ == 3:
        page_addr = (desc & 0xfffff000)
        page_size = 0x1000;
        phys_addr = page_addr | (address & 0xfff);
        acc = desc & 0xfff
        xn = desc & 1;
        texcb = ((desc >> 2) & 3) | (desc >> 4) & 0x1C
        if verbose:
            print(" -> 4K page at %X -> %X" % (page_addr, phys_addr))
    ap = ((desc >> 4) & 3) | ((desc >> 7) & 4);
    return PageAttributes(address, phys_addr, page_size, ap, texcb, xn)


def decode_ap(ap):
    if (ap == 0b001):
        return "P:RW"
    if (ap == 0b101):
        return "P:R "
    if (ap == 0b011):
        return "RW  "
    print(bin(ap))
    assert False


def decode_cached_attr(x):
    if x == 0b00:
        #~ return "Non-cacheable"
        return "NCACH"
    if x == 0b01:
        #~ return "Write-Back, Write-Allocate"
        return "WB,WA"
    if x == 0b10:
        #~ return "Write-Through, no Write-Allocate"
        return "WT,WN"
    if x == 0b11:
        #~ return "Write-Back, no Write-Allocate"
        return "WB,WN"


def decode_texcb(texcb):
    # Notes on "texcb".
    #
    # "Bufferable (B), Cacheable (C), and Type Extension (TEX) bit names
    # are inherited from earlier versions of the architecture. These names
    # no longer adequately describe the function of the B, C, and TEX bits."
    #
    # Thanks, ARM manual.

    if texcb & 0b10000:
        return "O:%s I:%s " % (decode_cached_attr((texcb >> 2) & 3), decode_cached_attr(texcb & 3))
    if texcb == 0b00001:
        return "Device          "
    if texcb == 0b00000:
        return "Strongly-ordered"
    if verbose:
        print(bin(texcb))
    assert False


def decode_xn(xn):
    if xn:
        return "XN"
    return "  "


if __name__ == "__main__":
    main()
