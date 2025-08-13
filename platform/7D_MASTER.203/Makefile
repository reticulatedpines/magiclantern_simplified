MODEL = 7D_MASTER
# This build is broken.  It's probably not hard to fix,
# but I don't think we run ML on the Master CPU, the Slave
# is the primary core for our purposes.
# Not sure, docs are quite bad.

#CANON_FIR = 7D000203.FIR
# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
MAIN_FIRMWARE_ADDR = 0xFF810000
PLATFORM_ARCH = armv5te
RESTARTSTART = 0xC80100
ML_BOOT_OBJ = boot-d45-ch.o

# magiclantern.lds script MUST be first
# entry.o MUST be second
# menu.o and debug.o must come before the modules
ML_OBJS-y = \
	magiclantern.lds \
	entry.o \
	master_boot.o \
	stubs.o \
	gdb.o \
	ptpbuf.o \
	ml_rpc.o \
	state-object.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
