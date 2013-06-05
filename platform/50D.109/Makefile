#Makefile for 50D firmware 109

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=109
MODEL=50D
UPDATE_NAME=50d_109.fir
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

ML_AUDIO_OBJ=n

ML_EXTRA_OBJS = \
	misc.o \
	afma.o \
	raw.o \
	chdk-dng.o \
	edmac-memcpy.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR     = 0xFF810000
RESTARTSTART    = 0x0004b000
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

all: autoexec.bin
