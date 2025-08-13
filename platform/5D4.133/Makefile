MODEL = 5D4
FW_VERSION = 133

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

#CANON_FIR  = 5D400133.FIR
#UPDATE_NAME_FIR = DUMP_5D4.FIR
MAIN_FIRMWARE_ADDR = 0xFE0A0000

# RESTARTSTART is the location in mem that ML binary will be copied to.
# This should be user_mem_start from cstart struct, aligned up to 0x10.
RESTARTSTART    = 0x1cc400

# Cortex R4
CFLAGS_PLATFORM += -mthumb -mlong-calls
PLATFORM_ARCH = armv7-r

ML_BOOT_OBJ     = boot-d678.o
ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
