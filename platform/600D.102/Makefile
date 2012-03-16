#Makefile for 60D firmware 109

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=102
MODEL=600D
UPDATE_NAME=600d_102.fir

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
	font-dyn.o \
	config.o \
	menu.o \
	debug.o \
	stdio.o \
	audio.o \
	bitrate.o \
	tweaks.o \
	lens.o \
	property.o \
	propvalues.o \
	gui.o \
	gui-common.o \
	misc.o \
	lensfocus.o \
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
	fps.o \
	hdr.o \
	lv-img-engio.o \
	state-object.o \

#include generic rules and definitions

include  $(TOP_DIR)/Makefile.inc


# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF010000
RESTARTSTART		= 0x00082000

#not used but...
FIRMWARE_ID_600D 	= 0x80000286

all: autoexec.bin
