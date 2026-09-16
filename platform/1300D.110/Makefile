MODEL = 1300D
FW_VERSION = 110

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

CONFIG_LVAPP_HACK_RELOC = y

ML_MODULES_SYM_NAME = t6_$(FW_VERSION).sym

MAIN_FIRMWARE_ADDR = 0xFE0C0000
PLATFORM_ARCH = armv5te

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
RESTARTSTART = 0xC80000
ML_BOOT_OBJ = boot-d45-ch.o

FIRMWARE_ID_1300D = 0x80000404
ML_SRC_EXTRA_OBJS += $(BUILD_DIR)/function_overrides.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete

