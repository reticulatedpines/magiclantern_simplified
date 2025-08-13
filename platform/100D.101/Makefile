MODEL = 100D
FW_VERSION = 101

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

#CANON_FIR = CCF16101.FIR
MAIN_FIRMWARE_ADDR = 0xFF0C0000
PLATFORM_ARCH = armv5te

# Load ML at the beginning of the AllocateMemory pool
# default 0x44C000 - 0xC3C000, patched to 0x4E0000 - 0xC3C000 (592K for us).
RESTARTSTART = 0x44C100
ML_BOOT_OBJ = boot-d45-am.o
ML_SRC_EXTRA_OBJS = $(BUILD_DIR)/reloc.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
