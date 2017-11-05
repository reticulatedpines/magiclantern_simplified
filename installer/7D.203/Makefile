MODEL=7D
include ../Makefile.installer

dummy-master-fir: dummy-master.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-e _start \
		-Ttext 0x800120 \
		$^ \
	);

clean:: 
	$(call rm_files, *.fir *.bin dummy-master-fir)

build_fir: autoexec-fir.bin dummy-master-fir.bin
	python ../../../dumper/build_fir7.py -r -s autoexec-fir.bin -m dummy-master-fir.bin ../../../dumper/7D000203.FIR $(UPDATE_NAME_FIR)
