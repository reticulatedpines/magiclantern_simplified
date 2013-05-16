#Makefile for 1100D firmware 105

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=105
MODEL=1100D
UPDATE_NAME=1100d_105_fake.fir

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

# compile position independent (experimental)
CONFIG_PIC=n

CONFIG_RELOC=y

ML_FONT_OBJS = \
	font-small2.o \
	font-med2.o \
	font-large.o \
	font-dyn.o \

ML_EXTRA_OBJS = \
	misc.o \
	ph_info_disp.o \
	flexinfo.o \

#include generic rules and definitions

include  $(TOP_DIR)/Makefile.inc

CFLAGS += -D CONFIG_STATIC_FONTS=1 

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF010000
RESTARTSTART    = 0x00C80100
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

#not used but...
FIRMWARE_ID_1100D	= 0x80000288

all: autoexec.bin
