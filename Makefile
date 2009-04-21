ARM_PATH=/usr/local/arm/oe/bin
MAP=5D21070a.map
CC=$(ARM_PATH)/arm-linux-gcc
CFLAGS=\
	-O3 \
	-Wall \
	-W \
	-fomit-frame-pointer \

%.s: %.c
	$(CC) $(CFLAGS) -S -o $@ $<
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


rom.list: ROM0.bin
	$(ARM_PATH)/arm-linux-objdump \
		-b binary \
		-m arm \
		-D \
		$^ \
	| ./fixup-rel $(MAP) - \
	> $@

rom.raw: ROM0.bin
	$(ARM_PATH)/arm-linux-objdump \
		-b binary \
		-m arm \
		-D \
		$^ \
	> $@

ROM0.elf: ROM0.subs.S
	$(CC) \
		-Wl,-N,-Ttext,0 \
		-nostdlib \
		-o $@ \
		$^

strings: ROM0.bin
	strings -t x $^

ROM0.bin: FORCE
FORCE:


5d200107.fir: eos5d2107.exe
	unzip -o $<

eos5d2107.exe:
	wget http://web.canon.jp/imaging/eosd/firm-e/eos5dmk2/data/eos5d2107.exe
