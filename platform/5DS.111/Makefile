#Makefile for 5DS

MODEL=5DS
PLATFORM_ARCH = armv7-r
ROMBASEADDR=0xFE0A0000

include ../Makefile.platform

UPDATE_NAME_FIR=DUMP-5DS.FIR

dummy-master-fir: dummy-master.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-e _start \
		-Ttext 0x40800120 \
		$^ \
	);

clean:: 
	$(call rm_files, $(UPDATE_NAME_FIR) *.bin dummy-master-fir)

build_fir: autoexec-fir.bin dummy-master-fir.bin
	python ../../../dumper/build_fir7.py -r -s autoexec-fir.bin -m dummy-master-fir.bin ../../../dumper/5DS00111.FIR $(UPDATE_NAME_FIR)
