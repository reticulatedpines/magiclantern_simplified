MODEL = 5D2
FW_VERSION = 212

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

FIRMWARE_ID	= 0x80000218

MAIN_FIRMWARE_ADDR = 0xFF810000
PLATFORM_ARCH = armv5te
ML_BOOT_OBJ = boot-d45.o

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
RESTARTSTART = 0x4E000

ML_PICSTYLE_AUTO = n

ML_SRC_EXTRA_OBJS = $(BUILD_DIR)/afma.o $(BUILD_DIR)/asm.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
