#Makefile for 600D firmware 102

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=102
MODEL=600D
UPDATE_NAME=600d_102.fir

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

# compile position independent (experimental)
CONFIG_PIC=n

CONFIG_RELOC=y

ML_AUDIO_OBJ = audio-lapis.o

ML_EXTRA_OBJS = \
	misc.o \
	ph_info_disp.o \
	flexinfo.o \
	raw.o \
	chdk-dng.o \
	edmac-memcpy.o \


#include generic rules and definitions

include  $(TOP_DIR)/Makefile.inc


# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR     = 0xFF010000
RESTARTSTART    = 0x00C80100
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

#not used but...
FIRMWARE_ID_600D 	= 0x80000286

all: autoexec.bin
