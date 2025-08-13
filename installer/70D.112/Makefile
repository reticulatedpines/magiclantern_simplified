MODEL = 70D
FW_VERSION = 112
TOP_DIR = ../..

CANON_FIR = 70D00112.FIR

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

MAIN_FIRMWARE_ADDR = 0xff0c0000
PLATFORM_ARCH = armv5te

# Load ML at user_mem_start (aka heap start / DRY_HEAP_START / malloc memory pool)
# The malloc memory pool will be pushed to the right, so it will start after our BSS.
# user_mem_start: FF0C1C64: 0xFAE30
RESTARTSTART = 0xfaf00
ML_BOOT_OBJ = boot-d45.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/$(INSTALLER_FIR)
