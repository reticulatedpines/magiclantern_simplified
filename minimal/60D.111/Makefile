#Makefile for 60D firmware 111

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=111
MODEL=60D

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

# magiclantern.lds script MUST be first
# entry.o MUST be second
# menu.o and debug.o must come before the modules
ML_OBJS-y = \
	magiclantern.lds \
	entry.o \
	minimal.o \
	$(PLATFORM_DIR)/stubs.o \

#include generic rules and definitions

include  $(TOP_DIR)/Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF010000
RESTARTSTART		= 0x0005f000
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

#not used but...
FIRMWARE_ID		= 0x80000286

all: autoexec.bin
