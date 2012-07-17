ARM_PATH=~/arm-toolchain
ARM_BINPATH=$(ARM_PATH)/bin
GCC_VERSION=4.4.2
CC=$(ARM_BINPATH)/arm-elf-gcc-$(GCC_VERSION)
OBJCOPY=$(ARM_BINPATH)/arm-elf-objcopy
AR=$(ARM_BINPATH)/arm-elf-ar
RANLIB=$(ARM_BINPATH)/arm-elf-ranlib
LD=$(CC)
HOST_CC=gcc
HOST_CFLAGS=-g -O3 -W -Wall

# Naming convention for Magic Lantern builds:
# General rules:
# - Always specify the camera and its firmware version number in the build name (e.g. 550d.fw109)
# - For non-release builds, specify the build date and author's (nick)name.
# - For experimental builds, add a short keyword indicating the particular feature tested.

# Examples for experimental builds:
# magiclantern-2010dec07.550d.fw108.cropmarks.a1ex.zip 
# magiclantern-2010nov23.550d.fw108.selectable-audio.piers.zip 

# Example for pre-release builds:
# magiclantern-2010dec17.550d.fw109.PRERELEASE.alex.zip

# Release builds:
# magiclantern-0.2.0.rc1.550d.fw109.zip
#~ VERSION=0.2.0.rc1.550d.fw109

BUILDVER=iso+menu.$(shell whoami)
VERSION:=$(shell date +'%Y%b%d').550d.fw109.$(BUILDVER)

#MacOS
#UMOUNT=hdiutil unmount
#CF_CARD="/Volumes/EOS_DIGITAL"

#Linux (Ubuntu 10.04)
CF_CARD=/media/EOS_DIGITAL/
UMOUNT=umount

all: magiclantern.fir


CONFIG_PYMITE		= n
CONFIG_RELOC		= n
CONFIG_TIMECODE		= n
CONFIG_LUA		= n

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF010000
RESTARTSTART		= 0x0008B000

# Firmware file IDs
FIRMWARE_ID_5D		= 0x80000218
FIRMWARE_ID_7D		= 0x80000250
FIRMWARE_ID_550D	= 0x80000270
FIRMWARE_ID		= $(FIRMWARE_ID_550D)

# PyMite scripting paths
PYMITE_PATH		= $(HOME)/build/pymite-08
PYMITE_LIB		= $(PYMITE_PATH)/src/vm/libpmvm_dryos.a
PYMITE_CFLAGS		= \
	-I$(PYMITE_PATH)/src/vm \
	-I$(PYMITE_PATH)/src/platform/dryos \

# Lua includes and libraries
LUA_PATH		= ./lua
LUA_LIB			= $(LUA_PATH)/liblua.a
LUA_CFLAGS		= -I$(LUA_PATH)

ifeq ($(CONFIG_LUA),y)
include $(LUA_PATH)/Makefile
endif


#
# Install a normal firmware file to the CF card.
#
install: magiclantern.fir magic.cfg autoexec.bin
	cp $^ $(CF_CARD)
	$(UMOUNT) $(CF_CARD)

zip: magiclantern-$(VERSION).zip

%.pdf: 
	cp doc/*.pdf .
%.bmp: 
	cp vram/*.bmp .

# zip.txt must be the first item on the list!
magiclantern-$(VERSION).zip: \
	zip.txt \
	magiclantern.fir \
	autoexec.bin \
	README \
	INSTALL.pdf \
	UserGuide.pdf \
	hd_ta.bmp\
	CineScop.bmp\
	fish8r.bmp\
	magic.cfg\
	make_bootable.sh

	-rm $@
	zip -z $@ < $^


FLAGS=\
	-Wp,-MMD,$(dir $@).$(notdir $@).d \
	-Wp,-MT,$@ \
	-nostdlib \
	-mlong-calls \
	-fomit-frame-pointer \
	-fno-strict-aliasing \
	-DCONFIG_MAGICLANTERN=1 \
	-DCONFIG_550D=1 \
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

dumper7.bin: dumper7


PATCH_ADDR=804D68
DUMPER_OFFSET=`echo "ibase=16;$(PATCH_ADDR)-800120;obase=10"|bc`
TOOLSDIR=../dumper
FIRNAME=7d000123.fir
UPDATER1=0250_123_updater1_.bin
PATCH=dumper7.bin
DUMPER_LEN=`stat -c%s $(PATCH)`
dup7.fir: $(PATCH) $(TOOLSDIR)/$(FIRNAME) $(TOOLSDIR)/$(UPDATER1)
	cp -f $(TOOLSDIR)/$(UPDATER1) .
	dd of=$(UPDATER1) if=$(PATCH) bs=1 seek=$(DUMPER_OFFSET) count=$(DUMPER_LEN) conv=notrunc #2>/dev/null
	python ../dumper/build_fir.py $(TOOLSDIR)/$(FIRNAME) $(UPDATER1) $@

dumper7: dumper_entry.o dumper123.o
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

#dumper_entry.o: flasher-stubs.S
dumper_entry.o: dumper_entry.S

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
	stubs-550d.109.o \
	version.o \
	bmp.o \
	font-huge.o \
	font-large.o \
	font-med.o \
	font-small.o \
	config.o \
	menu.o \
	debug.o \
	stdio.o \
	audio.o \
	lens.o \
	property.o \
	gui.o \
	bootflags.o \
	hotplug.o \
	zebra.o \
	focus.o \
	bracket.o \
	shoot.o \
	spotmeter.o \

NO=\
	ptp.o \

ML_OBJS-$(CONFIG_PYMITE) += \
	script.o \
	pymite-plat.o \
	pymite-nat.o \
	pymite-img.o \
	$(PYMITE_LIB) \

ML_OBJS-$(CONFIG_LUA) += \
	lua-glue.o \
	$(LUA_LIB) \

ML_OBJS-$(CONFIG_RELOC) += \
	liveview.o \
	reloc.o \

ML_OBJS-$(CONFIG_TIMECODE) += \
	timecode.o \

# Extract the stdio files that we need
STDIO_OBJ = \
	lib_a-setjmp.o \
	lib_a-strchr.o \
	lib_a-strrchr.o \
	lib_a-strlen.o \
	lib_a-strcat.o \
	lib_a-strncat.o \
	lib_a-strcmp.o \
	lib_a-strncmp.o \
	lib_a-strncpy.o \
	lib_a-strstr.o \
	lib_a-strcspn.o \
	lib_a-memcmp.o \
	lib_a-strcoll.o \
	lib_a-ctype_.o \

ARM_LIBC_A = $(ARM_PATH)/arm-elf/lib/libc.a

$(STDIO_OBJ): $(ARM_LIBC_A)
	$(AR) xv $? $(STDIO_OBJ)
libstdio.a: $(STDIO_OBJ)
	$(AR) cr $@ $^


magiclantern: $(ML_OBJS-y) libstdio.a 
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
		BEGIN { print "#define SYM(a,n) n=a; .global n;\n" }\
		s/[\r\n]//g; \
		s/^\s*0001:([0-9A-Fa-f]+)\s+([^\s]+)$$/SYM(0x\1,\2)\n/\
			and print;'


%.dis: %.bin
	$(ARM_BINPATH)/arm-linux-objdump \
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

magiclantern-5d.fir: autoexec.bin
	$(call build,ASSEMBLE,./assemble_fw \
		--output $@ \
		--user $< \
		--offset 0x120 \
		--flasher empty.bin \
		--id $(FIRMWARE_ID) \
		--zero \
	)

#
# Replace the start of the 550d firmware file with our own image
# We don't want to distribute any Canon code, so we replace the
# unencrypted flasher file with a zero-padded version.
#
550d-flasher.bin: autoexec.bin
	#cp ../1.0.8/0270_108_updaters.bin $@
	dd of=$@ if=/dev/zero bs=1829408 count=1
	dd \
		of=$@ \
		if=$< \
		bs=1 \
		conv=notrunc \
		seek=0 \

550d-empty.fir: 550d-empty.hdr
	( cat $< ; \
	dd if=/dev/zero bs=9538232 count=1 \
	) > $@

magiclantern.fir: ../dumper/550d_109.fir autoexec.bin
	@if [ -f ../dumper/build_fir.py ]; then \
		python ../dumper/build_fir.py -r $^ $@ ; \
	else \
		echo "\nNotice: ../dumper/build_fir.py not found; will not build magiclantern.fir. It's okay."; \
		[ -f magiclantern.fir ] && echo "Leaving magiclantern.fir unchanged.";\
		[ ! -f magiclantern.fir ] && echo "Please download magiclantern.fir from http://magiclantern.wikia.com/wiki/550D";\
		echo "";\
	fi; \

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
	@if [ X"$V" = X"1" ]; then \
		echo '$2'; \
	else \
		printf "[ %-8s ]   %s\n"  $1 $@; \
	fi; \
	$2


clean:
	-$(RM) \
		*.o \
		*.a \
		.*.d \
		font-*.c \
		magiclantern.lds \
		$(LUA_PATH)/*.o \
		$(LUA_PATH)/.*.d \
		*.pdf \
		*.bmp \

-include .*.d
