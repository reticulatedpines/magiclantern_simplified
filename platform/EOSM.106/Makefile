#Makefile for EOS M firmware 100

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=106
MODEL=EOSM
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
	exmem.o \
	bmp.o \
	font-dyn.o \
	config.o \
	menu.o \
	debug.o \
	stdio.o \
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
	focus.o \
	notify_box.o \
	dialog_test.o \
	vram.o \
	aj_port.o \
	lv-img-engio.o \
	fps-engio.o \
	tasks.o \
	tskmon.o \
	state-object.o \
	hdr.o \
	lv-img-engio.o \
	beep.o \
	audio-ak.o \
	vsync-lite.o \

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
	make autoexec.bin
	python ../../../dumper/build_fir.py -r ../../../dumper/5D300113.FIR autoexec.bin ml-5D3-113.fir
