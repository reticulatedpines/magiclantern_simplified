MODEL = EOSM
FW_VERSION = 202

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120
ML_BOOT_OBJ = boot-d45.o

#CANON_FIR = EOSM1202.FIR

MAIN_FIRMWARE_ADDR = 0xFF0C0000
PLATFORM_ARCH = armv5te

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
RESTARTSTART = 0x9E1E0

ML_AF_PATTERNS_OBJ = n

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
