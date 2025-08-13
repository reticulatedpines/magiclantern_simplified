# This makefile can be used directly, to build zips for all the default enabled cams,
# or invoked by platform/99D/Makefile to build one for one cam (including non-default).
#
# Note that there is significant code duplication (at the Make level) between
# this platform/Makefile and installer/Makefile.
# This greatly reduces complexity of the Make code at the expense of higher
# maintenance cost (but we very rarely change the build system).
# If you find bugs here, you should check if they exist there, too.
TOP_DIR ?= ..

SRC_DIR = $(TOP_DIR)/src
BUILD_DIR = build
PLATFORM_DIR = $(TOP_DIR)/platform

# Some CONFIG_* settings for platform builds,
# installer uses different settings.
#
# You can override like this:
# make clean; make CONFIG_QEMU=y
CONFIG_PTP      = n
CONFIG_PTP_CHDK = n
CONFIG_PTP_ML   = n
CONFIG_GDB      = n
CONFIG_GDBSTUB  = n
CONFIG_CONSOLE  = y
CONFIG_DEBUGMSG = 0
CONFIG_CCACHE   = n
CONFIG_TCC      = y
CONFIG_MODULES  = y
CONFIG_TINYPY   = n

# Work in progress - attempt to emulate both Canon firmware and autoexec.bin
# Emulation should work without having to enable CONFIG_QEMU,
# but this may enable workarounds for some corner cases,
# or some extra tools or tests not meant to be run on the actual camera.
CONFIG_QEMU     = n

# Work in progress - load ML as position-independent code (PIC)
# SJE: this is abandoned as far as I can tell, but we need the option, to disable it.
CONFIG_PIC      = n

include $(TOP_DIR)/Makefile.globals
include $(DEPS)

### This block is platform-global,
### but not modules-global
CFLAGS += \
    -march=$(PLATFORM_ARCH) \
    -I$(PLATFORM_DIR)/$(MODEL_DIR) \
    -I$(PLATFORM_DIR)/$(MODEL_DIR)/include
CFLAGS += \
    -DCONFIG_$(MODEL)=1 \
    -DCONFIG_$(MODEL)_$(FW_VERSION)=1 \
    -DRESTARTSTART=$(RESTARTSTART) \
    -DMAIN_FIRMWARE_ADDR=$(MAIN_FIRMWARE_ADDR) \
    -DCAMERA_MODEL=\"$(MODEL)\" \
    -DCONFIG_FW_VERSION=$(FW_VERSION) \
    -DCURRENT_CAMERA_SIGNATURE=SIG_$(MODEL)_$(FW_VERSION) \
    -DVERSION=\"$(VERSION)\"

BUILD_DATE = $(call eval_once,BUILD_DATE,$(shell LC_TIME=EN date +'%Y-%m-%d'))
VERSION = $(BUILD_DATE).$(MODEL).$(FW_VERSION)

# Location of files to turn into zip:
ZIP_DIR = build/zip
ZIP_MODULES_DIR = $(ZIP_DIR)/ML/modules
# All modules go here before placing in zip file hierarchy,
# for any post-module-build but per-cam, pre-zip processing:
CAM_MODULES_DIR = $(BUILD_DIR)/modules

ML_CAMS = \
    100D.101 \
    1100D.105 \
    200D.101 \
    500D.111 \
    50D.109 \
    550D.109 \
    5D2.212 \
    5D3.113 \
    5D3.123 \
    5D4.133 \
    600D.102 \
    60D.111 \
    650D.104 \
    6D.116 \
    6D2.111 \
    700D.115 \
    70D.112 \
    750D.110 \
    77D.110 \
    7D2.112 \
    80D.103 \
    850D.100 \
    EOSM.202 \
    M50.110 \
    R.180 \
    R5.152 \
    RP.160 \
    SX70.111 \
    SX740.102 \
    XF605.101

.PHONY: clean zip_contents disk_image $(BUILD_DIR) FORCE

# Construct targets for the default cams
#
# R.180 -> R.180/build/cam_complete
ML_PLATFORM_TARGETS = $(addsuffix /$(BUILD_DIR)/cam_complete, $(ML_CAMS))

ifeq ($(notdir $(shell pwd)),$(notdir $(PLATFORM_DIR)))
    # working dir is "platform", set a flag so we
    # can do variants of clean, build locations, etc.
    IS_PLATFORM_LEVEL_BUILD = true
endif

ifeq ($(IS_PLATFORM_LEVEL_BUILD), true)
    # If the last part of the working dir is "platform",
    # we do some actions here that make sense for that context
    # but not for individual cams.

    # Remove all cam completion marker files.
    # This forces $(BUILD_DIR)/cam_complete rule to run,
    # via make -C for every default cam, but this is very fast if
    # no changes have occured to the cam deps.  The marker file
    # depends on all relevant parts of the cam but is
    # created by "touch".
$(shell rm -- $(ML_PLATFORM_TARGETS) > /dev/null 2>&1)
else
    # If we're not running from platform/, we assume
    # we're running from platform/some_cam/ and
    # do the things we need for building one cam.
    MODEL_DIR = $(MODEL).$(FW_VERSION)

    ifndef AUTOEXEC_BASE_ADDR
        AUTOEXEC_BASE_ADDR = 0x800000
    endif
    ifndef FIR_BASE_ADDR
        FIR_BASE_ADDR = 0x800120
    endif
    ifndef MAIN_FIRMWARE_ADDR
        $(error MAIN_FIRMWARE_ADDR is not defined)
    endif
    ifndef RESTARTSTART
        $(error RESTARTSTART is not defined)
    endif
    ifndef ML_BOOT_OBJ
        $(error ML_BOOT_OBJ is not defined)
    endif
    ifndef PLATFORM_ARCH
        $(error PLATFORM_ARCH must be defined (e.g., armv5te, armv7-a))
    endif
endif

# Default target if make run in "platform" dir,
# builds those cams we expect to work and want to create zips for.
$(BUILD_DIR)/default_cams_complete: $(ML_PLATFORM_TARGETS) | $(BUILD_DIR)
	@touch $(BUILD_DIR)/default_cams_complete
	@echo Default cams build complete, success.

# Individual cams should make this target their default goal.
# It's not run when invoking make from the platform dir directly.
$(BUILD_DIR)/cam_complete: $(BUILD_DIR)/magiclantern.zip | $(BUILD_DIR)
	@touch $(BUILD_DIR)/cam_complete

# Target for disk images suitable for qemu-eos.  run_qemu.py
# knows to look in the output dir for the image files.
# Doesn't work from platform/, works for individual cams.
disk_image: $(BUILD_DIR)/magiclantern.zip | $(BUILD_DIR)
	../create_disk_image_from_zip.py $(BUILD_DIR)/magiclantern.zip

# If we try to build with multiple cams and modules is not up to date,
# the different cams can race to rebuild the same components.
# Because we generate source files during the build (yuck), modules can
# see empty files or partial content and fail to build.
# You're especially likely to hit this if modules has just been cleaned.

$(MODULES_DIR)/$(BUILD_DIR)/default_modules_complete: | $(BUILD_DIR)
	$(MAKE) -C $(MODULES_DIR)

# The static pattern rule that runs for every cam depends on this,
# to avoid them racing on modules directly, instead this touch
# target will (hopefully) make the races cheaper.
$(BUILD_DIR)/platform_modules_complete: $(MODULES_DIR)/$(BUILD_DIR)/default_modules_complete | $(BUILD_DIR)
	touch $(BUILD_DIR)/platform_modules_complete

# ML_PLATFORM_TARGETS is a string where each part is of the format:
# R.180/build/cam_complete, which is a marker file for that cam finishing the build.
# The following is a "static pattern rule", a bit like a for-each over
# the parts of the string.  $* expands out to R.180/build/cam_complete during
# the R.180 build, for example.
#
# It's very important that we have the prereq that triggers a module build.
# Each individual cam build will attempt to build modules.  This is a race.
# If the modules don't exist already, it will race while building them and often
# error.  If they race while they're finished, it's fine because there's
# nothing to do.
$(ML_PLATFORM_TARGETS): %: $(BUILD_DIR)/platform_modules_complete | $(BUILD_DIR)
	$(MAKE) -C $(subst /$(BUILD_DIR)/cam_complete,,$*)
	$(CP) $(subst /cam_complete,/magiclantern.zip,$*) $(BUILD_DIR)/magiclantern.$(BUILD_DATE).$(subst /build/cam_complete,,$*).zip

$(BUILD_DIR)/magiclantern.zip: zip_contents | $(BUILD_DIR)
	cd $(BUILD_DIR)/zip; zip -z ../../$@.tmp < ../../$(SRC_DIR)/zip.txt  -r .
	$(MV) $@.tmp $@

# Create the directory structure of the output dir
ifeq ($(IS_PLATFORM_LEVEL_BUILD), true)
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
else
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/tcc
	@mkdir -p $(BUILD_DIR)/modules
	@mkdir -p $(BUILD_DIR)/zip/ML/cropmks
	@mkdir -p $(BUILD_DIR)/zip/ML/data
	@mkdir -p $(BUILD_DIR)/zip/ML/doc
	@mkdir -p $(BUILD_DIR)/zip/ML/fonts
	@mkdir -p $(BUILD_DIR)/zip/ML/modules
	@mkdir -p $(BUILD_DIR)/zip/ML/scripts/lib
endif

# copy in files that are never rebuilt
$(ZIP_DIR): | $(BUILD_DIR)
	$(CP) $(TOP_DIR)/data/cropmks/*.bmp $(BUILD_DIR)/zip/ML/cropmks
	$(CP) $(TOP_DIR)/data/vram/*.lut $(BUILD_DIR)/zip/ML/data
	-$(CP) -v $(TOP_DIR)/doc/cam/* $(BUILD_DIR)/zip/ML/doc
	$(CP) $(TOP_DIR)/data/fonts/*.rbf $(BUILD_DIR)/zip/ML/fonts
	$(CP) $(TOP_DIR)/scripts/*.lua $(BUILD_DIR)/zip/ML/scripts
	$(CP) $(TOP_DIR)/scripts/lib/*.lua $(BUILD_DIR)/zip/ML/scripts/lib
	-$(CP) $(TOP_DIR)/doc/README.user $(BUILD_DIR)/zip/ML/README

zip_contents:: $(ZIP_DIR) $(BUILD_DIR)/autoexec.bin $(BUILD_DIR)/magiclantern.bin | $(BUILD_DIR)
	$(CP) $(BUILD_DIR)/autoexec.bin $(ZIP_DIR)/

ifeq ($(CONFIG_MODULES),y)
    ifneq ($(CONFIG_TCC),y)
        $(error CONFIG_MODULES requires CONFIG_TCC = y, please enable it)
    endif

CFLAGS += -DCONFIG_MODULES
ML_OBJS += $(BUILD_DIR)/module.o
ML_MODULES_SYM_NAME ?= $(MODEL)_$(FW_VERSION).sym
CFLAGS += -DCONFIG_MODULES_MODEL_SYM=\"$(ML_MODULES_SYM_NAME)\"

$(BUILD_DIR)/$(ML_MODULES_SYM_NAME): $(BUILD_DIR)/magiclantern.sym | $(BUILD_DIR)
	$(CP) $(BUILD_DIR)/magiclantern.sym $(BUILD_DIR)/$(ML_MODULES_SYM_NAME)

zip_contents:: $(BUILD_DIR)/$(ML_MODULES_SYM_NAME) \
               modules.hidden modules.included \
               $(MODULES_DIR)/$(BUILD_DIR)/default_modules_complete | $(BUILD_DIR)
	$(CP) $(BUILD_DIR)/$(ML_MODULES_SYM_NAME) $(ZIP_MODULES_DIR)/
	$(CP) $(MODULES_DIR)/$(BUILD_DIR)/*.mo $(CAM_MODULES_DIR)/
	$(PYTHON) $(PLATFORM_DIR)/create_hid_files.py modules.hidden $(ZIP_MODULES_DIR)
	$(PYTHON) $(PLATFORM_DIR)/copy_included_modules.py modules.included $(CAM_MODULES_DIR) $(ZIP_MODULES_DIR)
endif

# If installer/99D/ML-SETUP.FIR exists, use it, otherwise
# try and use the existing platform/99D/ML-SETUP.FIR
ifneq ("$(wildcard $(INSTALLER_DIR)/$(ML_MODEL_DIR)/$(INSTALLER_FIR))","")
zip_contents:: $(INSTALLER_DIR)/$(ML_MODEL_DIR)/$(INSTALLER_FIR) | $(BUILD_DIR)
	-$(CP) $(INSTALLER_DIR)/$(ML_MODEL_DIR)$(INSTALLER_FIR) $(ZIP_DIR)/

# FIXME enable the make command when we're ready to test installer builds;
# for now I'm working on platform builds
$(INSTALLER_DIR)/$(ML_MODEL_DIR)/$(INSTALLER_FIR): | $(BUILD_DIR)
#	$(MAKE) -C $(INSTALLER_DIR)/$(ML_MODEL_DIR) build_fir
else
# No installer for this cam; try and use the pre-built
# installer FIR; but failure is allowed since some cams
# don't have that, either.
zip_contents:: | $(BUILD_DIR)
	-$(CP) $(INSTALLER_FIR) $(ZIP_DIR)/
endif


ML_REBOOT_OBJ = $(BUILD_DIR)/reboot.o \
    $(BUILD_DIR)/disp_direct.o \
    $(BUILD_DIR)/font_direct.o \
    $(BUILD_DIR)/strlen.o \
    $(BUILD_DIR)/strcmp.o \
    $(BUILD_DIR)/gcc-libgcc.a \
    $(BUILD_DIR)/cache.o
# footer.o must be the last object linked in autoexec.bin,
# or xor_chk can fail due to unexpected global objects
# being placed later than footer_magic
ML_REBOOT_OBJ += $(BUILD_DIR)/footer.o

ML_LD_SCRIPT = $(BUILD_DIR)/magiclantern.lds
ML_OBJS += \
    $(BUILD_DIR)/stubs.o \
    $(BUILD_DIR)/version.o

ifeq ($(ML_ENTRY_OBJ), n)
    ML_ENTRY_OBJ =
else
    ifndef ML_ENTRY_OBJ
        # NB, entry.o must be the first object file when we link magiclantern binary,
        # so that copy_and_restart, the first function in entry.o, becomes
        # the first code in magiclantern.  We transfer control from autoexec
        # directly to this address.
        #
        # The linker invocation puts ML_ENTRY_OBJ in the correct place.
        ML_ENTRY_OBJ = $(BUILD_DIR)/entry.o
    endif
    ML_OBJS += $(ML_ENTRY_OBJ)
endif

ML_SRC_OBJS_00 = \
    task_utils.o \
    backtrace.o \
    log.o \
    dryos_rpc.o \
    sgi.o \
    cpu.o \
    mmu_utils.o \
    patch_mmu.o \
    patch_cache.o \
    patch_d6.o \
    $(ML_BOOT_OBJ) \
    fio-ml.o \
    mem.o \
    ico.o \
    edmac.o \
    menu.o \
    debug.o \
    rand.o \
    posix.o \
    util.o \
    imath.o \
    electronic_level.o \
    cfn.o \
    fps-engio_per_cam.o \
    gui.o \
    exmem.o \
    compositor.o \
    bmp.o \
    rbf_font.o \
    config.o \
    stdio.o \
    lcdsensor.o \
    tweaks.o \
    tweaks-eyefi.o \
    lens.o \
    property.o \
    propvalues.o \
    gui-common.o \
    chdk-gui_draw.o \
    movtweaks.o \
    menuhelp.o \
    menuindex.o \
    focus.o \
    notify_box.o \
    dialog_test.o \
    vram.o \
    greenscreen.o \
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
    beep.o \
    crop-mode-hack.o \
    ph_info_disp.o \
    flexinfo.o \
    screenshot.o \
    fileprefix.o \
    lvinfo.o \
    builtin-enforcing.o \
    powersave.o \
    ml-cbr.o \
    raw.o \
    chdk-dng.o \
    edmac-memcpy.o \
    sd.o \
    cache_hacks.o \
    patch.o \
    cache.o
ML_SRC_OBJS = $(addprefix $(BUILD_DIR)/, $(ML_SRC_OBJS_00))

# This block handles the optional object files,
# which get appended to the above variable.
#
# FIXME - is there a nicer way to handle this?
# The problem is that if e.g. ML_ZEBRA_OBJ is empty,
# we later try to compile "build/", a directory,
# which fails.  Having to do a conditional for each
# possible optional component is verbose and ugly.
ifeq ($(ML_INIT_OBJ), n)
    ML_INIT_OBJ =
else
    ifndef ML_INIT_OBJ
        ML_INIT_OBJ = init.o
    endif
    ML_SRC_OBJS += $(BUILD_DIR)/$(ML_INIT_OBJ)
endif

ifeq ($(ML_PICSTYLE_AUTO), n)
    ML_PICSTYLE_OBJ = picstyle-noauto.o
else
    ML_PICSTYLE_OBJ = picstyle.o
endif
ML_SRC_OBJS += $(BUILD_DIR)/$(ML_PICSTYLE_OBJ)

ifeq ($(ML_BITRATE_OBJ), n)
    ML_BITRATE_OBJ =
else
    ifndef ML_BITRATE_OBJ
        ML_BITRATE_OBJ = bitrate.o
    endif
    ML_SRC_OBJS += $(BUILD_DIR)/$(ML_BITRATE_OBJ)
endif

ifeq ($(ML_AF_PATTERNS_OBJ), n)
    ML_AF_PATTERNS_OBJ =
else
    ifndef ML_AF_PATTERNS_OBJ
        ML_AF_PATTERNS_OBJ = af_patterns.o
    endif
    ML_SRC_OBJS += $(BUILD_DIR)/$(ML_AF_PATTERNS_OBJ)
endif

ifeq ($(ML_BOOTFLAGS_OBJ), n)
    ML_BOOTFLAGS_OBJ =
else
    ifndef ML_BOOTFLAGS_OBJ
        ML_BOOTFLAGS_OBJ = bootflags.o
    endif
    ML_SRC_OBJS += $(BUILD_DIR)/$(ML_BOOTFLAGS_OBJ)
endif

ifeq ($(ML_AUDIO_OBJ), n)
    ML_AUDIO_OBJ =
else
    ifndef ML_AUDIO_OBJ
        ML_AUDIO_OBJ = audio-ak.o
    endif
    ML_SRC_OBJS += $(BUILD_DIR)/$(ML_AUDIO_OBJ)
endif

ifeq ($(ML_ZEBRA_OBJ), n)
    ML_ZEBRA_OBJ =
else
    ifndef ML_ZEBRA_OBJ
        ML_ZEBRA_OBJ = zebra.o vectorscope.o
    endif
    ML_SRC_OBJS += $(BUILD_DIR)/zebra.o
    ML_SRC_OBJS += $(BUILD_DIR)/vectorscope.o
endif

ML_OBJS += $(ML_SRC_OBJS)
ML_OBJS += $(ML_SRC_EXTRA_OBJS)

# FIXME:
# List of files that we will extract from dietlibc archive.
# Note there is a nasty hack hiding here.  It looks like we're
# prefixing each archive member name with the directory it will
# be extracted to.  But that's a lie: bizarrely, ar ignores this,
# but still succeeds in the extraction.
#
# The prefix is there so Make realises build/strlen.o exists,
# so that it can succeed in building anything dependent on it.
# Yuck.  Surely there is a better way of indicating that?
# subst maybe, start with the bare list, append BUILD_DIR to each
# item, store in two different vars for the different purposes?
DIET_OBJS = \
    $(BUILD_DIR)/strrchr.o \
    $(BUILD_DIR)/strncpy.o \
    $(BUILD_DIR)/strlen.o \
    $(BUILD_DIR)/strncmp.o \
    $(BUILD_DIR)/strcpy.o \
    $(BUILD_DIR)/strcmp.o \
    $(BUILD_DIR)/strcasecmp.o \
    $(BUILD_DIR)/errno.o \
    $(BUILD_DIR)/errno_location.o \
    $(BUILD_DIR)/strtoull.o \
    $(BUILD_DIR)/strtol.o \
    $(BUILD_DIR)/strtoul.o \
    $(BUILD_DIR)/strchr.o \
    $(BUILD_DIR)/strstr.o \
    $(BUILD_DIR)/memcmp.o \
    $(BUILD_DIR)/memmove.o \
    $(BUILD_DIR)/atoi.o \
    $(BUILD_DIR)/atol.o

$(DIET_OBJS): $(DIETLIBC_A) | $(BUILD_DIR)
	$(AR) x $? $(DIET_OBJS) --output $(BUILD_DIR)

$(BUILD_DIR)/dietlibc.a: $(DIET_OBJS) | $(BUILD_DIR)
	$(AR) cr $@ $^
CFLAGS += $(CFLAGS_PLATFORM)

# Now a list of files we get from newlib.  Apparently the
# dietlibc memcpy/memset caused hangs.  And newlib is faster.
#
# Not checked if that is still true.  If so, should we use
# more things from newlib?
NEWLIB_OBJS = \
    $(BUILD_DIR)/lib_a-setjmp.o \
    $(BUILD_DIR)/lib_a-memcpy.o \
    $(BUILD_DIR)/lib_a-memcpy-stub.o \
    $(BUILD_DIR)/lib_a-memccpy.o \
    $(BUILD_DIR)/lib_a-memset.o \
    $(BUILD_DIR)/lib_a-memmove.o

NEWLIB_PATH=$(SRC_DIR)/libs/arm-none-eabi-O3
NEWLIB_LIBM_A=$(NEWLIB_PATH)/libm.a
NEWLIB_LIBC_A=$(NEWLIB_PATH)/libc.a
$(NEWLIB_OBJS): $(NEWLIB_LIBC_A) | $(BUILD_DIR)
	$(AR) x $? $(NEWLIB_OBJS) --output $(BUILD_DIR)

$(BUILD_DIR)/newlib-libc.a: $(NEWLIB_OBJS) | $(BUILD_DIR)
	$(AR) cr $@ $^

$(BUILD_DIR)/newlib-libm.a: $(NEWLIB_LIBM_A) | $(BUILD_DIR)
	$(CP) $^ $@

ARM_LIBGCC_A=$(shell $(CC) -print-libgcc-file-name)
$(BUILD_DIR)/gcc-libgcc.a: $(ARM_LIBGCC_A) | $(BUILD_DIR)
	$(CP) $^ $@

$(BUILD_DIR)/version.o: $(BUILD_DIR)/version.c | $(BUILD_DIR)

LFLAGS = \
        -u strcasecmp \
        -u memmove \
        -u strstr \
        -u strtof \
        -u log10f \
        -u log2f \
        -u localeconv \
        -u asinf \
        -u strerror \
        -u strpbrk \
        -u ceilf \
        -u tanf \
        -u strspn \
        -u fmodf \
        -u acosf \
        -u atan2f \
        -u expf \
        -u memchr \
        -u frexpf

# Force including various arithmetic funcs that used to be provided
# via the unpleasant hack in builtin-enforcing.c.
# This ensures they're exported for modules even when ML doesn't
# use them.
LFLAGS += -u __aeabi_f2ulz -u __aeabi_f2lz

# Force including software division library code even on ARMv7-R, which
# always has hardware division.  Modules need these since they're
# built for lowest-common-denominator ARMv5.  -R magiclantern still
# uses efficient hw division, it just includes the code for sw div.
LFLAGS += -u __aeabi_idiv -u __aeabi_idivmod

ML_LIBC = $(BUILD_DIR)/dietlibc.a \
          $(BUILD_DIR)/newlib-libc.a \
          $(BUILD_DIR)/newlib-libm.a \
          $(BUILD_DIR)/gcc-libgcc.a
AFLAGS =
INCLUDE_ARGS = -I. -Iinclude -Ibuild -I$(SRC_DIR)

# FIXME can we have the versioned zip depend on magiclantern.zip
# and something like the tcc_status trick?
# So it always updates the name with a copy, but doesn't need to change the
# content if a rebuild wasn't required?
#$(BUILD_DIR)/magiclantern-$(VERSION).zip: $(BUILD_DIR)/magiclantern.zip
#	$(CP) $< $@

# if there is a .c file in the cam dir with the same name
# as in src/, this rule being first means we'll use the
# cam one.
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -c -o $@ $< $(INCLUDE_ARGS) $(LFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -c -o $@ $< $(INCLUDE_ARGS) $(LFLAGS)

$(BUILD_DIR)/reboot.o: $(SRC_DIR)/reboot.c $(BUILD_DIR)/version.bin $(BUILD_DIR)/magiclantern.bin | $(BUILD_DIR)

# include ML version, camera, firmware, some git info, build date
# (this will need moving / updating for universal autoexec,
# where per cam info makes no sense).
#
# TODO: could add the -DCONFIG flags,
# could add if git was dirty or not.
#
# FIXME: stop using FORCE target.  This should be something like
# "build if there are any changes to source since the last build,
# or build if we can't tell (e.g. no version.bin from last build?)"
# Could use something like the tcc_status target?
$(BUILD_DIR)/version.bin: FORCE | $(BUILD_DIR)
	@echo '' > $@.tmp
	@echo '' >> $@.tmp
	@echo 'Magic Lantern $(VERSION)' >> $@.tmp
	@echo 'Camera   : $(MODEL)' >> $@.tmp
	@echo 'Firmware : $(FW_VERSION)' >> $@.tmp
	@printf 'Commit: ' >> $@.tmp
	$(BUILD_TOOLS_DIR)/git_summary.py >> $@.tmp
	@echo '' >> $@.tmp
	@echo 'Built on : '`date -u "+%Y-%m-%d %H:%M:%S"`' by '`whoami`@`hostname` >> $@.tmp
	@echo '' >> $@.tmp
	$(MV) $@.tmp $@

$(BUILD_DIR)/version.c: FORCE | $(BUILD_DIR)
	@echo "const char build_version[] = \"$(VERSION)\";" > $(BUILD_DIR)/version.tmp
	@printf 'const char build_id[] = "' >> $(BUILD_DIR)/version.tmp
	$(BUILD_TOOLS_DIR)/git_summary.py >> $(BUILD_DIR)/version.tmp
	@echo '";' >> $(BUILD_DIR)/version.tmp
	@echo 'const char build_date[] ="'`$(DATE) "+%Y-%m-%d %H:%M:%S %Z"`'";' >> $(BUILD_DIR)/version.tmp
	@echo 'const char build_user[] = "'`whoami`@`hostname`'";'  >> $(BUILD_DIR)/version.tmp
	mv $(BUILD_DIR)/version.tmp $@

$(BUILD_DIR)/xor_chk: $(BUILD_TOOLS_DIR)/xor_chk.c | $(BUILD_DIR)
	$(HOST_CC) $< -o $@

$(BUILD_DIR)/stubs.o: stubs.S | $(BUILD_DIR)
	$(CC) $(AFLAGS) -I../../include/ -c -o $@ $<

$(BUILD_DIR)/entry.o: $(SRC_DIR)/entry.S | $(BUILD_DIR)
	$(CC) $(AFLAGS) -I../../include/ -c -o $@ $<

$(BUILD_DIR)/magiclantern.lds: $(SRC_DIR)/magiclantern.lds.S | $(BUILD_DIR)
	$(CC) -E $(CFLAGS) $< | $(GREP) -v '^#' > $(BUILD_DIR)/magiclantern.lds.tmp
	$(MV) $(BUILD_DIR)/magiclantern.lds.tmp $@

$(BUILD_DIR)/magiclantern: $(ML_LD_SCRIPT) $(ML_ENTRY_OBJ) $(ML_OBJS) $(ML_LIBC) | $(BUILD_DIR)
	$(LD) -o $@ -N $(LFLAGS) -T $^ -Map=$(BUILD_DIR)/location.map

$(BUILD_DIR)/magiclantern.bin: $(BUILD_DIR)/magiclantern | $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@
	$(STAT_CMD) $@

$(BUILD_DIR)/autoexec: $(ML_REBOOT_OBJ) | $(BUILD_DIR)
	$(LD) -o $@ -e _start -N -Ttext $(AUTOEXEC_BASE) $^ --cref -Map=$(BUILD_DIR)/autoexec.map

$(BUILD_DIR)/autoexec.bin: $(BUILD_DIR)/autoexec $(BUILD_DIR)/xor_chk | $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@; \
	$(BUILD_DIR)/xor_chk $@

$(BUILD_DIR)/%.sym: $(BUILD_DIR)/% | $(BUILD_DIR)
	$(READELF) -sW $< | $(GREP) GLOBAL | $(GREP) -v " UND " | $(GREP) -E 'FUNC|OBJECT|ABS|NOTYPE' | $(AWK) "{print \$$2 \" \" \$$8;}" | $(GREP) -v '^ ' | $(GREP) -v __config_ | $(GREP) -v -E ' _[a-zA-Z]' | sort > $@.tmp
	$(MV) $@.tmp $@

# copy tcc source into our build dir to avoid other things clobbering
# tcc or partially building it while we're building, etc,
# as well as trivialise cleanup.
$(BUILD_DIR)/tcc/libtccx.o: $(BUILD_DIR)/tcc_status | $(BUILD_DIR)
	rm -rf -- "$(BUILD_DIR)/tcc"; \
	$(CP) -r ../../tcc $(BUILD_DIR); \
	$(MAKE) -C $(BUILD_DIR)/tcc clean; \
	$(MAKE) -C $(BUILD_DIR)/tcc

# We should rebuild tcc if anything in the dir changes, so record
# the timestamp of all source files to a file that we control,
# use cmp to check against the prior runs record.
$(BUILD_DIR)/tcc_status: FORCE | $(BUILD_DIR)
	@ls -lR ../../tcc | grep -e "\.c$$" -e "\.h$$" -e "\.S$$" -e "configure" -e "Makefile" -e "config.mak" | cmp -s - $@ \
	|| ls -lR ../../tcc | grep -e "\.c$$" -e "\.h$$" -e "\.S$$" -e "configure" -e "Makefile" -e "config.mak" > $@

FORCE:

ifeq ($(strip $(BUILD_DIR)),)
    $(error platform BUILD_DIR was empty!  Avoiding potentially dangerous clean target)
else
    ifeq ($(IS_PLATFORM_LEVEL_BUILD), true)
        # "clean" in platform/ triggers clean of platform/build
        # and all default platform/cam/build
        ML_PLATFORM_BUILD_DIRS = $(addsuffix /$(BUILD_DIR),$(ML_CAMS))
clean:
	@echo Cleaning platforms: $(ML_PLATFORM_BUILD_DIRS)
	-@rm -rf -- $(BUILD_DIR)
	-@rm -rf -- $(ML_PLATFORM_BUILD_DIRS)

    else
        # last part of working dir not "platform",
        # we're cleaning a single cam
clean:
	@rm -rf -- $(BUILD_DIR) ||:

    endif
endif
