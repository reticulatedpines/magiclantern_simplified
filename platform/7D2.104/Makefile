#Makefile for 7D2

MODEL=7D2
PLATFORM_ARCH = armv7-r

include ../Makefile.platform

UPDATE_NAME_FIR=DUMP-7D2.FIR

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
	python ../../../dumper/build_fir7.py -r -s autoexec-fir.bin -m dummy-master-fir.bin ../../../dumper/7D200104.FIR $(UPDATE_NAME_FIR)
