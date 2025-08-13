MODEL = 550D
FW_VERSION = 109

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

FIRMWARE_ID_550D = 0x80000270

#CANON_FIR = e8kr7109.fir

MAIN_FIRMWARE_ADDR = 0xFF010000
PLATFORM_ARCH = armv5te

# For injecting ML into AllocateMemory (system memory) pool - which is much bigger; 
# The malloc pool is very small on 550D and creates problems when Canon firmware tries to malloc.
# They don't always check for successful mallocs, and this causes weird bugs - camera acting like drunk :)
RESTARTSTART = 0xC70100
ML_BOOT_OBJ = boot-d45-am.o

ML_PICSTYLE_AUTO = n

CONFIG_LVAPP_HACK_RELOC = y

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
