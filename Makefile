ARM_PATH=/opt/local/bin
CC=$(ARM_PATH)/arm-elf-gcc-4.3.2
OBJCOPY=$(ARM_PATH)/arm-elf-objcopy
LD=$(CC)
HOST_CC=gcc
HOST_CFLAGS=-g -O3 -W -Wall


# 5D memory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF810000
RESTARTSTART		= 0x00048000
RELOCADDR		= 0x00050000


all: \
	5d2_dumper.fir \
	magiclantern.fir \



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
	-O3 \
	-Wall \
	-W \


AFLAGS=\
	$(FLAGS) \


%.s: %.c
	$(CC) $(CFLAGS) -S -o $@ $<
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%.i: %.c
	$(CC) $(CFLAGS) -E -c -o $@ $<
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

dumper_entry.o: flasher-stubs.S

reboot.o: reboot.c magiclantern.bin
5d-hack.bin: 5d-hack

magiclantern.lds: magiclantern.lds.S
	$(CPP) $(CFLAGS) $< | grep -v '^#' > $@

# magiclantern.lds script MUST be first
# entry.o MUST be second
magiclantern: \
	magiclantern.lds \
	entry.o \
	5d-hack.o \
	gui.o \
	audio.o \
	hotplug.o \
	bmp.o \
	font.o \
	stubs-5d2.110.o \

	$(LD) \
		-o $@ \
		-N \
		-nostdlib \
		-mthumb-interwork \
		-march=armv5te \
		-T \
		$^


font.c: font.in mkfont
	./mkfont < $< > $@

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

1.1.0/ROM0.elf: 1.1.0/ROM0.bin 1.1.0/ROM0.map
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

magiclantern.fir: reboot.bin 5d200107.1.flasher.bin
	./assemble_fw \
		--output $@ \
		--user $< \
		--offset 0x120 \
		--zero \

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
