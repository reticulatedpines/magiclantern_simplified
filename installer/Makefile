# This makefile can be used directly, to build zips for all the default enabled cams,
# or invoked by installer/99D/Makefile to build one for one cam (including non-default).
#
# Note that there is significant code duplication (at the Make level) between
# this installer/Makefile and platform/Makefile.
# This greatly reduces complexity of the Make code at the expense of higher
# maintenance cost (but we very rarely change the build system).
# If you find bugs here, you should check if they exist there, too.

TOP_DIR ?= ..

SRC_DIR = $(TOP_DIR)/src
BUILD_DIR = build
PLATFORM_DIR = $(TOP_DIR)/platform
INSTALLER_DIR = $(TOP_DIR)/installer

# Disable early CONFIG_* settings for installer builds,
# we never want these features there.
CONFIG_PTP      = n
CONFIG_PTP_CHDK = n
CONFIG_PTP_ML   = n
CONFIG_GDB      = n
CONFIG_GDBSTUB  = n
CONFIG_CONSOLE  = n
CONFIG_DEBUGMSG = 0
CONFIG_CCACHE   = n
CONFIG_TCC      = n
CONFIG_MODULES  = n
CONFIG_TINYPY   = n
CONFIG_QEMU     = n
CONFIG_PIC      = n

include $(TOP_DIR)/Makefile.globals

CFLAGS += \
    -march=$(PLATFORM_ARCH) \
    -I$(PLATFORM_DIR)/$(MODEL_DIR) \
    -I$(PLATFORM_DIR)/$(MODEL_DIR)/include
CFLAGS += \
    -DCONFIG_$(MODEL)=1 \
    -DCONFIG_$(MODEL)_$(FW_VERSION)=1 \
    -DCONFIG_INSTALLER \
    -DRESTARTSTART=$(RESTARTSTART) \
    -DMAIN_FIRMWARE_ADDR=$(MAIN_FIRMWARE_ADDR) \
    -DCAMERA_MODEL=\"$(MODEL)\" \
    -DCONFIG_FW_VERSION=$(FW_VERSION) \
    -DCURRENT_CAMERA_SIGNATURE=SIG_$(MODEL)_$(FW_VERSION) \
    -DVERSION=\"$(VERSION)\"
CFLAGS += $(CFLAGS_PLATFORM)

VERSION = $(call eval_once,VERSION,Nightly.$(shell LC_TIME=EN date +'%Y%b%d').$(MODEL)$(FW_VERSION))


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
    600D.102 \
    60D.111 \
    650D.104 \
    6D.116 \
    6D2.111 \
    700D.115 \
    70D.112 \
    750D.110 \
    7D2.112 \
    EOSM.202

.PHONY: clean $(BUILD_DIR) FORCE

# Construct targets for the default installers
#
# 600D.102 -> 600D.102/build/ML-SETUP.FIR
ML_INSTALLER_TARGETS = $(addsuffix /$(BUILD_DIR)/$(INSTALLER_FIR), $(ML_CAMS))

ifeq ($(notdir $(shell pwd)),$(notdir $(INSTALLER_DIR)))
    # working dir is "installer", set a flag so we
    # can do variants of clean, build locations, etc.
    IS_INSTALLER_LEVEL_BUILD = true
endif

ifeq ($(IS_INSTALLER_LEVEL_BUILD), true)
    # If the last part of the working dir is "installer",
    # we can do actions here that make sense for that context
    # but not for individual cams.
else
    # If we're not running from installer/, we assume
    # we're running from installer/some_cam/ and
    # do the things we need for building one cam.
    ML_BUILD_FIR_DIR = ../../../dumper
    ML_FIR_DIR = $(ML_BUILD_FIR_DIR)/fir
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
    ifndef CANON_FIR
        $(error CANON_FIR must be defined (e.g., 5D300123.FIR))
    endif
endif

# This rule is the default, but should run only if we're building
# all installers from ./installer
$(BUILD_DIR)/default_installers_complete: $(ML_INSTALLER_TARGETS) | $(BUILD_DIR)
	@touch $(BUILD_DIR)/default_installers_complete
	@echo Default installers build complete, success.

# ML_INSTALLER_TARGETS is a string where each part is of the format:
# 600D.102/build/ML-SETUP.FIR
# The following is a "static pattern rule", a bit like a for-each over
# the parts of the string.  $* expands out to 600D.102/build/ML-SETUP.FIR during
# the 600D.102 build, for example.
#
# Here we remove the suffix, to trigger make -C on each installer subdir;
# that is, perform the default make action for each cam's installer (ML-SETUP.FIR)
$(ML_INSTALLER_TARGETS): %: | $(BUILD_DIR)
	$(MAKE) -C $(subst /$(BUILD_DIR)/$(INSTALLER_FIR),,$*)

ifdef DUAL_DIGIC
# These cams need two binaries in the FIR, one per Digic.
OPTIONAL_SECOND_UPDATER_FILE = $(BUILD_DIR)/second_updater.bin
OPTIONAL_SECOND_UPDATER_CMD = -m $(OPTIONAL_SECOND_UPDATER_FILE)

$(BUILD_DIR)/second_updater.o: second_updater.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -DMAIN_FIRMWARE_ADDR=$(MAIN_FIRMWARE_ADDR) \
    -c -o $@ $< $(INCLUDE_ARGS) $(LFLAGS)

$(BUILD_DIR)/second_updater: $(BUILD_DIR)/second_updater.o | $(BUILD_DIR)
	$(LD) -o $@ -nostdlib -e _start -Ttext $(FIR_BASE) $^

$(BUILD_DIR)/second_updater.bin: $(BUILD_DIR)/second_updater $(BUILD_DIR)/xor_chk | $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@; \
    $(STAT_CMD) $@

endif

# Individual cams should make this target their default installer goal.
# It's not run when invoking make from the installer dir directly.
$(BUILD_DIR)/$(INSTALLER_FIR): $(BUILD_DIR)/autoexec.bin $(OPTIONAL_SECOND_UPDATER_FILE) | $(BUILD_DIR)
	python3 $(ML_BUILD_FIR_DIR)/build_fir.py -r \
    $(OPTIONAL_SECOND_UPDATER_CMD) \
    -s $(BUILD_DIR)/autoexec.bin \
    $(ML_FIR_DIR)/$(CANON_FIR) \
    $(BUILD_DIR)/$(INSTALLER_FIR)

# Create the output dir
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

ifeq ($(CONFIG_MODULES),y)
    $(error CONFIG_MODULES should not be set for an installer build)
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
    magiclantern.lds \
    entry.o \
    cache.o \
    $(ML_BOOT_OBJ) \
    installer.o \
    version.o \
    fio-ml.o \
    ico.o \
    bmp.o \
    rbf_font.o \
    stdio.o \
    dialog_test.o \
    bootflags.o \
    mem.o \
    util.o

ML_SRC_OBJS = $(addprefix $(BUILD_DIR)/, $(ML_SRC_OBJS_00))

# This block handles the optional object files,
# which get appended to the above variable.
ifeq ($(ML_BOOTFLAGS_OBJ), n)
    ML_BOOTFLAGS_OBJ =
else
    ifndef ML_BOOTFLAGS_OBJ
        ML_BOOTFLAGS_OBJ = bootflags.o
    endif
    ML_SRC_OBJS += $(BUILD_DIR)/$(ML_BOOTFLAGS_OBJ)
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

# if there is a .c file in the cam dir with the same name
# as in src/, this rule being first means we'll use the
# cam one.
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -c -o $@ $< $(INCLUDE_ARGS) $(LFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -c -o $@ $< $(INCLUDE_ARGS) $(LFLAGS)

$(BUILD_DIR)/function_overrides.o: $(PLATFORM_DIR)/$(MODEL_DIR)/function_overrides.c | $(BUILD_DIR)
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

$(BUILD_DIR)/stubs.o: $(PLATFORM_DIR)/$(MODEL_DIR)/stubs.S | $(BUILD_DIR)
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
	$(LD) -o $@ -e _start -N -Ttext $(FIR_BASE) $^ --cref -Map=$(BUILD_DIR)/autoexec.map

$(BUILD_DIR)/autoexec.bin: $(BUILD_DIR)/autoexec $(BUILD_DIR)/xor_chk | $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@; \
	$(BUILD_DIR)/xor_chk $@

FORCE:

ifeq ($(strip $(BUILD_DIR)),)
    $(error installer BUILD_DIR was empty!  Avoiding potentially dangerous clean target)
else
    ifeq ($(IS_INSTALLER_LEVEL_BUILD), true)
        # "clean" in installer/ triggers clean of all default installer/cam/build
        ML_INSTALLER_BUILD_DIRS = $(addsuffix /$(BUILD_DIR),$(ML_CAMS))
clean:
	@echo Cleaning installers: $(ML_INSTALLER_BUILD_DIRS)
	-@rm -rf -- $(BUILD_DIR)
	-@rm -rf -- $(ML_INSTALLER_BUILD_DIRS)

    else
        # last part of working dir not "installer",
        # we're cleaning a single cam
clean:
	@rm -rf -- $(BUILD_DIR) ||:

    endif
endif
