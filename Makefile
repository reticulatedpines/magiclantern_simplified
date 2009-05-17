ARM_PATH=/usr/local/arm/oe/bin
CC=$(ARM_PATH)/arm-linux-gcc
LD=$(ARM_PATH)/arm-linux-gcc
HOST_CC=gcc
HOST_CFLAGS=-g -O3 -W -Wall

OBJCOPY=$(ARM_PATH)/arm-linux-objcopy

# 5D memory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF810000
RESTARTSTART		= 0x00048000
RELOCADDR		= 0x00050000


all: \
	5d2_dumper.fir \
	5d2_reboot.fir \



FLAGS=\
	-Wp,-MMD,.$@.d \
	-Wp,-MT,$@ \
	-nostdlib \
	-fomit-frame-pointer \
	-fno-strict-aliasing \
	-DRELOCADDR=$(RELOCADDR) \
	-DRESTARTSTART=$(RESTARTSTART) \
	-DROMBASEADDR=$(ROMBASEADDR) \

NOT_USED_FLAGS=\
	-march=armv5te \
	-mthumb \
	-mthumb-interwork \

CFLAGS=\
	$(FLAGS) \
	-Os \
	-Wall \
	-W \


AFLAGS=\
	$(FLAGS) \


%.s: %.c
	$(CC) $(CFLAGS) -S -o $@ $<
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%: %.c
	$(CC) $(CFLAGS) -o $@ $<
%.o: %.S
	$(CC) $(AFLAGS) -c -o $@ $<
%.bin: %
	$(OBJCOPY) -O binary $< $@

dumper: dumper_entry.o dumper.o
	$(LD) \
		-o $@ \
		-nostdlib \
		-mthumb-interwork \
		-march=armv5te \
		-e _start \
		$^

reboot.o: reboot.c 5d-hack.bin
5d-hack.bin: 5d-hack

5d-hack: 5d-hack.o stubs-5d2.107.o
	$(LD) \
		-o $@ \
		-nostdlib \
		-mthumb-interwork \
		-march=armv5te \
		-Ttext=$(RESTARTSTART) \
		$^



reboot: reboot.o
	$(LD) \
		-o $@ \
		-nostdlib \
		-mthumb-interwork \
		-march=armv5te \
		-e _start \
		-Ttext=0x800000 \
		$^

%-stubs.S: %.map
	perl -ne > $@ < $< '\
		BEGIN { print "#define SYM(a,n) n=a; .global n;\n" } \
		s/[\r\n]//g; \
		s/^\s*0001:([0-9A-Fa-f]+)\s+([^\s]+)$$/SYM(0x\1,\2)\n/ \
			and print; \
	'


%.dis: %.bin
	$(ARM_PATH)/arm-linux-objdump \
		-b binary \
		-m arm \
		-D \
		$< \
	> $@

BASE=0xFF800000
#BASE=0
#BASE=0xFF000000

ROM0.elf: ROM0.bin 5D21070a.map
	./remake-elf \
		--base $(BASE) \
		--cc $(CC) \
		--relative \
		-o $@ \
		$^


strings: ROM0.bin
	strings -t x $^

ROM0.bin: FORCE
FORCE:


#
# Fetch the firmware archive from the Canon website
# and unpack it to generate the pristine firmware image.
#
eos5d2107.exe:
	wget http://web.canon.jp/imaging/eosd/firm-e/eos5dmk2/data/eos5d2107.exe

5d200107.fir: eos5d2107.exe
	-unzip -o $< $@
	touch $@

# Extract the flasher binary file from the firmware image
# and generate an ELF from it for analysis.
%.1.flasher.bin: %.fir dissect_fw
	./dissect_fw $< . $(basename $<)

flasher.elf: 5d200107.1.flasher.bin flasher.map
	./remake-elf \
		--cc $(CC) \
		--base 0x800120 \
		-o $@ \
		$^

dumper.elf: 5d2_dump.fir flasher.map
	./remake-elf \
		--cc $(CC) \
		--base 0x800000 \
		-o $@ \
		$^

#
# Generate a new firmware image suitable for dumping the ROM images
#
5d2_dumper.fir: dumper.bin 5d200107.1.flasher.bin
	./assemble_fw \
		--output $@ \
		--user $< \
		--offset 0x5ab8 \

5d2_reboot.fir: reboot.bin 5d200107.1.flasher.bin
	./assemble_fw \
		--output $@ \
		--user $< \
		--offset 0x120 \

dummy_data_head.bin:
	perl -e 'print chr(0) x 24' > $@

#ROM0.bin: 5d200107.fir

# Use the dump_toolkit files
# deprectated; use the dumper.c program instead
5d2_dump.fir:
	-rm $@
	cat \
		5d200107.0.header.bin \
		5d200107.1.flasher.bin \
		dump_toolkit/repack/dummy_data_head.bin \
	> $@
	./patch-bin $@ < dump_toolkit/diffs/5d2_dump.diff


# Firmware manipulation tools
dissect_fw: dissect_fw.c
	$(HOST_CC) $(HOST_CFLAGS) -o $@ $<


clean:
	-rm -f *.o *.a

-include .*.o.d
