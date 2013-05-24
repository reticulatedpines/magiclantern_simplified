#Makefile for 40D firmware 111

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=111
MODEL=40D
UPDATE_NAME=40D_111.fir
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

ML_ZEBRA_OBJ = zebra-5dc.o

ML_EXTRA_OBJS = \
	init.o \
	dummy.o \
	dm-spy.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#

ROMBASEADDR     = 0xFF810000
RESTARTSTART    = 0x00B80000
AUTOEXEC_BASE   = 0x10800000
FIR_BASE        = 0x10800120

all: autoexec.bin

ptpinstall: autoexec.bin
	gvfs-mount -s gphoto2
	ptpcam -i
	sleep 2
	cat $(TOP_DIR)/platform/40D.111/ptpcam.txt | ptpcam --chdk
	diff autoexec.bin autoexec.test
