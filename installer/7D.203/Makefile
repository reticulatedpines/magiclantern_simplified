MODEL=7D
include ../Makefile.installer

RESTARTSTART = 0x88000
ROMBASEADDR  = 0xFF010000

dummy-master: dummy-master.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-e _start \
		-Ttext 0x800000 \
		$^ \
	);

build_fir: dummy-master.bin
	dd if=autoexec.bin of=autoexec.fir bs=288 skip=1 >/dev/null 2>&1
	dd if=dummy-master.bin of=dummy-master.fir bs=288 skip=1 >/dev/null 2>&1
	python ../../../dumper/build_fir7.py -r -s autoexec.fir -m dummy-master.fir ../../../dumper/7D000203.FIR 7D_203.fir
