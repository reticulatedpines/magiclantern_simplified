#Makefile for 6D 112 firmware

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=112
MODEL=6D
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

ML_BITRATE_OBJ = bitrate-6d.o

ML_EXTRA_OBJS = \
	misc.o \
	dm-spy.o \
	ph_info_disp.o \
	flexinfo.o \
	afma.o \

#	evf-hacks.o \
#	video_hacks.o \
#	flexinfo.o \

#	af_patterns.o \ // dangerous

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF0C0000
RESTARTSTART	= 0x00CBC100
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

all: autoexec.bin
