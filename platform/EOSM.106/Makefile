#Makefile for EOS M firmware 106

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=106
MODEL=EOSM
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

ML_EXTRA_OBJS = \
	misc.o \
	dm-spy.o \
	video_hacks.o \
	raw.o \
	chdk-dng.o \
	tp-spy.o \
	reloc.o \
	edmac-memcpy.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF0C0000
RESTARTSTART	= 0x0009E300
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

all: autoexec.bin
