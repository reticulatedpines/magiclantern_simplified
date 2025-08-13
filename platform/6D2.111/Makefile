MODEL = 6D2
FW_VERSION = 111

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

#CANON_FIR  = 6D200111.FIR
FIRMWARE_ID     = 0x80000406
#UPDATE_NAME_FIR = BLNK_6D2.FIR
MAIN_FIRMWARE_ADDR = 0xE0040000

# RESTARTSTART is the location in mem that ML binary will be copied to.
# This should be user_mem_start from cstart struct, aligned up to 0x10.
RESTARTSTART = 0xe0f90

# Cortex A9, binaries loaded as Thumb
PLATFORM_ARCH = armv7-a
CFLAGS_PLATFORM += -mthumb -mlong-calls

ML_BOOT_OBJ = boot-d678.o
ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
