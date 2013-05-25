# Generic makefile
include $(TOP_DIR)/Makefile.setup

#
# here are Magic Lantern Core additional CONFIG_ defines
#
ifeq ($(CONFIG_PYMITE),y)
CFLAGS += $(PYMITE_CFLAGS)
endif

ifeq ($(CONFIG_PTP),y)
CFLAGS += -DCONFIG_PTP
endif

ifeq ($(CONFIG_MEMCHECK),y)
CFLAGS += -DCONFIG_MEMCHECK
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

ifeq ($(CONFIG_MODULES),y)
ifneq ($(CONFIG_TCC),y)
$(error CONFIG_MODULES requires CONFIG_TCC = y, please enable it)
endif
CFLAGS += -DCONFIG_MODULES
endif

ifeq ($(CONFIG_QEMU),y)
CFLAGS += -DCONFIG_QEMU -DCONFIG_STATIC_FONTS
endif


# PyMite scripting paths
PYMITE_PATH		= $(HOME)/build/pymite-08
PYMITE_LIB		= $(PYMITE_PATH)/src/vm/libpmvm_dryos.a
PYMITE_CFLAGS	= \
	-I$(PYMITE_PATH)/src/vm \
	-I$(PYMITE_PATH)/src/platform/dryos

ifeq ($(ML_BITRATE_OBJ), n)
ML_BITRATE_OBJ =
else ifndef ML_BITRATE_OBJ
ML_BITRATE_OBJ = bitrate.o
endif

ifeq ($(ML_AUDIO_OBJ), n)
ML_AUDIO_OBJ =
else ifndef ML_AUDIO_OBJ
ML_AUDIO_OBJ = audio-ak.o
endif

ifeq ($(ML_ZEBRA_OBJ), n)
ML_ZEBRA_OBJ =
else ifndef ML_ZEBRA_OBJ
ML_ZEBRA_OBJ = zebra.o
endif

ifeq ($(ML_BOOTFLAGS_OBJ), n)
ML_BOOTFLAGS_OBJ =
else ifndef ML_BOOTFLAGS_OBJ
ML_BOOTFLAGS_OBJ = bootflags.o
endif

ifeq ($(ML_AF_PATTERNS_OBJ), n)
ML_AF_PATTERNS_OBJ = 
else ifndef ML_AF_PATTERNS_OBJ
ML_AF_PATTERNS_OBJ = af_patterns.o
endif

ML_SMALL_FONT_OBJS = \
    font-small2.o \
    font-med2.o \
    font-large.o \

ML_STATIC_FONT_OBJS = \
	font-small.o \
	font-med.o \
	font-large.o

ML_DYNAMIC_FONT_OBJS = \
	font-dyn.o

ifeq ($(CONFIG_SMALL_FONTS), y)
ML_STATIC_FONT_OBJS = $(ML_SMALL_FONT_OBJS)
endif

ML_FONT_OBJS = $(ML_DYNAMIC_FONT_OBJS)

ifeq ($(CONFIG_STATIC_FONTS), y)
ML_FONT_OBJS += $(ML_STATIC_FONT_OBJS)
CFLAGS += -DCONFIG_STATIC_FONTS=1
endif

#
# global disabler for extra objects
#
ifneq ($(CONFIG_NO_EXTRA_OBJS),y)

ifndef CONFIG_ADD_INSTALLER_OBJECTS
CONFIG_ADD_SOURCE_OBJECTS = y
endif

#
# depending on CONFIG_ defines add .o files (may get merged into definition above?) 
#

# magiclantern.lds script MUST be first
# entry.o MUST be second
# menu.o and debug.o must come before the modules
ML_OBJS-y = \
	magiclantern.lds \
	$(SRC_DIR)/entry.o \
	$(PLATFORM_DIR)/stubs.o \
	version.o

ML_INSTALLER_OBJS = \
	installer.o \
	ico.o \
	bmp.o \
	$(ML_FONT_OBJS) \
	stdio.o \
	property.o \
	propvalues.o \
	dialog_test.o \
	bootflags.o \
	vram.o \

#	chdk-gui_draw.o

ML_SRC_OBJS = \
	boot-hack.o \
	ico.o \
	edmac.o \
	menu.o \
	debug.o \
	cfn.o \
	gui.o \
	picstyle.o \
	exmem.o \
	bmp.o \
	$(ML_FONT_OBJS) \
	config.o \
	stdio.o \
	$(ML_BITRATE_OBJ) \
	tweaks.o \
	lens.o \
	property.o \
	propvalues.o \
	gui-common.o \
	chdk-gui_draw.o \
	movtweaks.o \
	menuhelp.o \
	menuindex.o \
	$(ML_AF_PATTERNS_OBJ) \
	focus.o \
	notify_box.o \
	$(ML_BOOTFLAGS_OBJ) \
	dialog_test.o \
	vram.o \
	aj_port.o \
	fps-engio.o \
	shoot.o \
	hdr.o \
	lv-img-engio.o \
	state-object.o \
	tasks.o \
	vsync-lite.o \
	tskmon.o \
	battery.o \
	imgconv.o \
	histogram.o \
	falsecolor.o \
	$(ML_AUDIO_OBJ) \
	$(ML_ZEBRA_OBJ) \
	beep.o \


ML_OBJS-$(CONFIG_ADD_INSTALLER_OBJECTS) += \
	$(ML_INSTALLER_OBJS)

ML_OBJS-$(CONFIG_ADD_SOURCE_OBJECTS) += \
	$(ML_SRC_OBJS)

ML_OBJS-y += $(ML_EXTRA_OBJS)

ML_OBJS-$(CONFIG_GDB) += \
	gdb.o

ML_OBJS-$(CONFIG_PTP) += \
	ptp.o

ML_OBJS-$(CONFIG_MEMCHECK) += \
	memcheck.o

ML_OBJS-$(CONFIG_PTP_CHDK) += \
	ptp-chdk.o

ML_OBJS-$(CONFIG_PTP_ML) += \
	ptp-ml.o

ML_OBJS-$(CONFIG_PYMITE) += \
	script.o \
	pymite-plat.o \
	pymite-nat.o \
	pymite-img.o \
	$(PYMITE_LIB)

ML_OBJS-$(CONFIG_PLUGINS) += \
	plugin.o

ML_OBJS-$(CONFIG_RELOC) += \
	liveview.o \
	reloc.o

ML_OBJS-$(CONFIG_TIMECODE) += \
	timecode.o

ML_OBJS-$(CONFIG_CONSOLE) += \
	console.o

ML_OBJS-$(CONFIG_PICOC)	+= \
	pico.o \
	picoc.o \
	chdk-gui_script.o

ML_OBJS-$(CONFIG_TCC) += \
	tcc-glue.o \
	tcc-gui.o \
	chdk-gui_script.o \
	script_lib.o \
	$(TOP_DIR)/tcc/libtcc.a

ML_OBJS-$(CONFIG_MODULES) += \
	module.o

ML_OBJS-$(CONFIG_QEMU)	+= \
	qemu-util.o \
	$(ML_STATIC_FONT_OBJS)

ifdef ML_EXCLUDE_OBJS
ML_OBJS-y := $(filter-out $(ML_EXCLUDE_OBJS), $(ML_OBJS-y))
endif

endif

ifeq ($(CONFIG_TINYPY),y)
LFLAGS += \
	-u pow \
	-u qsort \
	-u strstr \
	-u atof \

endif

#
# here areMagic Lantern Core rules
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

	mkdir $(CF_CARD_ML_DIR) || echo "no problem"
	mkdir $(CF_CARD_ML_DIR)/settings || echo "no problem"
	mkdir $(CF_CARD_ML_DIR)/data || echo "no problem"
	cp $(SRC_DIR)/FONTS.DAT $(CF_CARD_ML_DIR)data/
	mkdir $(CF_CARD_ML_DIR)/cropmks || echo "no problem"
	cp $(CROP_DIR)/*.bmp $(CF_CARD_ML_DIR)/cropmks/
	mkdir $(CF_CARD_ML_DIR)/scripts || echo "no problem"
	cp $(SCRIPT_DIR)/*.lua $(CF_CARD_ML_DIR)/scripts/
	cp $(SCRIPT_DIR)/*.c $(CF_CARD_ML_DIR)/scripts/
	cp $(SCRIPT_DIR)/*.py $(CF_CARD_ML_DIR)/scripts/
	cp $(VRAM_DIR)/*.lut $(CF_CARD_ML_DIR)/data/
	make -C $(TOP_DIR)/modules install
	cp $(MODULES_DIR)/*/*.mo $(CF_CARD)/ML/modules/ || echo "no problem"
#	mkdir $(CF_CARD_ML_DIR)/plugins || echo "no problem"
	cp magiclantern.sym $(CF_CARD_ML_DIR)/modules/magic.sym
	# todo: magic.sym is camera-specific, rename it (e.g. 550D-109.sym)

#	cp $(PLUGINS_DIR)/*/*.bin $(CF_CARD_ML_DIR)/plugins/

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
		#~ ML/plugins/*



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

reboot.o: $(SRC_DIR)/reboot.c magiclantern.bin magiclantern.sym

magiclantern.lds: $(SRC_DIR)/magiclantern.lds.S
	$(call build,CPP,$(CPP) $(CFLAGS) $< | grep -v '^#' > $@)

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
	atoi.o \
	atol.o \
	strstr.o \
	qsort.o \
	atof.o \

DIETLIBC_A = $(SRC_DIR)/libs/dietlib-0.33-none-eabi-Os/dietlibc.a

$(DIET_OBJ): $(DIETLIBC_A)
	$(call build,AR,$(AR) x $? $(DIET_OBJ))

dietlibc.a: $(DIET_OBJ)
	$(call build,AR,$(AR) cr $@ $^)

# use setjmp from newlib because... the one from dietlibc freezes
# memcpy/memset are also much faster in newlib
NEWLIB_OBJ = \
	lib_a-setjmp.o \
	lib_a-memcpy.o \
	lib_a-memcpy-stub.o \
	lib_a-memccpy.o \
	lib_a-memset.o \

NEWLIB_LIBC_A = $(SRC_DIR)/libs/arm-none-eabi-O3/libc.a

$(NEWLIB_OBJ): $(NEWLIB_LIBC_A)
	$(call build,AR,$(AR) x $? $(NEWLIB_OBJ))

newlib-libc.a: $(NEWLIB_OBJ)
	$(call build,AR,$(AR) cr $@ $^)

magiclantern: $(ML_OBJS-y) dietlibc.a newlib-libc.a
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

$(PLATFORM_DIR)/version.c: FORCE
		echo 'const char build_version[] = "$(VERSION)";' > $@
		echo 'const char build_id[] = "'`hg id`'";' >> $@
		echo 'const char build_date[] ="'`date -u "+%Y-%m-%d %H:%M:%S"`'";' >> $@
		echo 'const char build_user[] = "'`whoami`@`hostname`'";'  >> $@

autoexec: reboot.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-march=armv5te \
		-e _start \
		-Ttext $(AUTOEXEC_BASE) \
		$^ -Xlinker --cref -Xlinker -Map=autoexec.map \
	); \
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

qemu-helper: qemu-helper.o stubs.S
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-march=armv5te \
		-e _start \
		-Ttext 0x30000000 \
		$^ \
		-lm -lgcc \
	); \

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

docs:
	cd $(DOC_DIR) && $(PYTHON) mkdoc.py
	cd $(DOC_DIR) && $(PYTHON) mkdoc-cam.py

docq:
	cd $(DOC_DIR) && $(PYTHON) mkdoc.py
	cd $(DOC_DIR) && $(PYTHON) mkdoc-cam.py quick

installdoc:
	mkdir $(CF_CARD_ML_DIR)/doc || echo "no problem"
	cp -v $(DOC_DIR)/cam/* $(CF_CARD_ML_DIR)/doc/

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

zip: prepare_zip magiclantern-$(VERSION).zip

#
# are these still relevant?
#

BASE=0xFF800000

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

include $(TOP_DIR)/Makefile.filerules

FORCE:

