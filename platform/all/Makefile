#Makefile for an autoexec.bin able to run on all cameras

# This makefile is broken after the make refactor.
# It's used for installer, and I think "rescue"

x: prepare-all autoexec-all autoexec.bin
#	cp autoexec-all.bin autoexec.bin
	$(call build,STAT,$(STAT_CMD) autoexec.bin)

prepare-all:
	$(MAKE) -C .. all-supported-unified-models

reboot-all.o : prepare-all

autoexec.bin: autoexec-all.bin
	$(call build,CP,$(CP) autoexec-all.bin autoexec.bin)

autoexec-all: reboot-all.o
	$(call build,LD,$(LD) \
		-o autoexec-all \
		-nostdlib \
		-e _start \
		-Ttext 0x800000 \
		$^ \
	); \

MODEL := UNIFIED

ML_MODEL_DIR = all


zip_contents:: $(PLATFORM_INSTALL_SUPPORTED_TARGETS) autoexec.bin
	$(call build,CP,$(CP) autoexec.bin $(INSTALL_DIR)/)

$(PLATFORM_INSTALL_ALL_TARGETS):
	$(call call_make_platform)

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

PLATFORM_ARCH = armv5te
ML_BOOT_OBJ = boot-d45.o

MAIN_FIRMWARE_ADDR = 0xFF010000

RESTARTSTART = 0

ML_SRC_REBOOT_OBJ = reboot-all.o

CONFIG_MODULES = n

include  ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
