#Makefile for 650D firmware 101

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=101
MODEL=650D
UPDATE_NAME=$(MODEL)-$(FW_VERSION).fir 

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
	font-large.o \
	font-med.o \
	font-small.o \
	property.o \
	propvalues.o \
	dialog_test.o \
	bootflags.o \
	vram.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

CFLAGS += -DCONFIG_STATIC_FONTS=1

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF0C0000
RESTARTSTART		= 0x0007e100

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
installer: magiclantern.bin updater.bin
	make clean
	make magiclantern.bin
	make updater.bin
	python ../../../../550_alex/dumper/build_fir7.py -r -i 0x301 -s updater.bin ../../../../550_alex/dumper/550d_109.fir 650D_101.fir
clean:
	rm -f *.fir *.o autoexec* magiclantern* updater* ml.bin

install:
	cp $(UPDATE_NAME) /media/EOS_DIGITAL/ && umount /media/EOS_DIGITAL 
