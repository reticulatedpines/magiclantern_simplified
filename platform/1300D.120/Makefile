#Makefile for 1300D

MODEL = 1300D
FW_VERSION = 120

TOP_DIR = ../..

# WTF
AUTOEXEC_BASE = 0x800000
# WTF
FIR_BASE = 0x800120

MAIN_FIRMWARE_ADDR = 0xFE0C0000
PLATFORM_ARCH = armv5te

# Load ML at the end of the AllocateMemory pool
# Default 0x2D0000 - 0xD00000, patched to 0x2D0000 - 0xC80000 (512K for us).
RESTARTSTART = 0xC80000
ML_BOOT_OBJ = boot-d45-ch.o
ML_SRC_EXTRA_OBJS = $(BUILD_DIR)/reloc.o

ML_SRC_EXTRA_OBJS += function_overrides.o

# 1300D_101.sym is to long
ML_MODULES_SYM_NAME=t6_$(FW_VERSION).sym

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
