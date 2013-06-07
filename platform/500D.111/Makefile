#Makefile for 500D firmware 111

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=111
MODEL=500D
UPDATE_NAME=500d_111.fir
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

ML_PICSTYLE_AUTO = n

ML_EXTRA_OBJS = \
	misc.o \
	ph_info_disp.o \
	flexinfo.o \
	lcdsensor.o \
	raw.o \
	chdk-dng.o \
	edmac-memcpy.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR     = 0xFF010000
RESTARTSTART    = 0x0004D000
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

all: autoexec.bin
