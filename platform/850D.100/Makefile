MODEL = 850D
FW_VERSION = 100

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

#CANON_FIR  = CCF18101.FIR
FIRMWARE_ID = 0x80000435
#UPDATE_NAME_FIR = LOG_850D.FIR
MAIN_FIRMWARE_ADDR = 0xE0040000

#CONFIG_MMIO_TRACE = y

# user_mem_start from cstart struct. Align to 0x10 up
RESTARTSTART = 0x171a40

# Cortex A9, binaries loaded as Thumb
# MIDR: 0x414fc091
PLATFORM_ARCH = armv7-a
CFLAGS_PLATFORM += -mthumb -mlong-calls

ML_BOOT_OBJ = boot-d678.o
ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
