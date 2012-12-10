#Makefile for EOS M firmware 106 installer

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=106
MODEL=EOSM
UPDATE_NAME=EOSM106.fir 
ML_VERSION=unified1

#used in CFLAGS
PLATFORM_INC=../../platform/$(MODEL).$(FW_VERSION)

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

# magiclantern.lds script MUST be first
# entry.o MUST be second
# menu.o and debug.o must come before the modules
ML_OBJS-y = \
	magiclantern.lds \
	$(SRC_DIR)/entry.o \
	installer.o \
	$(PLATFORM_DIR)/stubs.o \
	version.o \
	bmp.o \
	chdk-gui_draw.o \
	font-dyn.o \
	stdio.o \
	property.o \
	propvalues.o \
	dialog_test.o \
	bootflags.o \
	vram.o \

#include generic rules and definitions
include  $(TOP_DIR)/Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF0C0000
RESTARTSTART		= 0x0009E300

updater: reboot-ins.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-march=armv5te \
		-e _start \
		-Ttext 0x800000 \
		$^ \
	); \


# Notice I rename these to avoid problems with main Makefile
installer: 
	make clean
	make magiclantern.bin
	make updater.bin
	python ../../../dumper/build_fir.py -r ../../../dumper/5D300113.FIR updater.bin $(UPDATE_NAME)

clean:
	rm -f *.fir *.o autoexec* magiclantern* updater* ml.bin

install:
	cp $(UPDATE_NAME) /media/EOS_DIGITAL/ && umount /media/EOS_DIGITAL 
