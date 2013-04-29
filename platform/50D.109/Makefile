#Makefile for 550D firmware 109

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=109
MODEL=50D
UPDATE_NAME=50d_109.fir
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
	boot-hack.o \
	stubs.o \
	version.o \
	exmem.o \
	bmp.o \
	font-dyn.o \
	config.o \
	menu.o \
	debug.o \
	stdio.o \
	property.o \
	propvalues.o \
	gui.o \
	gui-common.o \
	misc.o \
	tweaks.o \
	movtweaks.o \
	bitrate.o \
	lens.o \
	picstyle.o \
	cfn.o \
	zebra.o \
	shoot.o \
	chdk-gui_draw.o \
	menuhelp.o \
	menuindex.o \
	af_patterns.o \
	focus.o \
	notify_box.o \
	bootflags.o \
	dialog_test.o \
	vram.o \
	morse.o \
	aj_port.o \
	fps-engio.o \
	hdr.o \
	lv-img-engio.o \
	state-object.o \
	tasks.o \
	tskmon.o \
	beep.o \
	vsync-lite.o \
	afma.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR     = 0xFF810000
RESTARTSTART    = 0x0004b000
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

all: autoexec.bin
