#Makefile for 550D firmware 109

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=108
MODEL=50D
UPDATE_NAME=50d_108.fir
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

# magiclantern.lds script MUST be first
# entry.o MUST be second
# menu.o and debug.o must come before the modules
ML_OBJS-y = \
	magiclantern.lds \
	$(SRC_DIR)/entry.o \
	boot-hack.o \
	stubs.o \
	version.o \
	bmp.o \
	font-large.o \
	font-med.o \
	font-small.o \
	config.o \
	menu.o \
	debug.o \
	stdio.o \
	audio.o \
	bitrate.o \
	property.o \
	propvalues.o \
	gui.o \
	misc.o \
	tweaks.o \
	lens.o \
	lensfocus.o \
	cfn.o \
	lcdsensor.o \
	zebra.o \
	shoot.o \
	chdk-gui_draw.o \
	movtweaks.o \
	my_memset.o \
	menuhelp.o \
	menuindex.o \
	focus.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF810000
RESTARTSTART		= 0x0004b000

all: autoexec.bin
