#Makefile for EOS 650D firmware 101

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=101
MODEL=650D
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

# compile position independent (experimental)
CONFIG_PIC=n

ML_EXTRA_OBJS = \
	misc.o \
	ph_info_disp.o \
	flexinfo.o \
	dm-spy.o \
	raw.o \
	chdk-dng.o \
	edmac-memcpy.o \

#include generic rules and definitions
include  $(TOP_DIR)/Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF0C0000
RESTARTSTART	= 0x0007E100
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

all: autoexec.bin

x: 
	make clean
	make autoexec-fir.bin
	python ../../../dumper/build_fir.py -r ../../../dumper/5D300113.FIR autoexec-fir.bin ml-650D-101.fir
