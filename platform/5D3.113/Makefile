MODEL = 5D3
FW_VERSION = 113

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120
ML_BOOT_OBJ = boot-d45.o

MAIN_FIRMWARE_ADDR = 0xFF0C0000
PLATFORM_ARCH = armv5te

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
RESTARTSTART = 0x69500

ML_BITRATE_OBJ = bitrate-5d3.o
ML_AF_PATTERNS_OBJ = n

ML_SRC_EXTRA_OBJS = $(BUILD_DIR)/video_hacks.o $(BUILD_DIR)/afma.o $(BUILD_DIR)/asm.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
