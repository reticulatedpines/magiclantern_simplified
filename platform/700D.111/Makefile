#Makefile for EOS 700D firmware 111

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=111
MODEL=700D
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

# compile position independent (experimental)
CONFIG_PIC=n

ML_EXTRA_OBJS = \
	misc.o \
#	ph_info_disp.o \
#	flexinfo.o \
#	raw.o \
#	chdk-dng.o \
#	edmac-memcpy.o \

#include generic rules and definitions
include  $(TOP_DIR)/Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
# BUT IT IS CURRENTLY WRONG!!
#
ROMBASEADDR	    = 0xFF0C0000
RESTARTSTART	= 0x0007EC00
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

all: autoexec.bin
