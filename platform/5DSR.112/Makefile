MODEL = 5DSR
FW_VERSION = 112

TOP_DIR = ../..

AUTOEXEC_BASE = 0x800000
FIR_BASE = 0x800120

#UPDATE_NAME_FIR = LOG_5DSR.FIR

#dummy-master-fir: dummy-master.o
#	$(call build,LD,$(LD) \
#		-o $@ \
#		-nostdlib \
#		-e _start \
#		-Ttext 0x40800120 \
#		$^ \
#	);
#
#clean:: 
#	$(call rm_files, $(UPDATE_NAME_FIR) *.bin dummy-master-fir)
#
#build_fir: autoexec-fir.bin dummy-master-fir.bin
#	python ../../../dumper/build_fir7.py -r -s autoexec-fir.bin -m dummy-master-fir.bin ../../../dumper/$(CANON_FIR) $(UPDATE_NAME_FIR)

#CANON_FIR = 5DSR0112.FIR
FIRMWARE_ID = 0x80000401
MAIN_FIRMWARE_ADDR = 0xFE0A0000

# user_mem_start from cstart struct. Align to 0x10 up
RESTARTSTART = 0x1cc400

# Cortex R4
CFLAGS_PLATFORM += -mlong-calls
PLATFORM_ARCH = armv7-r

ML_BOOT_OBJ = boot-d678.o

include ../Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/cam_complete
