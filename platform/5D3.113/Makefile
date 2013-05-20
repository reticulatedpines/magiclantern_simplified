#Makefile for 5D Mark III firmware 113

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=113
MODEL=5D3
ML_VERSION=unified1 


# compile position independent (experimental)
CONFIG_PIC=n

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

ML_BITRATE_OBJ = bitrate-5d3.o

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
RESTARTSTART	= 0x00069500
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

CFLAGS += -DCONFIG_5D3_MINIMAL=1 -DCONFIG_NO_SNAP_SIM=1

x: 
	make clean
	make autoexec-fir.bin
	python ../../../dumper/build_fir.py -r ../../../dumper/5D300113.FIR autoexec-fir.bin ml-5D3-113.fir
