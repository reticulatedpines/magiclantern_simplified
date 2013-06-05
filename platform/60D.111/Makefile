#Makefile for 60D firmware 111

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=111
MODEL=60D
UPDATE_NAME=60d00111.fir

ML_FIR=ml-60D-111.fir

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

CONFIG_RELOC = y
ML_PICSTYLE_AUTO = n

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
RESTARTSTART    = 0x1FE00000
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

#not used but...
FIRMWARE_ID		= 0x80000286

all: autoexec.bin

