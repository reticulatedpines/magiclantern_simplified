#Makefile for 550D firmware 109

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=111
MODEL=5D
UPDATE_NAME=5D_111.fir
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

ML_OBJS-y = \
	magiclantern.lds \
	$(SRC_DIR)/entry.o \
	stubs.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF810000

# needs changed
RESTARTSTART    = 0x1234

updater: reboot-5d.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-march=armv5te \
		-e _start \
		-Ttext 0x800000 \
		$^ \
	); \

all:
	make clean
	make updater.bin
	python ../../../dumper/build_fir.py -r ../../../dumper/5d000111.fir updater.bin ml-5D-111.fir
