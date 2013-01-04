#Makefile for 7D firmware 203 master digic

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=203
MODEL=7D_MASTER
UPDATE_NAME=7D_203.fir
ML_VERSION=unified1 

CONFIG_NO_PTP=y

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

# magiclantern.lds script MUST be first
# entry.o MUST be second
# menu.o and debug.o must come before the modules
ML_OBJS-y = \
	magiclantern.lds \
	$(SRC_DIR)/entry.o \
	master_boot.o \
	stubs.o \
	gdb.o \
	ptpbuf.o \
	ml_rpc.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

#FLAGS += -fPIC -DPOSITION_INDEPENDENT

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR     = 0xFF810000
RESTARTSTART    = 0xC80100
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

all: autoexec.bin
