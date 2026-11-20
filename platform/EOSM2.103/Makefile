MODEL = EOSM2
FW_VERSION = 103

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

#CANON_FIR = EOSM2103.FIR
MAIN_FIRMWARE_ADDR = 0xFF0C0000
ROMBASEADDR = 0xFF0C0000
PLATFORM_ARCH = armv5te

# Load ML at the beginning of the AllocateMemory pool
# default 0x44C000 - 0xC2A000, patched to 0x4E0000 - 0xC2A000 (592K for us).
RESTARTSTART = 0x44C100
ML_BOOT_OBJ = boot-d45-am.o
ML_SRC_EXTRA_OBJS = $(BUILD_DIR)/reloc.o

ML_AF_PATTERNS_OBJ = n

# Strict FAT32 8.3 short filename (DryOS FIO rejects 9-char "EOSM2_103")
ML_MODULES_SYM_NAME = EOSM2103.SYM

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
