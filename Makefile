ARM_PATH=/usr/local/arm/oe/bin
CC=$(ARM_PATH)/arm-linux-gcc
LD=$(ARM_PATH)/arm-linux-ld

OBJCOPY=$(ARM_PATH)/arm-linux-objcopy


CFLAGS=\
	-g \
	-O3 \
	-Wall \
	-W \
	-fpic \
	-static \
	-nostdlib \
	-fomit-frame-pointer \

%.s: %.c
	$(CC) $(CFLAGS) -S -o $@ $<
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%: %.c
	$(CC) $(CFLAGS) -o $@ $<
%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $<
%.bin: %
	$(OBJCOPY) -O binary $< $@

dumper: dumper_entry.o dumper.o
	$(LD) -o $@ $^



%.dis: %.bin
	$(ARM_PATH)/arm-linux-objdump \
		-b binary \
		-m arm \
		-D \
		$< \
	> $@

BASE=0xFF800000
#BASE=0

ROM0.elf: ROM0.bin 5D21070a.map
	./remake-elf \
		--base $(BASE) \
		--cc $(ARM_CC) \
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
		--cc $(ARM_CC) \
		--base 0x800120 \
		-o $@ \
		$^

#
# Generate a new firmware image suitable for dumping the ROM images
#
5d200107_dump.fir: 5d200107.1.flasher.bin dummy_data_head.bin
	cat > $@ \
		5d200107.0.header.bin \
		5d200107.1.flasher.bin \
		dummy_data_head.bin \

dummy_data_head.bin:
	perl -e 'print chr(0) x 24' > $@

#ROM0.bin: 5d200107.fir


# Firmware manipulation tools
dissect_fw: dissect_fw.c
