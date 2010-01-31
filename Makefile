ARM_PATH=/opt/local/bin
CC=$(ARM_PATH)/arm-elf-gcc-4.3.2
OBJCOPY=$(ARM_PATH)/arm-elf-objcopy
LD=$(CC)
HOST_CC=gcc
HOST_CFLAGS=-g -O3 -W -Wall
VERSION=0.1.7

CONFIG_PYMITE		= n
CONFIG_RELOC		= n
CONFIG_TIMECODE		= y
CONFIG_LUA		= n

# 5D memory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF810000
RESTARTSTART		= 0x00048000

# Firmware file IDs
FIRMWARE_ID_5D		= 0x80000218
FIRMWARE_ID_7D		= 0x80000250
FIRMWARE_ID		= $(FIRMWARE_ID_5D)

# PyMite scripting paths
PYMITE_PATH		= $(HOME)/build/pymite-08
PYMITE_LIB		= $(PYMITE_PATH)/src/vm/libpmvm_dryos.a
PYMITE_CFLAGS		= \
	-I$(PYMITE_PATH)/src/vm \
	-I$(PYMITE_PATH)/src/platform/dryos \

# Lua includes and libraries
LUA_PATH		= $(HOME)/build/lua-5.1.4
LUA_LIB			= $(LUA_PATH)/src/liblua.a
LUA_CFLAGS		= -I$(LUA_PATH)/src


all: magiclantern.fir

CF_CARD="/Volumes/EOS_DIGITAL"

#
# Install a normal firmware file to the CF card.
#
install: magiclantern.fir magiclantern.cfg cropmarks.bmp autoexec.bin test.pym
	cp $^ $(CF_CARD)
	hdiutil unmount $(CF_CARD)

zip: magiclantern-$(VERSION).zip

# zip.txt must be the first item on the list!
magiclantern-$(VERSION).zip: \
	zip.txt \
	magiclantern.fir \
	magiclantern.cfg \
	cropmarks.bmp \
	autoexec.bin \
	README \
	LICENSE \

	-rm $@
	zip -z $@ < $^


FLAGS=\
	-Wp,-MMD,.$@.d \
	-Wp,-MT,$@ \
	-nostdlib \
	-fomit-frame-pointer \
	-fno-strict-aliasing \
	-DRESTARTSTART=$(RESTARTSTART) \
	-DROMBASEADDR=$(ROMBASEADDR) \
	-DVERSION=\"$(VERSION)\" \


CFLAGS=\
	$(FLAGS) \
	-g \
	-O3 \
	-Wall \
	-W \
	-Wno-unused-parameter \
	-D__ARM__ \

ifeq ($(CONFIG_PYMITE),y)
CFLAGS += $(PYMITE_CFLAGS)
endif

ifeq ($(CONFIG_LUA),y)
CFLAGS += $(LUA_CFLAGS)
endif

NOT_USED_FLAGS=\
	-march=armv5te \
	-mthumb-interwork \
	-msoft-float \

AFLAGS=\
	$(FLAGS) \


%.s: %.c
	$(call build,CC -S,$(CC) $(CFLAGS) -S -o $@ $<)
%.o: %.c
	$(call build,CC,$(CC) $(CFLAGS) -c -o $@ $<)
%.i: %.c
	$(call build,CPP,$(CC) $(CFLAGS) -E -c -o $@ $<)
%: %.c
	$(call build,LD,$(CC) $(CFLAGS) -o $@ $<)
%.o: %.S
	$(call build,AS,$(CC) $(AFLAGS) -c -o $@ $<)
%.bin: %
	$(call build,OBJCOPY,$(OBJCOPY) -O binary $< $@)

dumper: dumper_entry.o dumper.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-mthumb-interwork \
		-march=armv5te \
		-e _start \
		$^ \
	)

test: test.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-mthumb-interwork \
		-march=armv5te \
		-e _start \
		$^ \
	)

dumper_entry.o: flasher-stubs.S

reboot.o: reboot.c magiclantern.bin

5d-hack.bin: 5d-hack

magiclantern.lds: magiclantern.lds.S
	$(call build,CPP,$(CPP) $(CFLAGS) $< | grep -v '^#' > $@)

# magiclantern.lds script MUST be first
# entry.o MUST be second
# menu.o and debug.o must come before the modules
ML_OBJS-y = \
	magiclantern.lds \
	entry.o \
	5d-hack.o \
	stdio.o \
	menu.o \
	debug.o \
	gui.o \
	audio.o \
	lens.o \
	focus.o \
	zebra.o \
	spotmeter.o \
	hotplug.o \
	config.o \
	bootflags.o \
	ptp.o \
	bmp.o \
	bracket.o \
	font-huge.o \
	font-large.o \
	font-med.o \
	font-small.o \
	stubs-5d2.110.o \
	version.o \


ML_OBJS-$(CONFIG_PYMITE) += \
	script.o \
	pymite-plat.o \
	pymite-nat.o \
	pymite-img.o \
	$(PYMITE_LIB) \

ML_OBJS-$(CONFIG_LUA) += \
	$(LUA_LIB) \

ML_OBJS-$(CONFIG_RELOC) += \
	liveview.o \
	reloc.o \

ML_OBJS-$(CONFIG_TIMECODE) += \
	timecode.o \

magiclantern: $(ML_OBJS-y)
	$(call build,LD,$(LD) \
		-o $@ \
		-N \
		-nostdlib \
		-mthumb-interwork \
		-march=armv5te \
		-T \
		$^ \
		-lm \
		-lgcc \
	)

# These do not need to be run.  Since bigtext is not
# a standard program, the output files are checked in.
font-huge.in: generate-font
	$(call build,'GENFONT',./$< > $@ \
		'-*-helvetica-*-r-*-*-72-*-100-100-*-*-iso8859-*' \
		40 66 \
	)
font-large.in: generate-font
	$(call build,'GENFONT',./$< > $@ \
		'-*-helvetica-*-r-*-*-34-*-100-100-*-*-iso8859-*' \
		19 25 \
	)
font-med.in: generate-font
	$(call build,'GENFONT',./$< > $@ \
		'-*-helvetica-*-r-*-*-17-*-100-100-*-*-iso8859-*' \
		10 16 \
	)
font-small.in: generate-font
	$(call build,'GENFONT',./$< > $@ \
		'-*-helvetica-*-r-*-*-10-*-100-100-*-*-iso8859-*' \
		6 8 \
	)

font-huge.c: font-huge.in mkfont
	$(call build,MKFONT,./mkfont \
		< $< \
		> $@ \
		-width 60 \
		-height 70 \
		-name font_huge \
	)

font-large.c: font-large.in mkfont
	$(call build,MKFONT,./mkfont \
		< $< \
		> $@ \
		-width 28 \
		-height 32 \
		-name font_large \
	)

font-med.c: font-med.in mkfont
	$(call build,MKFONT,./mkfont \
		< $< \
		> $@ \
		-width 12 \
		-height 16 \
		-name font_med \
	)

font-small.c: font-small.in mkfont
	$(call build,MKFONT,./mkfont \
		< $< \
		> $@ \
		-width 8 \
		-height 12 \
		-name font_small \
	)

version.c: FORCE
	$(call build,VERSION,( \
		echo 'const char build_version[] = "$(VERSION)";' ; \
		echo 'const char build_id[] = "'`hg id`'";' ; \
		echo 'const char build_date[] ="'`date -u "+%Y-%m-%d %H:%M:%S"`'";' ; \
		echo 'const char build_user[] = "'`whoami`@`hostname`'";' ; \
	) > $@)

autoexec: reboot.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-mthumb-interwork \
		-march=armv5te \
		-e _start \
		-Ttext=0x800000 \
		$^ \
	)

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

ifdef FETCH_FROM_CANON
%.1.flasher.bin: %.fir dissect_fw
	./dissect_fw $< . $(basename $<)
endif

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
		--id $(FIRMWARE_ID) \

magiclantern.fir: autoexec.bin
	$(call build,ASSEMBLE,./assemble_fw \
		--output $@ \
		--user $< \
		--offset 0x120 \
		--flasher empty.bin \
		--id $(FIRMWARE_ID) \
		--zero \
	)

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


#
# Embedded Python scripting
#
SCRIPTS=\
	main.py \

#	$(PYMITE_PATH)/src/tools/pmImgCreator.py \

pymite-nat.c pymite-img.c: $(SCRIPTS)
	$(call build,PYMITE,\
	./pymite-compile \
		-c \
		-u \
		-o pymite-img.c \
		--native-file=pymite-nat.c \
		$^ \
	)

%.pym: %.py
	$(call build,PYMITE,\
	./pymite-compile \
		-b \
		-u \
		-o $@ \
		$^ \
	)

# Quiet the build process
build = \
	@if [ "$V" == 1 ]; then \
		echo '$2'; \
	else \
		printf "[ %-8s ]   %s\n"  $1 $@; \
	fi; \
	$2


clean:
	-rm -f *.o *.a font-*.c

-include .*.d
