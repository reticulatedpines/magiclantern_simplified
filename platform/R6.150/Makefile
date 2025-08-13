MODEL = R6
FW_VERSION = 150

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

FIRMWARE_ID = 0x80000453
MAIN_FIRMWARE_ADDR = 0xE0100000

# user_mem_start from cstart struct. Align to 0x10 up
RESTARTSTART = 0x190010

# Cortex A9, binaries loaded as Thumb
CFLAGS_PLATFORM = -mthumb -mlong-calls
PLATFORM_ARCH = armv7-a

ML_BOOT_OBJ = boot-d678.o #well R6 is digic X
ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
