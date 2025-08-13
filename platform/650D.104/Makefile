MODEL = 650D
FW_VERSION = 104

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120
ML_BOOT_OBJ = boot-d45.o

MAIN_FIRMWARE_ADDR = 0xFF0C0000
PLATFORM_ARCH = armv5te

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
RESTARTSTART = 0x7E100

include  ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
