ARM_PATH=/usr/local/arm/oe/bin
ARM_CC=$(ARM_PATH)/arm-linux-gcc
MAP=5D21070a.map
CFLAGS=\
	-g \
	-O3 \
	-Wall \
	-W \
	-fpic \
	-static \
	-fomit-frame-pointer \

%.s: %.c
	$(CC) $(CFLAGS) -S -o $@ $<
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%: %.c
	$(CC) $(CFLAGS) -o $@ $<


rom.list: ROM0.bin
	$(ARM_PATH)/arm-linux-objdump \
		-b binary \
		-m arm \
		-D \
		$^ \
	| ./fixup-rel $(MAP) - \
	> $@

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
		--cc $(CC) \
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

%.1.flasher.bin: %.fir dissect_fw
	./dissect_fw $< . $(basename $<)

#ROM0.bin: 5d200107.fir


# Firmware manipulation tools
dissect_fw: dissect_fw.c
