MODEL = 6D
FW_VERSION = 116
TOP_DIR = ../..

CANON_FIR = 6D000116.FIR

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

MAIN_FIRMWARE_ADDR = 0xFF0C0000
PLATFORM_ARCH = armv5te

# Load ML at user_mem_start (aka heap start / DRY_HEAP_START / malloc memory pool)
# The malloc memory pool will be pushed to the right, so it will start after our BSS.
# user_mem_start: ff0c1c64: 000d7868
RESTARTSTART = 0xD7900
ML_BOOT_OBJ = boot-d45.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/$(INSTALLER_FIR)
