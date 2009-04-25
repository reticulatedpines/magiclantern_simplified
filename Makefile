ARM_PATH=/usr/local/arm/oe/bin
CC=$(ARM_PATH)/arm-linux-gcc
LD=$(ARM_PATH)/arm-linux-gcc
HOST_CC=gcc
HOST_CFLAGS=-g -O3 -W -Wall

OBJCOPY=$(ARM_PATH)/arm-linux-objcopy


FLAGS=\
	-Wp,-MMD,.$@.d \
	-Wp,-MT,$@ \
	-nostdlib \
	-fomit-frame-pointer \
	-fno-strict-aliasing
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
		--entry 0x805ab8 \
		-nostdlib \
		-mthumb-interwork \
		-march=armv5te \
		-e _start \
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

dumper.elf: 5d200107_dump.fir flasher.map
	./remake-elf \
		--cc $(CC) \
		--base 0x800000 \
		-o $@ \
		$^

#
# Generate a new firmware image suitable for dumping the ROM images
#
5d200107_dump.fir: dumper.bin 5d200107.1.flasher.bin dummy_data_head.bin
	./assemble_fw \
		--output $@ \
		--user $< \

dummy_data_head.bin:
	perl -e 'print chr(0) x 24' > $@

#ROM0.bin: 5d200107.fir

# Use the dump_toolkit files
5d2_dump.fir:
	-rm $@
	cat \
		5d200107.fir.0.header.bin \
		5d200107.fir.1.flasher.bin \
		dump_toolkit/repack/dummy_data_head.bin \
	> $@
	./patch-bin $@ < dump_toolkit/diffs/5d2_dump.diff


# Firmware manipulation tools
dissect_fw: dissect_fw.c
	$(HOST_CC) $(HOST_CFLAGS) -o $@ $<


clean:
	-rm -f *.o *.a *.bin

-include .*.o.d
