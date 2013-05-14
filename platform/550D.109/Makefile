#Makefile for 550D firmware 109

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=109
MODEL=550D
UPDATE_NAME=550d_109.fir
ML_VERSION=unified1 

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

CONFIG_RELOC = y

ML_EXTRA_OBJS = \
	misc.o \
	lcdsensor.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR     = 0xFF010000

# for injecting ML into malloc pool (Trammell's approach)
#RESTARTSTART		= 0x0008B000


# For injecting ML into AllocateMemory (system memory) pool - which is much bigger; 
# The malloc pool is very small on 550D and creates problems when Canon firmware tries to malloc.
# They don't always check for successful mallocs, and this causes weird bugs - camera acting like drunk :)
RESTARTSTART    = 0xC80100

AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

#not used but...
FIRMWARE_ID_550D	= 0x80000270

all: autoexec.bin
