#Makefile for 60D firmware 109

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=101
MODEL=600D
UPDATE_NAME=600d_101.fir

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

#include generic rules and definitions

include  $(TOP_DIR)/Makefile.inc


# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF010000
RESTARTSTART		= 0x00082000

#not used but...
FIRMWARE_ID_600D 	= 0x80000286


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
	python ../../../dumper/build_fir.py -r ../../../dumper/CCF11101.FIR updater.bin ml-600d-101.fir

clean:
	rm -f *.fir *.o autoexec* magiclantern* updater* ml.bin

install:
	cp ml-600d-101.fir /media/EOS_DIGITAL/ && umount /media/EOS_DIGITAL 
