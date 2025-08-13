MODEL = 80D
FW_VERSION = 103

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

#CANON_FIR  = 80D00103.FIR
FIRMWARE_ID     = 0x80000350
#UPDATE_NAME_FIR = BOOT_80D.FIR
MAIN_FIRMWARE_ADDR = 0xFE0A0000

# user_mem_start from cstart struct. Align to 0x10 up
RESTARTSTART = 0x1cc400

# Cortex R4
CFLAGS_PLATFORM += -mthumb -mlong-calls
PLATFORM_ARCH = armv7-r

ML_BOOT_OBJ = boot-d678.o
ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
