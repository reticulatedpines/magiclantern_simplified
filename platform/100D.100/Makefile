#Makefile for 100D firmware 100

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=100
MODEL=100D
ML_VERSION=unified1 


# compile position independent (experimental)
CONFIG_PIC=n

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

ML_BITRATE_OBJ = bitrate-5d3.o
ML_AF_PATTERNS_OBJ = n
ML_BOOTFLAGS_OBJ = n

ML_EXTRA_OBJS = \
	video_hacks.o \
	fio-5d3.o \
	misc.o \
	ph_info_disp.o \
	flexinfo.o \
	afma.o \
	raw.o \
	chdk-dng.o \
	edmac-memcpy.o \
	asm.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF0C0000
RESTARTSTART	= 0x000C5400
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

CFLAGS += -DCONFIG_5D3_MINIMAL=1 -DCONFIG_NO_SNAP_SIM=1
