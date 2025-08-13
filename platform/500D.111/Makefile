MODEL=500D
FW_VERSION=111

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

ML_BOOT_OBJ = boot-d45.o

MAIN_FIRMWARE_ADDR = 0xFF010000
PLATFORM_ARCH = armv5te

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
RESTARTSTART = 0x4D000

ML_PICSTYLE_AUTO = n

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
