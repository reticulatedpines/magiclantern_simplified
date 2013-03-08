#Makefile for 5Dc firmware 111

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=111
MODEL=5DC
UPDATE_NAME=5DC_111.fir
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

# magiclantern.lds script MUST be first
# entry.o MUST be second
# menu.o and debug.o must come before the modules
ML_OBJS-y = \
	magiclantern.lds \
	entry.o \
	init.o \
	stubs.o \
	dummy.o \
	version.o \
	bmp.o \
	font-large.o \
	font-med.o \
	font-small.o \
	config.o \
	menu.o \
	debug.o \
	stdio.o \
	property.o \
	propvalues.o \
	gui.o \
	gui-common.o \
	tweaks.o \
	lens.o \
	picstyle.o \
	cfn.o \
	zebra-5dc.o \
	shoot.o \
	chdk-gui_draw.o \
	menuhelp.o \
	menuindex.o \
	af_patterns.o \
	focus.o \
	notify_box.o \
	dialog_test.o \
	vram.o \
	aj_port.o \
	state-object.o \
	tasks.o \
	beep.o \
	dm-spy.o \
	tp-spy.o \
	reloc.o \


#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

CFLAGS += -DCONFIG_STATIC_FONTS=1

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR     = 0xFF810000
RESTARTSTART    = 0x00980000
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

all: autoexec.bin
