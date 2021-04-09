# 5DS R 1.1.2

MODEL=5DSR

include ../Makefile.platform

UPDATE_NAME_FIR=LOG_5DSR.FIR

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
	python ../../../dumper/build_fir7.py -r -s autoexec-fir.bin -m dummy-master-fir.bin ../../../dumper/$(CANON_NAME_FIR) $(UPDATE_NAME_FIR)
