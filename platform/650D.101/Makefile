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
	aj_port.o \
	lv-img-engio.o \
	fps-engio.o \
	hdr.o \
	lv-img-engio.o \
	state-object.o \
	tasks.o \
	tskmon.o \
	beep.o \
	audio-ak.o \
	vsync-lite.o \
	ph_info_disp.o \
	flexinfo.o \
	dm-spy.o \

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
