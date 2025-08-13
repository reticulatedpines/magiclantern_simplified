MODEL = 200D
FW_VERSION = 101

FIRMWARE_ID     = 0x80000417
MAIN_FIRMWARE_ADDR = 0xE0040000

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

# user_mem_start from cstart struct. Align to 0x10 up
#RESTARTSTART = 0x000e1bd0
#ML_BOOT_OBJ     = boot-d678.o

# copying ML to AllocMem region, see PTR_ALLOC_MEM_START
RESTARTSTART = 0x4d0000 # Aligning the end at 0x10000 allows
                        # more efficient packing for preceding
                        # MMU structs
ML_BOOT_OBJ = boot-d678-am.o

# Cortex A9, binaries loaded as Thumb
# MIDR: 0x414fc091, see get_cpu_info() in arm-mcr.h
PLATFORM_ARCH = armv7-a
CFLAGS_PLATFORM += -mthumb -mlong-calls

ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o

TOP_DIR = ../..

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
