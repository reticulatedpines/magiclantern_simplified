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
	audio-ak.o \
	bitrate.o \
	tweaks.o \
	lens.o \
	property.o \
	propvalues.o \
	gui.o \
	gui-common.o \
	misc.o \
	picstyle.o \
	cfn.o \
	zebra.o \
	shoot.o \
	chdk-gui_draw.o \
	movtweaks.o \
	my_memset.o \
	menuhelp.o \
	menuindex.o \
	af_patterns.o \
	focus.o \
	notify_box.o \
	bootflags.o \
	dialog_test.o \
	vram.o \
	morse.o \
	liveview.o \
	reloc.o \
	aj_port.o \
	fps-engio.o \
	hdr.o \
	lv-img-engio.o \
	state-object.o \
	tasks.o \
	tskmon.o \
	beep.o \
	vsync-lite.o \

#include generic rules and definitions

include  $(TOP_DIR)/Makefile.inc


# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR     = 0xFF010000
RESTARTSTART    = 0x0005f000
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

#not used but...
FIRMWARE_ID		= 0x80000286

all: autoexec.bin

