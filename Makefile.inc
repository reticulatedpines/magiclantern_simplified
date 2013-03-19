# Generic makefile
# Include default and user-customized options where needed
# If you want to customize them copy Makefile.user.default to Makefile.user keeping only the options that you want to customize
include $(TOP_DIR)/Makefile.user.default
-include $(TOP_DIR)/Makefile.user

ifeq ($(CONFIG_CCACHE), y)
    CC := $(CCACHE) $(CC)
    HOST_CC := $(CCACHE) $(CC)
endif	

UNAME:=$(shell uname)

ifeq ($(UNAME), Darwin)
	# Variable declaration for Mac OS X
	UMOUNT=hdiutil unmount
	CF_CARD="/Volumes/EOS_DIGITAL"
	STAT_CMD=stat -f "%N: %z bytes"
else
	# Default settings for remaining operating systems
	UMOUNT=umount
	CF_CARD=/media/EOS_DIGITAL/
	STAT_CMD=stat -c "%n: %s bytes"
endif


# PyMite scripting paths
PYMITE_PATH		= $(HOME)/build/pymite-08
PYMITE_LIB		= $(PYMITE_PATH)/src/vm/libpmvm_dryos.a
PYMITE_CFLAGS		= \
	-I$(PYMITE_PATH)/src/vm \
	-I$(PYMITE_PATH)/src/platform/dryos \


#
# Install a normal firmware file to the CF card.
#
all: autoexec.bin

menuindexentries.h:
	$(call build,"MENU IDX", if [ -f $(DOC_DIR)/menuindex.txt ]; then \
		cd $(DOC_DIR); $(PYTHON) menuindex.py; \
	else \
		echo "No menuindex.txt not running \"$(PYTHON) menuindex.py\""; \
	fi)

menuindex.o: menuindexentries.h

install: autoexec.bin
	cp $^ $(CF_CARD)

	mkdir $(CF_CARD)/ML || echo "no problem"
	mkdir $(CF_CARD)/ML/settings || echo "no problem"
	mkdir $(CF_CARD)/ML/data || echo "no problem"
	cp $(SRC_DIR)/FONTS.DAT $(CF_CARD)/ML/data/
	mkdir $(CF_CARD)/ML/cropmks || echo "no problem"
	cp $(CROP_DIR)/*.bmp $(CF_CARD)/ML/cropmks/
	mkdir $(CF_CARD)/ML/scripts || echo "no problem"
	cp $(SCRIPT_DIR)/*.lua $(CF_CARD)/ML/scripts/
	cp $(SCRIPT_DIR)/*.c $(CF_CARD)/ML/scripts/
	mkdir $(CF_CARD)/ML/plugins || echo "no problem"
	cp $(PLUGINS_DIR)/*/*.bin $(CF_CARD)/ML/plugins/
	cp $(VRAM_DIR)/*.lut $(CF_CARD)/ML/data/

	$(UMOUNT) $(CF_CARD)

copy-autoexec: autoexec.bin
	cp autoexec.bin $(CF_CARD)
	$(UMOUNT) $(CF_CARD)

ptpinstall: autoexec.bin
	gvfs-mount -s gphoto2
	sleep 2
	cat $(TOP_DIR)/src/ptpcam.txt | ptpcam --chdk
	diff autoexec.bin autoexec.test

prepare_zip:
	rm -r zip || echo "ok"
	mkdir zip
	mkdir zip/ML || echo "ok"
	cp $(SRC_DIR)/zip.txt zip
	cp $(TOP_DIR)/README zip/ML/
	cp $(DOC_DIR)/INSTALL.pdf zip/
	cp $(DOC_DIR)/UserGuide.pdf zip/
	mkdir zip/ML/logs || echo "ok"
	mkdir zip/ML/settings || echo "ok"
	mkdir zip/ML/data || echo "ok"
	cp $(SRC_DIR)/FONTS.DAT zip/ML/data
	cp $(VRAM_DIR)/*.lut zip/ML/data
	mkdir zip/ML/cropmks || echo "ok"
	cp $(CROP_DIR)/*.bmp zip/ML/cropmks/
	mkdir zip/ML/scripts || echo "ok"
	cp $(SCRIPT_DIR)/*.lua zip/ML/scripts/
	cp $(SCRIPT_DIR)/*.c zip/ML/scripts/
	#mkdir zip/ML/plugins || echo "ok"
	#cp $(PLUGINS_DIR)/*/*.bin zip/ML/plugins/
	mkdir zip/ML/doc || echo "ok"
	cp $(DOC_DIR)/cam/* zip/ML/doc/
	cp $(SRC_DIR)/logo.bmp zip/ML/docs
	make x
	cp ../../installer/550D.109/*.fir zip || echo "no FIR"
	cp ../../installer/60D.111/*.fir zip || echo "no FIR"
	cp ../../installer/600D.102/*.fir zip || echo "no FIR"
	cp ../../installer/50D.109/*.fir zip || echo "no FIR" 
	cp ../../installer/500D.111/*.fir zip || echo "no FIR"
	cp ../../installer/5D2.212/*.fir zip || echo "no FIR"
	cp ../../installer/1100D.105/*.fir zip || echo "no FIR"
	cp ../../installer/EOSM.100/*.fir zip || echo "no FIR"
	cp ../../installer/650D.101/*.fir zip || echo "no FIR"

# zip.txt must be the first item on the list!
magiclantern-$(VERSION).zip: \
	zip/zip.txt \
	zip/ML/README \
	zip/INSTALL.pdf \
	zip/UserGuide.pdf \

	-rm $@
	cp autoexec.bin zip
	chmod -x zip/autoexec.bin
	cd zip; zip -z ../$@ < \
		zip.txt \
		ML/README \
		INSTALL.pdf \
		UserGuide.pdf \
		ML/cropmks/* \
		ML/doc/* \
		ML/data/* \
		ML/scripts/*.c \
		*.fir \
		autoexec.bin \

		#~ ML/scripts/* \
		#~ ML/plugins/* \

CFLAGS=\
	$(FLAGS) \
	-Os \
	-Wall \
	-W \
	-Wno-unused-parameter \
	-Wno-implicit-function-declaration \
	-Wno-unused-function \
	-Wno-missing-field-initializers \
	-Wno-format \
	-Wdouble-promotion \
	-ffast-math \
	-fno-trapping-math \
	-fsingle-precision-constant \
	-fshort-double \
	-std=gnu99 \
	-D__ARM__ \
	-I$(PLATFORM_INC) \
	-I$(SRC_DIR) \
	-I$(PICOC_PATH) \
	-mthumb-interwork \

NOT_USED_FLAGS=\
	-march=armv5te \
	-mthumb-interwork \
	-msoft-float \

AFLAGS=\
	$(FLAGS) \
    
LFLAGS=

CFLAGS += $(CFLAG_USER)
LFLAGS += $(LFLAG_USER)
AFLAGS += $(AFLAG_USER)

ifeq ($(CONFIG_PYMITE),y)
CFLAGS += $(PYMITE_CFLAGS)
endif

ifeq ($(CONFIG_PTP),y)
CFLAGS += -DCONFIG_PTP
endif

ifeq ($(CONFIG_GDB),y)
CFLAGS += -DCONFIG_GDB
endif

ifeq ($(CONFIG_GDBSTUB),y)
CFLAGS += -DCONFIG_GDB -DCONFIG_GDBSTUB
endif

ifeq ($(CONFIG_PIC), y)
CFLAGS += -fPIC -DPOSITION_INDEPENDENT
AFLAGS += -fPIC -DPOSITION_INDEPENDENT
LFLAGS += -fPIC
# fixme: dietlib with -fPIC
endif

ifeq ($(CONFIG_PICOC),y)
ifneq ($(CONFIG_CONSOLE),y)
$(error CONFIG_PICOC requires CONFIG_CONSOLE = y, please enable it)
endif
CFLAGS += -DCONFIG_PICOC
endif

ifeq ($(CONFIG_TCC),y)
ifneq ($(CONFIG_CONSOLE),y)
$(error CONFIG_TCC requires CONFIG_CONSOLE = y, please enable it)
endif
CFLAGS += -DCONFIG_TCC
endif

ifeq ($(CONFIG_CONSOLE),y)
CFLAGS += -DCONFIG_CONSOLE
endif


FLAGS=\
	-Wp,-MMD,$(patsubst %.o,%.d,$(dir $@)$(notdir $@)) \
	-Wp,-MT,$@ \
	-nostdlib \
	-fomit-frame-pointer \
	-fno-strict-aliasing \
	-DCONFIG_MAGICLANTERN=1 \
	-DCONFIG_$(subst .,_,$(MODEL))=1 \
	-DRESTARTSTART=$(RESTARTSTART) \
	-DROMBASEADDR=$(ROMBASEADDR) \
	-DVERSION=\"$(VERSION)\" \
	-DCONFIG_DEBUGMSG=$(CONFIG_DEBUGMSG) \


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

reboot.o: $(SRC_DIR)/reboot.c magiclantern.bin

magiclantern.lds: $(SRC_DIR)/magiclantern.lds.S
	$(call build,CPP,$(CPP) $(CFLAGS) $< | grep -v '^#' > $@)

ifneq ($(CONFIG_NO_EXTRA_OBJS),y)

ML_OBJS-y += \
	ico.o \

ML_OBJS-$(CONFIG_GDB) += \
	gdb.o \

ML_OBJS-$(CONFIG_PTP) += \
	ptp.o \

ML_OBJS-$(CONFIG_PTP_CHDK) += \
	ptp-chdk.o \

ML_OBJS-$(CONFIG_PTP_ML) += \
	ptp-ml.o \

ML_OBJS-$(CONFIG_PYMITE) += \
	script.o \
	pymite-plat.o \
	pymite-nat.o \
	pymite-img.o \
	$(PYMITE_LIB) \

ML_OBJS-$(CONFIG_PLUGINS) += \
	plugin.o \

ML_OBJS-$(CONFIG_RELOC) += \
	liveview.o \
	reloc.o \

ML_OBJS-$(CONFIG_TIMECODE) += \
	timecode.o \

ML_OBJS-$(CONFIG_CONSOLE) += \
	console.o \

ML_OBJS-$(CONFIG_PICOC)	+= \
	pico.o \
	picoc.o \
	chdk-gui_script.o \

ML_OBJS-$(CONFIG_TCC) += \
	tcc-glue.o \
	tcc-gui.o \
	chdk-gui_script.o \
	script_lib.o \
	$(TOP_DIR)/tcc/libtcc.a \

endif

$(TOP_DIR)/tcc/libtcc.a: FORCE
	make -C $(TOP_DIR)/tcc

# Extract the stdio files that we need
DIET_OBJ = \
	strrchr.o \
	strncpy.o \
	strlen.o \
	strncmp.o \
	strcpy.o \
	strcmp.o \
	memset.o \
	memcpy.o \
	memccpy.o \
	setjmp.o \
	longjmp.o \
	__longjmp.o \
	sigjmp.o \
	sigprocmask.o \
	rt_sigprocmask.o \
	unified.o \
	errno.o \
	errno_location.o \
	sscanf.o \
	vsscanf.o \
	__v_scanf.o \
	sprintf.o \
	vsprintf.o \
	strtold.o \
	strtod.o \
	memmove.o \
	strtof.o \
	strtoull.o \
	strtol.o \
	strtoul.o \
	strchr.o \
	memcmp.o \

DIETLIBC_A = $(SRC_DIR)/libs/dietlib-0.33-none-eabi-Os/dietlibc.a

$(DIET_OBJ): $(DIETLIBC_A)
	$(call build,AR,$(AR) x $? $(DIET_OBJ))

dietlibc.a: $(DIET_OBJ)
	$(call build,AR,$(AR) cr $@ $^)

magiclantern: $(ML_OBJS-y) dietlibc.a
	$(call build,LD,$(LD) \
		-o $@ \
		-N \
		$(LFLAGS) \
		-nostdlib \
		-mthumb-interwork \
		-march=armv5te \
		-T \
		$^ \
		-lm \
		-lgcc \
        -Xlinker -Map=location.map \
	)


# These do not need to be run.  Since bigtext is not
# a standard program, the output files are checked in.
#font-large.in: generate-font
#	$(call build,'GENFONT',./$< > $@ \
#		'fixed-bold-32' \
#		16 30 \
#	)
#font-med.in: generate-font
#	$(call build,'GENFONT',./$< > $@ \
#		'-misc-fixed-medium-r-normal--20-2' \
#		10 15 \
#	)
#font-small.in: generate-font
#	$(call build,'GENFONT',./$< > $@ \
#		'terminus-bold-12' \
#		6 10 \
#	)


#font-large.c: font-large.in mkfont
#	$(call build,MKFONT,./mkfont \
#		< $< \
#		> $@ \
#		-width 20 \
#		-height 32 \
#		-name font_large \
#	)
#
#font-med.c: font-med.in mkfont
#	$(call build,MKFONT,./mkfont \
#		< $< \
#		> $@ \
#		-width 12 \
#		-height 20 \
#		-name font_med \
#	)
#
#font-small.c: font-small.in mkfont
#	$(call build,MKFONT,./mkfont \
#		< $< \
#		> $@ \
#		-width 8 \
#		-height 12 \
#		-name font_small \
#	)

$(PLATFORM_DIR)/version.c: FORCE
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
		-march=armv5te \
		-e _start \
		-Ttext $(AUTOEXEC_BASE) \
		$^ -Xlinker --cref -Xlinker -Map=autoexec.map \
	); \
	$(STAT_CMD) autoexec.bin; \
	$(READELF) -l magiclantern | grep -C 2 MemSiz

autoexec-fir: reboot.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-march=armv5te \
		-e _start \
		-Ttext $(FIR_BASE) \
		$^ -Xlinker --cref -Xlinker -Map=autoexec.map \
	);

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

magiclantern.fir: autoexec.bin
	@if [ -f $(BUILDER_DIR)/build_fir.py ]; then \
		$(PYTHON) $(BUILDER_DIR)/build_fir.py -r $^ $@ ; \
	else \
		echo "\nNotice: $(BUILDER_DIR)/build_fir.py not found; will not build magiclantern.fir. It's okay."; \
		[ -f magiclantern.fir ] && echo "Leaving magiclantern.fir unchanged.";\
		[ ! -f magiclantern.fir ] && echo "Please download magiclantern.fir ";\
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
# Embedded $(PYTHON) scripting
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

docs:
	cd $(DOC_DIR) && $(PYTHON) mkdoc.py
	cd $(DOC_DIR) && $(PYTHON) mkdoc-cam.py

docq:
	cd $(DOC_DIR) && $(PYTHON) mkdoc.py
	cd $(DOC_DIR) && $(PYTHON) mkdoc-cam.py quick

installdoc:
	mkdir $(CF_CARD)/ML/doc || echo "no problem"
	cp -v $(DOC_DIR)/cam/* $(CF_CARD)/ML/doc/

clean:
	$(call build,CLEAN,$(RM) -f \
		*.o   \
		*.d \
		*.a \
        *.map \
		$(BIN_FILES) \
		magiclantern.lds \
		*.bmp \
		*.pdf \
		*.txt \
		make_bootable.sh \
		422-jpg.py \
		README \
		data/cropmks/* \
		scripts/* \
		doc/* \
		$(SRC_DIR)/*.o \
		)
	$(call build,CLEAN,$(RM) -rf zip)

COBJS := $(filter %.o, $(ML_OBJS-y))
DEPS  := $(COBJS:.o=.d)
-include $(DEPS)

%.s: %.c
	$(call build,CC -S,$(CC) $(CFLAGS) -S -o $@ $<)
%.o: $(PLATFORM_DIR)/%.c
	$(call build,CC,$(CC) $(CFLAGS) -c -o $@ $<)
%.o: $(SRC_DIR)/%.c
	$(call build,CC,$(CC) $(CFLAGS) -c -o $@ $<)
%.o: $(PICOC_PATH)/%.c
	$(call build,CC,$(CC) $(CFLAGS) -c -mthumb -fwhole-program -o $@ $<)
%.i: %.c
	$(call build,CPP,$(CC) $(CFLAGS) -E -c -o $@ $<)
%: %.c
	$(call build,LD,$(CC) $(CFLAGS) -o $@ $<)
%.o: $(PLATFORM_DIR)/%.S
	$(call build,AS,$(CC) $(AFLAGS) -c -o $@ $<)
%.o: $(SRC_DIR)/%.S
	$(call build,AS,$(CC) $(AFLAGS) -c -o $@ $<)
%.bin: %
	$(call build,OBJCOPY,$(OBJCOPY) -O binary $< $@)

zip: prepare_zip magiclantern-$(VERSION).zip
