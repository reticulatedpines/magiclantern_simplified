#Makefile for 5Dc firmware 111

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=111
MODEL=5DC
UPDATE_NAME=5DC_111.fir
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

CONFIG_RELOC = y
CONFIG_STATIC_FONTS=y

ML_ZEBRA_OBJ = zebra-5dc.o

ML_EXCLUDE_OBJS = \
	boot-hack.o \
	bitrate.o \
	fps-engio.o \
	audio-ak.o \
	entry.o \
	$(SRC_DIR)/entry.o \
	movtweaks.o \
	exmem.o \
	liveview.o \
	bootflags.o \
	lv-img-engio.o \
	hdr.o \

ML_EXTRA_OBJS = \
		local_entry.o \
		init.o \
		dummy.o \
		dm-spy.o \
		tp-spy.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

CFLAGS += -DCONFIG_STATIC_FONTS=1 -Wno-unused-variable

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR     = 0xFF810000
RESTARTSTART    = 0x00980000
AUTOEXEC_BASE   = 0x10800000
FIR_BASE        = 0x10800120

all: autoexec.bin
