#Makefile for an autoexec.bin able to run on all cameras

x: prepare-all autoexec-all autoexec-all.bin
	cp autoexec-all.bin autoexec.bin
	$(STAT_CMD) autoexec.bin; \

prepare-all:
	$(MAKE) -C .. all-supported-models

autoexec-all: reboot-all.o
	$(call build,LD,$(LD) \
		-o autoexec-all \
		-nostdlib \
		-march=armv5te \
		-e _start \
		-Ttext 0x800000 \
		$^ \
	); \

clean::
	$(call rm_file, autoexec-all autoexec-all.bin)

empty:=
space:= $(empty) $(empty)
MODEL=$(subst $(space),.,$(SUPPORTED_MODELS))

ML_MODEL_DIR = all

include  ../Makefile.platform
