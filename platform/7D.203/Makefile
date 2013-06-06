#Makefile for 7D firmware 203

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=203
MODEL=7D
UPDATE_NAME=7D_203.fir
ML_VERSION=unified1 

# compile position independent (experimental)
CONFIG_PIC=n

#used in CFLAGS
PLATFORM_INC=.

PLATFORM_DIR=$(PLATFORM_PATH)/$(MODEL).$(FW_VERSION)

CONFIG_GDB=y
ML_PICSTYLE_AUTO=n

ML_EXTRA_OBJS = \
	ml_rpc.o \
	video_hacks.o \
	misc.o \
	ptpbuf.o \
	ph_info_disp.o \
	flexinfo.o \
	afma.o \

#include generic rules and definitions
#TOP_DIR defined in upper Makefile
include ../../Makefile.inc

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR     = 0xF8010000
RESTARTSTART    = 0x1F800100
AUTOEXEC_BASE   = 0x40800000
FIR_BASE        = 0x40800120

#CFLAGS += -DCONFIG_7D_MINIMAL=1

all: autoexec.bin

FIR-MASTER: autoexec-fir-master
FIR-SLAVE: autoexec-fir-slave

# autoexec-fir-master: CFLAGS += -DCONFIG_7D_FIR_MASTER
# autoexec-fir-slave: CFLAGS += -DCONFIG_7D_FIR_SLAVE

# autoexec-fir-master: clean autoexec-fir
	# $(call build,OBJCOPY,$(OBJCOPY) -O binary autoexec-fir $@.bin)
	# $(STAT_CMD) $@.bin;
	# $(READELF) -l autoexec-fir | grep -C 2 MemSiz
	# cp location.map ../../location.master

    
# autoexec-fir-slave: clean autoexec-fir
	# $(call build,OBJCOPY,$(OBJCOPY) -O binary autoexec-fir $@.bin)
	# $(STAT_CMD) $@.bin;
	# $(READELF) -l autoexec-fir | grep -C 2 MemSiz
