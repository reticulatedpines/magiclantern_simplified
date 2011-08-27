#Makefile for 550D firmware 109

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=109
MODEL=550D
UPDATE_NAME=550d_109.fir
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
	font-large.o \
	font-med.o \
	font-small.o \
	property.o \
	propvalues.o \

#include generic rules and definitions

include  $(TOP_DIR)/Makefile.inc

CFLAGS += -DCONFIG_STATIC_FONTS=1

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF010000
RESTARTSTART		= 0x0008B000

#not used but...
FIRMWARE_ID_550D	= 0x80000270

updater: reboot-ins.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-march=armv5te \
		-e _start \
		-Ttext 0x800000 \
		$^ \
	); \

all: magiclantern.bin updater.bin
	python ../../../dumper/build_fir.py -r ../../../dumper/e8kr7109.fir updater.bin ml-550d-109.fir

clean:
	rm -f *.fir *.o autoexec* magiclantern* updater* ml.bin

install:
	cp ml-550d-109.fir /media/EOS_DIGITAL/ && umount /media/EOS_DIGITAL 
