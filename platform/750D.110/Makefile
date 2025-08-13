MODEL = 750D
FW_VERSION = 110

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

#CANON_FIR = 750D0110.FIR
FIRMWARE_ID = 0x80000393
#UPDATE_NAME_FIR = CCF23110.FIR
MAIN_FIRMWARE_ADDR = 0xFE0A0000

# user_mem_start from cstart struct. Align to 0x10 up
RESTARTSTART = 0x001cc400

# Cortex R4
CFLAGS_PLATFORM += -mthumb -mlong-calls
PLATFORM_ARCH = armv7-r

ML_BOOT_OBJ = boot-d678.o
ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
