#!/usr/bin/env python3

"""
* 25sep2021, kitor : initial implementation for RP
* 01oct2021, coon  : RP config added (RP)
* 02oct2021, kitor : Rewritten to make unversal for Digic 7 and 8, added more configs

Decode DmacInfo and related structures in Digic 6 and 7
See https://www.magiclantern.fm/forum/index.php?topic=26249.0
"""

import sys, argparse, os
import logging
from pprint import pprint

THUMB_FLAG = 0x1
BOOMER_UNDEFINED = 0xFFFFFFFF
BOOMER_InSelTypeMask = 0x0000FF00
BOOMER_AssertInfoMask = 0xFFFF0000

class Globals:
    file = None
    fileSize = 0

class Flags:
    def __init__(self, mapping, unkn_prefix):
        self._mapping = {}

        for i in range(0, 32):
            flag = 1 << i
            if i in mapping.keys():
                desc = mapping[i]
            else:
                desc = "{}0x{:02x}".format(unkn_prefix, i)

            self._mapping[flag] = (i, desc)


    def getSetFlags(self, flags):
        out = {}
        for e in self._mapping.keys():
            if self._is_set(flags, e):
                index = self._mapping[e][0]
                desc = self._mapping[e][1]
                out[index] = desc

        return out


    @staticmethod
    def _is_set(x,n):
        return x & n != 0

class CPU:
    class DIGIC():
        # I left stuff as class attributes as anyway only one instance will be created
        ROM_BASE = 0x0
        ROM_LEN  = 0x0
        IVT_len  = 0x0
        flags    = {}
        addressTable = []

        _config   = {}
        _lengths = {
            "IVT"            : 0,
            "Channels"       : 0,
        }

        def __init__(self, config):
            self._config = config

            # Create address table used for in-file lookup of memory space addresses
            self.ROM_LEN = Globals.fileSize
            romcpy = self._config["romcpy"] if "romcpy" in self._config.keys() else {}
            self.addressTable = self._createAddressTable()


        def _createAddressTable(self):
            # add mappings
            if "romcpy" in self._config.keys():
                for e in self._config["romcpy"].keys():
                    start = e
                    end = e + self._config["romcpy"][e][1]
                    file_base = self._config["romcpy"][e][0]

                    self.addressTable.append((start, end, file_base))

            # add rom itself
            self.addressTable.append((self.ROM_BASE, self.ROM_BASE + self.ROM_LEN, 0))

            logging.debug("AddressTable:")
            for e in self.addressTable:
                logging.debug("   0x{:08x} 0x{:08x} 0x{:08x}".format(e[0],e[1],e[2]))

            return self.addressTable


        def getFileOffset(self, address):
            # Converts memory space address into in-file offset based on mappings
            for e in self.addressTable:
                if address >= e[0] and address < e[1]:
                    fileOffset = address - e[0] + e[2]
                    logging.debug("Mapping for 0x{:08x}: 0x{:08x}".format(address, fileOffset))
                    return fileOffset

            raise Exception("Address offset not found: 0x{:08x}".format(address))


        def getTotalChannels(self):
            return self._lengths["Channels"]


        def getISRs(self):
            return self._config["ISRs"]


        def printISRs(self):
            # Prints ISRs from config
            print("Known ISRs:")
            print("   ADDR   |  NAME")
            for i in self._config["ISRs"].keys():
                name = self._config["ISRs"][i]
                print("0x{:08x} {}".format(i, name))


        def getDmacInfo(self):
            # Loads DmacInfo structure from ROM
            out = {}
            with open(Globals.file, "rb") as rom_file:
                offset = self.getFileOffset(self._config["DmacInfo"])
                rom_file.seek(offset, 0)
                print("DmacInfo start: 0x{:08x}".format(offset))

                for i in range(self.getTotalChannels()):
                    addr = rom_file.read(4)
                    flag = rom_file.read(4)

                    addr = int.from_bytes(addr, byteorder='little')
                    flag = int.from_bytes(flag, byteorder='little')

                    out[i] = [addr, flag]
            return out


        def getInterruptHandlers(self):
            # Loads InterruptHandlers structure from ROM
            out = {}
            with open(Globals.file, "rb") as rom_file:
                offset = self.getFileOffset(self._config["InterruptHandlers"])
                rom_file.seek(offset, 0)
                print("InterruptHandlers start: 0x{:08x}".format(offset))
                for i in range(self.getTotalChannels()):
                    unk = rom_file.read(4)
                    cbr = rom_file.read(4)

                    unk = int.from_bytes(unk, byteorder='little')
                    cbr = int.from_bytes(cbr, byteorder='little')
                    out[i] = [unk, cbr]
            return out


        def getIVT(self):
            # Loads Interrupt Vector Table structure from ROM
            out = {}
            with open(Globals.file, "rb") as rom_file:
                offset = self.getFileOffset(self._config["IVT"])
                rom_file.seek(offset, 0)
                print("IVT start: 0x{:08x}".format(offset))
                for i in range(self._lengths["IVT"]):
                    ptr    = rom_file.read(4)
                    ptr    = int.from_bytes(ptr, byteorder='little')
                    out[i] = ptr

                offset = self.getFileOffset(self._config["IVT_GIC"])
                rom_file.seek(offset, 0)
                print("IVT_GIC start: 0x{:08x}".format(offset))
                start = len(out)
                end = start + self._lengths["IVT_GIC"]
                for i in range(start, end):
                    ptr    = rom_file.read(4)
                    ptr    = int.from_bytes(ptr, byteorder='little')
                    out[i] = ptr

                # get description strings for each entry
                for id in out:
                    ptr = self.getFileOffset(out[id])
                    rom_file.seek(ptr, 0)
                    msg = rom_file.read(0x40)
                    msg = msg.decode().split("\0")[0]
                    out[id] = msg

                logging.debug("Interrupt Vector Table:")
                for id in out:
                    logging.debug("{} {}".format(id, out[id]))

            return out


    class DIGIC_6(DIGIC):
        # So far DIGIC 6 seems to use different config structures
        def __init__(self, config):
            raise Exception("DIGIC 6 not supported!")


    class DIGIC_7(DIGIC):
        # Stuff specific to DIGIC 7

        ROM_BASE = 0xE0000000

        _lengths = {
            "IVT"            : 448,
            "IVT_GIC"        : 64,
            "Channels"       : 53,
        }

        flags = Flags({
            0x0  : "INFO_DMAC_TYPE_WRITE",
            0x1  : "INFO_DMAC_TYPE_READ",
            0x4  : "INFO_DMAC_S_C",
            0x5  : "INFO_DMAC_M",
            0x6  : "INFO_DMAC_L",
            0x7  : "INFO_DMAC_OPT",
            0x8  : "INFO_DMAC_SHREK",
            0x9  : "INFO_DMAC_DANCING",
            0xA  : "INFO_DIV_MODE",
            0xB  : "INFO_HDIR_MODE",
            0xC  : "INFO_HCOPY_MODE",
            0xD  : "INFO_FRAME_MODE",
            0xE  : "INFO_DISTER_MODE",
            0x11 : "INFO_BANK_MODE",
        }, "__INFO_")


    class DIGIC_8(DIGIC):
        # Stuff specific to DIGIC 8

        ROM_BASE    = 0xE0000000

        _lengths = {
            "IVT"            : 512,
            "IVT_GIC"        : 64,
            "Channels"       : 76,
            "PackUnpack"     : 0,
            "BoomerVdKick"   : 265,
            "BoomerSelector" : 225,
        }

        # For easier Ghidra decomp reading:
        # 0x0e INFO_VITON_MODE
        # 0x0F INFO_OPTI_MODE
        # 0x13 INFO_XSYS_DIV_MODE
        # 0x14 INFO_DIV_MODE
        # 0x15 INFO_32BIT_MODE
        # 0x16 INFO_64BIT_MODE
        # 0x17 INFO_128BIT_MODE
        # 0x18 INFO_DMAC_DANCING
        # 0x1D INFO_DMAC_SS
        # 0x1E INFO_DMAC_TYPE_READ
        # & 1  INFO_DMAC_TYPE_WRITE
        flags = Flags({
            0x0  : "INFO_DMAC_TYPE_WRITE",
            0x1  : "INFO_DMAC_TYPE_READ",
            0x2  : "INFO_DMAC_SS",
            0x7  : "INFO_DMAC_DANCING",
            0x8  : "INFO_128BIT_MODE",
            0x9  : "INFO_64BIT_MODE",
            0xA  : "INFO_32BIT_MODE",
            0xB  : "INFO_DIV_MODE",
            0xC  : "INFO_XSYS_DIV_MODE",
            0x10 : "INFO_OPTI_MODE",
            0x11 : "INFO_VITON_MODE",
        }, "__INFO_")

        # Engine::Edmac.c
        PackUnpackModeFlags = Flags({
            0x0  : "INFO_PACK_UNPACK_MODE",
            0x1  : "INFO_PACK_UNPACK_XMODE",
        }, "__UNKNOWN_")

        # Engine::BoomerVdKick.c
        BoomerVdType = {
            0x1 : "E_BOOMER_VD_KICK",
        }

        # Engine::ChaseCtl.c, no relation found so far to EDMAC ID
        ChasePort = {
            0x2e : "ELD_EDMAC_CHASER_EMPTY"
        }

        # Engine::BoomerSreset.c, no relation found so far to EDMAC ID
        # This is matched to BoomerID
        SelectId = {
            0xD70000: "ELD_BOOMER_DAFIGARO_1",
            0xD80000: "ELD_BOOMER_DAFIGARO_2",
            0xD90000: "ELD_BOOMER_DAFIGARO_3",
        }

        def __init__(self, config):
            self._config = config
            # pass config to base class
            super().__init__(config)


        def getTotalBoomerVdKick(self):
            return self._lengths["BoomerVdKick"]


        def getTotalBoomerSelector(self):
            return self._lengths["BoomerSelector"]


        def getPackUnpackId(self):
            arr = []
            with open(Globals.file, "rb") as rom_file:
                offset = self.getFileOffset(self._config["PackUnpackId"])
                rom_file.seek(offset, 0)
                print("PackUnpackId start: 0x{:08x}".format(offset))
                for i in range(self.getTotalChannels()):
                    id = rom_file.read(4)
                    id = int.from_bytes(id, byteorder='little')
                    arr.append(id)
            Globals.TotalPackUnpack = max(arr)
            return arr


        def getPackUnpackInfo(self):
            arr = {}
            with open(Globals.file, "rb") as rom_file:
                offset = self.getFileOffset(self._config["PackUnpackInfo"])
                rom_file.seek(offset, 0)
                print("PackUnpackInfo start: 0x{:08x}".format(offset))
                for i in range(self.getTotalChannels()):
                    ptr = rom_file.read(4)
                    unk = rom_file.read(4)
                    inf = rom_file.read(4)

                    ptr = int.from_bytes(ptr, byteorder='little')
                    unk = int.from_bytes(unk, byteorder='little')
                    inf = int.from_bytes(inf, byteorder='little')
                    arr[i] = [ptr, unk, inf]
            return arr;


        # BoomerID
        # 0xFFFFFFFF: BOOMER_UNDEFINED
        #
        # InSelType
        # flags | 0x0000FF00 InSelType
        # flags | 0xFFFF0000 AssertInfo
        def getDmacBoomerInfo(self):
            arr = {}
            with open(Globals.file, "rb") as rom_file:
                offset = self.getFileOffset(self._config["DmacBoomerInfo"])
                rom_file.seek(offset, 0)
                print("DmacBoomerInfo start: 0x{:08x}".format(offset))
                for i in range(self.getTotalChannels()):
                    id    = rom_file.read(4)
                    type  = rom_file.read(4)
                    edmac = rom_file.read(4)

                    id     = int.from_bytes(id, byteorder='little')
                    type   = int.from_bytes(type, byteorder='little')
                    edmac  = int.from_bytes(edmac, byteorder='little')
                    arr[i] = [id, type, edmac]
            return arr;


        def getBoomerVdKickInfo(self):
            arr = {}
            with open(Globals.file, "rb") as rom_file:
                offset = self.getFileOffset(self._config["BoomerVdKickInfo"])
                rom_file.seek(offset, 0)
                print("BoomerVdKickInfo start: 0x{:08x}".format(offset))
                for i in range(self.getTotalBoomerVdKick()):
                    VdType = rom_file.read(4)
                    addr1  = rom_file.read(4)
                    addr2  = rom_file.read(4)

                    VdType = int.from_bytes(VdType, byteorder='little')
                    addr1  = int.from_bytes(addr1, byteorder='little')
                    addr2  = int.from_bytes(addr2, byteorder='little')
                    arr[i] = [VdType, addr1, addr2]
            return arr;

        def getBoomerSelector1(self):
            arr = []
            with open(Globals.file, "rb") as rom_file:
                offset = self.getFileOffset(self._config["BoomerSelector1"])
                rom_file.seek(offset, 0)
                print("BoomerSelector1 start: 0x{:08x}".format(offset))
                for i in range(self.getTotalBoomerSelector()):
                    id = rom_file.read(4)
                    id = int.from_bytes(id, byteorder='little')
                    arr.append(id)
            return arr

        def getBoomerInputPort(self):
            arr = []
            with open(Globals.file, "rb") as rom_file:
                offset = self.getFileOffset(self._config["BoomerInputPort"])
                rom_file.seek(offset, 0)
                print("BoomerInputPort start: 0x{:08x}".format(offset))
                for i in range(self.getTotalBoomerSelector()):
                    id = rom_file.read(4)
                    id = int.from_bytes(id, byteorder='little')
                    arr.append(id)
            return arr

#
# CONFIGS
#
configs = {
    "200D_101" : {
        "CPU"               : CPU.DIGIC_7,
        "IVT"               : 0x202b8,
        "IVT_GIC"           : 0x209b8,

        # those are in order in single block
        "DmacInfo"          : 0x66264,
        "InterruptHandlers" : 0x6640c,

        "ISRs" : {
            0x36f70 | THUMB_FLAG : "EDMAC_ReadISR",
            0x36ed3 | THUMB_FLAG : "EDMAC_WriteISR",
        },

        # format: dst : (stkip, count), like romcpy.sh
        "romcpy" : {
            0x4000 : (0x11585B0, 0x68C10),
        },
    },

    "M50_110" : {
        "CPU"               : CPU.DIGIC_8,
        "IVT"               : 0x180b8,
        "IVT_GIC"           : 0x18cac,

        # those are in order in single block
        "UnknArr"           : 0xe0dd7848,
        "PackUnpackId"      : 0xe0dd7a70,
        "DmacInfo"          : 0xe0dd7ba0,
        "PackUnpackInfo"    : 0xe0dd7e00,
        "DmacBoomerInfo"    : 0xe0dd7fc8,
        "InterruptHandlers" : 0xe0dd8358,

        # those are in order in single block
        "BoomerSelector1"   : 0xe0fc40e0,
        "BoomerInputPort"   : 0xe0fc4464,
        "BoomerVdKickInfo"  : 0xe0fc47e8,

        "ISRs" : {
            0xe054bdd2 | THUMB_FLAG : "EDMAC_ReadISR",
            0xe054be8c | THUMB_FLAG : "EDMAC_WriteISR",
            0xe054a13e | THUMB_FLAG : "EDMAC_UnknownISR",
        },

        # format: dst : (stkip, count), like romcpy.sh
        "romcpy" : {
            0x4000 : (0x12D7170, 0x511E8),
        },
    },

    "R_180" : {
        "CPU"               : CPU.DIGIC_8,
        "IVT"               : 0x19390,
        "IVT_GIC"           : 0x19d34,

        # those are in order in single block
        "UnkArr"            : 0xe0dd590c,
        "PackUnpackId"      : 0xe0dd5b34,
        "DmacInfo"          : 0xE0DD5C64,
        "PackUnpackInfo"    : 0xe0dd5ec4,
        "DmacBoomerInfo"    : 0xe0dd608c,
        "InterruptHandlers" : 0xE0DD641C,

        # those are in order in single block
        "BoomerSelector1"   : 0xe0f72e08,
        "BoomerInputPort"   : 0xe0f7318c,
        "BoomerVdKickInfo"  : 0xe0f73510,

        "ISRs" : {
            0xe05378d6 | THUMB_FLAG : "EDMAC_ReadISR",
            0xe0537990 | THUMB_FLAG : "EDMAC_WriteISR",
            0xe0535c4b | THUMB_FLAG : "EDMAC_UnknownISR",
        },

        # format: dst : (stkip, count), like romcpy.sh
        "romcpy" : {
            0x4000 : (0x12C4294, 0x23028),
        },
    },

    "RP_160" : {
        "CPU"               : CPU.DIGIC_8,
        "IVT"               : 0x1ba48,
        "IVT_GIC"           : 0x1c3ec,

        # those are in order in single block
        "UnkArr"            : 0xe0e1408c,
        "PackUnpackId"      : 0xe0e142b4,
        "DmacInfo"          : 0xe0e143e4,
        "PackUnpackInfo"    : 0xe0e14644,
        "DmacBoomerInfo"    : 0xe0e1480c,
        "InterruptHandlers" : 0xe0e14b9c,

        # those are in order in single block
        "BoomerSelector1"   : 0xe0fd06bc,
        "BoomerInputPort"   : 0xe0fd0a40,
        "BoomerVdKickInfo"  : 0xe0fd0dc4,

        "ISRs" : {
            0xe057d69c | THUMB_FLAG : "EDMAC_ReadISR",
            0xe057d754 | THUMB_FLAG : "EDMAC_WriteISR",
            0xe057bc2a | THUMB_FLAG : "EDMAC_UnknownISR",
        },

        # format: dst : (stkip, count), like romcpy.sh
        "romcpy" : {
            0x4000 : (0x12F6400, 0x247fc),
        },
    },

    "250D_100" : {
        "CPU"               : CPU.DIGIC_8,
        "IVT"               : 0x19F6C,
        "IVT_GIC"           : 0x1A910,

        # those are in order in single block
        "UnkArr"            : 0xe103cce8,
        "PackUnpackId"      : 0xe103cf10,
        "DmacInfo"          : 0xe103d040,
        "PackUnpackInfo"    : 0xe103d2a0,
        "DmacBoomerInfo"    : 0xe103d468,
        "InterruptHandlers" : 0xe103d7f8,

        # those are in order in single block
        "BoomerSelector1"   : 0xe12039e8,
        "BoomerInputPort"   : 0xe1203d6c,
        "BoomerVdKickInfo"  : 0xe12040f0,

        "ISRs" : {
            0xe05c8270 | THUMB_FLAG : "EDMAC_ReadISR",
            0xe05c8328 | THUMB_FLAG : "EDMAC_WriteISR",
            0xe05c6886 | THUMB_FLAG : "EDMAC_UnknownISR",
        },

        # format: dst : (stkip, count), like romcpy.sh
        "romcpy" : {
            0x4000 : (0x1549DF4, 0x5ccd0),
        },
    },
}


def decodeModeInfo(config):
    EOS = config["CPU"](config)

    DmacInfo         = EOS.getDmacInfo()
    ISRArr           = EOS.getInterruptHandlers()
    ISRs             = EOS.getISRs()
    IVT              = EOS.getIVT()

    # DIGIC 8
    if isinstance(EOS, CPU.DIGIC_8):
        PackUnpackId     = EOS.getPackUnpackId()
        PackUnpackInfo   = EOS.getPackUnpackInfo()
        DmacBoomerInfo   = EOS.getDmacBoomerInfo()

        BoomerVdKickInfo = EOS.getBoomerVdKickInfo()
        BoomerSelector1  = EOS.getBoomerSelector1()
        BoomerInputPort  = EOS.getBoomerInputPort()

    EOS.printISRs()

    for i in range(EOS.getTotalChannels()):
    #for i in range(0,1):
        print("================================================================")
        print("ID: {:>2}, addr: {}".format(i, hex(DmacInfo[i][0])))

        print("FLAGS {:#032b}".format( DmacInfo[i][1]))
        print("  INDEX: FLAG_NAME")
        flags = EOS.flags.getSetFlags(DmacInfo[i][1])
        for flag in flags.keys():
            print("     {:>2}: {}".format(flag, flags[flag]))
        print()

        # interrupts and handlers
        tmp = ISRs[ISRArr[i][1]] if ISRArr[i][1] in ISRs.keys() else "UNKNOWN_0x{:08x}".format(ISRArr[i][1])
        print("Interrupt")
        print("    ID : {} (0x{:03x})".format(IVT[ISRArr[i][0]], ISRArr[i][0]))
        print("    ISR: {}".format(tmp))
        print()

        # skip Boomer for non DIGIC8
        if not isinstance(EOS, CPU.DIGIC_8):
            continue

        # PackUnpack
        puid = PackUnpackId[i]
        mode = PackUnpackInfo[puid][2]
        mode = EOS.PackUnpackModeFlags.getSetFlags(mode)
        if mode:
            print("PackUnpackInfo")
            print("    PackUnpackId      : {}".format(puid))
            print("    ptr               : {}".format(hex(PackUnpackInfo[puid][0])))
            print("    unk               : {}".format(PackUnpackInfo[puid][1]))
            print("PackUnpackInfoMode")
            for flag in mode.keys():
                print("    {:>2}: {}".format(flag, mode[flag]))
            print()

        BoomerID = DmacBoomerInfo[i][0];
        print("DmacBoomerInfo {}".format( "BOOMER_UNDEFINED" if BoomerID == BOOMER_UNDEFINED else hex(BoomerID)))
        if BoomerID != BOOMER_UNDEFINED:
            print("    BoomerInSelType         : 0x{:08x}".format(DmacBoomerInfo[i][1]))
            print("        `-- & InSelType     : 0x{:08x}".format(DmacBoomerInfo[i][1] & BOOMER_InSelTypeMask))
            print("    BoomerInSelEdmacType    : 0x{:08x}".format(DmacBoomerInfo[i][2]))
            print("        `-- & AssertInfo    : 0x{:08x}".format(DmacBoomerInfo[i][2] & BOOMER_AssertInfoMask))

            VdKickId = BoomerID >> 0x10
            print("BoomerVdKickInfo for BoomerID {} ({})".format(hex(BoomerID), hex(VdKickId)))
            VdType   = BoomerVdKickInfo[VdKickId][0]
            print("    VdType                  : {} {}".format(
                hex(VdType),
                EOS.BoomerVdType[VdType] if VdType in EOS.BoomerVdType.keys() else "__UNKNOWN_{}".format(hex(VdType))))
            print("    addr1                   : {}".format(hex(BoomerVdKickInfo[VdKickId][1])))
            print("    addr2                   : {}".format(hex(BoomerVdKickInfo[VdKickId][2])))

            if VdType == 0x1: # Two address arrays are defined only for E_BOOMER_VD_KICK
                print("BoomerSelector for {}".format(hex(BoomerID)))
                print("    addr1               : {}".format(hex(BoomerSelector1[VdKickId])))
                print("    addr2 (InputPort?)  : {}".format(hex(BoomerInputPort[VdKickId])))

        print()
    return

def main():
    parser = argparse.ArgumentParser(description="Decode DM buffers")


    file_args = parser.add_argument_group("Input file")
    file_args.add_argument("type", choices=configs.keys(), help="ROM type" )
    file_args.add_argument("file", default="ROM1.bin", help="ROM dump to analyze")

    parser.add_argument("--debug", "-d", action="store_true", help="Enable debug prints")
    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    Globals.file = args.file
    Globals.fileSize = os.path.getsize(Globals.file)

    decodeModeInfo(configs[args.type])

if __name__ == "__main__":
    main()
