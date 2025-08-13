MODEL = 750D
FW_VERSION = 110
TOP_DIR = ../..

CANON_FIR  = CCF23110.FIR

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

MAIN_FIRMWARE_ADDR = 0xfe0a0000
PLATFORM_ARCH = armv7-r
CFLAGS_PLATFORM += -mthumb -mlong-calls

# user_mem_start from cstart struct. Align to 0x10 up
RESTARTSTART = 0x1cc400
ML_BOOT_OBJ = boot-d678.o
ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/$(INSTALLER_FIR)
