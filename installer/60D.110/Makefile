#Makefile for 60D firmware 110

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=110
MODEL=60D
UPDATE_NAME=60d00110.fir

ML_FIR=ml-60D-110.fir

#used in CFLAGS
PLATFORM_INC=.

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
RESTARTSTART		= 0x0005f000

#not used but...
FIRMWARE_ID		= 0x80000286

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
	python ../../../dumper/build_fir.py -r ../../../dumper/60D00110.FIR updater.bin ml-60d-110.fir

clean:
	rm -f *.fir *.o autoexec* magiclantern* updater* ml.bin

install:
	cp ml-60d-110.fir /media/EOS_DIGITAL/ && umount /media/EOS_DIGITAL 
