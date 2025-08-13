MODEL = R
FW_VERSION = 180

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

#CANON_FIR  = EOSR0180.FIR   # not good
#UPDATE_NAME_FIR = LOG_R180.FIR
FIRMWARE_ID = 0x00000424
MAIN_FIRMWARE_ADDR = 0xE0040000

# user_mem_start from cstart struct. Align to 0x10 up
RESTARTSTART = 0xb0080 # 0xb0078 in ROM

# Cortex A9, binaries loaded as Thumb
CFLAGS_PLATFORM = -mthumb -mlong-calls
PLATFORM_ARCH = armv7-a

ML_BOOT_OBJ = boot-d678.o
ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o $(BUILD_DIR)/test_features.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
