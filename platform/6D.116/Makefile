MODEL = 6D
FW_VERSION = 116

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

#CANON_FIR = 6D000116.FIR
MAIN_FIRMWARE_ADDR = 0xFF0C0000
PLATFORM_ARCH = armv5te

# Load ML at the beginning of the AllocateMemory pool
# default 0x44C000 - 0xD3C000, patched to 0x4E0000 - 0xD3C000 (592K for us).
RESTARTSTART = 0x44C100
ML_BOOT_OBJ = boot-d45-am.o

ML_BITRATE_OBJ = bitrate-5d3.o
ML_SOUND_CODEC_OBJ = ak4646.o

ML_SRC_EXTRA_OBJS = $(BUILD_DIR)/afma.o $(BUILD_DIR)/reloc.o $(BUILD_DIR)/gps.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
