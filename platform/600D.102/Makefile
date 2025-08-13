MODEL = 600D
FW_VERSION = 102

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120
FIRMWARE_ID_600D = 0x80000286

MAIN_FIRMWARE_ADDR = 0xFF010000
PLATFORM_ARCH = armv5te

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
RESTARTSTART = 0xC70100
ML_BOOT_OBJ = boot-d45-ch.o

ML_AUDIO_OBJ = audio-lapis.o

CONFIG_LVAPP_HACK_RELOC = y

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
