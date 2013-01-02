#Makefile for an autoexec.bin able to run on all cameras

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top
 
empty:=
space:= $(empty) $(empty)
MODEL=$(subst $(space),.,$(SUPPORTED_MODELS))

include  $(TOP_DIR)/Makefile.inc

autoexec-all: reboot-all.o
	$(call build,LD,$(LD) \
		-o autoexec-all \
		-nostdlib \
		-march=armv5te \
		-e _start \
		-Ttext 0x800000 \
		$^ \
	); \

ROMBASEADDR		= 0xFF010000

x: autoexec-all autoexec-all.bin
	cp autoexec-all.bin autoexec.bin
	$(STAT_CMD) autoexec.bin; \

