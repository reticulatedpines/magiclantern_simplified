# Makefile for modules.  ML modules are cam and architecture independent,
# the produced binaries should be safe to copy to any cam.
# At runtime, TCC attempts to load the module, and will refuse if
# it can't resolve extern symbol dependencies.
#
# WARNING: the above is theory.  In practice, devs have not behaved.
# It's fairly common that modules hard-code addresses that only
# work on some cams.  You must audit the code before enabling a module
# for a new port in progress.

# This makefile can be used directly, to build all modules,
# or invoked by modules/some_module/Makefile to build one module.

# Modules are built as ARMv5TE, which we expect to be supported by all
# current cams (ARMv5TE, ARMv7-A, ARMv7-R, ARMv7-R).
#
# It is still possible for module code to attempt to call things not defined
# on a given cam.  In theory this gets managed at runtime; TCC loader
# notices the unsatisfied dependency and refuses to load the module.
MODULE_ARCH = armv5te

TOP_DIR ?= ..
SRC_DIR = $(TOP_DIR)/src
BUILD_DIR = build
MODULES_DIR = $(TOP_DIR)/modules

include $(TOP_DIR)/Makefile.globals
include $(DEPS)

CFLAGS += -march=$(MODULE_ARCH)

# List of modules that are built by default, including
# when building a zip for a cam.
ML_MODULES = \
    raw_video/mlv_lite \
    raw_video/mlv_play \
    raw_video/mlv_rec \
    raw_video/mlv_snd \
    file_man \
    pic_view \
    ettr \
    dual_iso \
    silent \
    dot_tune \
    autoexpo \
    arkanoid \
    deflick \
    img_name \
    lua \
    bench \
    selftest \
    adv_int \
    crop_rec \
    dev_tools/edmac \
    yolo

.PHONY: clean $(BUILD_DIR)

# Construct various targets for the default modules.
#
# lua -> lua/build/module_complete
ML_MODULE_TARGETS = $(addsuffix /$(BUILD_DIR)/module_complete, $(ML_MODULES))
#
# lua -> lua/build/lua.mo
ML_MODULE_PATHS = $(foreach MOD_PATH,$(ML_MODULES),$(MOD_PATH)/$(BUILD_DIR)/$(notdir $(MOD_PATH)).mo)

WORKING_DIR = $(shell pwd)
ifeq ($(notdir $(WORKING_DIR)),$(notdir $(MODULES_DIR)))
    # If the last part of the working dir is "modules",
    # we do some actions here that make sense for that context
    # but not for individual modules.

    # Remove all module completion marker files.
    # This forces $(BUILD_DIR)/module_complete rule to run,
    # via make -C for every module, but this is very fast if
    # no changes have occured to the module.  The marker file
    # depends on all relevant parts of the module but is
    # created by "touch".
$(shell rm $(ML_MODULE_TARGETS) > /dev/null 2>&1)
endif

# Default target if make run in "modules" dir
#$(BUILD_DIR)/default_modules_complete: $(ML_MODULE_TARGETS) $(ML_MODULE_PATHS) | $(BUILD_DIR)
$(BUILD_DIR)/default_modules_complete: $(ML_MODULE_TARGETS) | $(BUILD_DIR)
	@$(PYTHON3) mark_cross_module_deps.py $(OBJCOPY) $(ML_MODULES)
	@$(CP) $(ML_MODULE_PATHS) $(BUILD_DIR)/
	@touch $(BUILD_DIR)/default_modules_complete
	@echo Modules built successfully

# Individual modules should make this target their default goal.
# It's not run when invoking make from the modules dir directly,
# as e.g. MODULES_OBJS wouldn't be defined.
$(BUILD_DIR)/module_complete: $(BUILD_DIR)/$(MODULE_NAME).sym \
                              $(BUILD_DIR)/$(MODULE_NAME).dep \
                              $(BUILD_DIR)/module_strings.h \
                              $(BUILD_DIR)/README.rst \
                              $(MODULE_OBJS) \
                              $(BUILD_DIR)/$(MODULE_NAME).mo \
                              | $(BUILD_DIR)
	@touch $(BUILD_DIR)/module_complete

# This is the rule for e.g. lua/build/lua.mo, which is distinct
# from the rule for build/lua.mo - even though both are referring to
# the same file.
#
# Make works on paths, and the path is different when triggering
# from modules dir.  This rule therefore won't match anything when
# run from e.g. modules/lua, but will match in modules/, where
# we use it to trigger a build for every module.
#
# ML_MODULE_PATHS format: lua/build/lua.mo
#$(ML_MODULE_PATHS): %: | $(BUILD_DIR)
#	$(MAKE) -C $(firstword $(subst /, ,$*))

# The CP string mangling turns e.g.
# lua/build/module_complete -> lua/build/lua.mo build/
#
# ML_MODULE_TARGETS format: lua/build/module_complete
$(ML_MODULE_TARGETS): %: | $(BUILD_DIR)
	$(MAKE) -C $(subst /$(BUILD_DIR)/module_complete,,$*)
	@$(CP) $(subst /module_complete,,$*)/$(notdir $(subst /$(BUILD_DIR)/module_complete,,$*)).mo $(BUILD_DIR)/

$(BUILD_DIR)/$(MODULE_NAME).mo: $(MODULE_OBJS) | $(BUILD_DIR)
	$(LD) -r -o $@.tmp $(MODULE_OBJS)
	$(STRIP) --strip-debug --strip-unneeded --discard-all --discard-locals $@.tmp
	$(MV) $@.tmp $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -c -o $@ $<

# Targets for "clean"
ifeq ($(strip $(BUILD_DIR)),)
    $(error BUILD_DIR was empty!  Avoiding potentially dangerous clean target)
else
    ifeq ($(notdir $(WORKING_DIR)),$(notdir $(MODULES_DIR)))
        # if the last part of the working dir is "modules",
        # we want to clean all modules
        ML_MODULE_BUILD_DIRS = $(addsuffix /$(BUILD_DIR),$(ML_MODULES))
clean:
	@echo Cleaning: $(ML_MODULE_BUILD_DIRS)
	-@rm -rf -- $(ML_MODULE_BUILD_DIRS)
	-@rm -rf -- $(BUILD_DIR)
	@mkdir $(BUILD_DIR)

    else
        # last part of working dir not "modules",
        # we want to clean a single module
clean:
	@rm -rf -- $(BUILD_DIR) ||:
	@mkdir $(BUILD_DIR)

    endif
endif

# SJE TODO - should we do -mlong-calls for all cams? It has some space and runtime cost,
# though this is probably quite low.  It's required for cams where the addresses where
# modules will be loaded (heap) are further than 32MB from ML or DryOS code.  E.g. on
# 200D, heap tends to be 0xbb_0000 and stubs are 0xe000_0000 or 0xdf00_0000, too far.
MODULE_CFLAGS += -I../../src/
CFLAGS += -mlong-calls -DMODULE -DMODULE_NAME=$(MODULE_NAME) -DCONFIG_MAGICLANTERN $(MODULE_CFLAGS) -I.

$(BUILD_DIR)/module_strings.h: $(BUILD_DIR)/README.rst | $(BUILD_DIR)
	$(PYTHON) $(MODULES_DIR)/readme2modulestrings.py > $@.tmp
	$(MV) $@.tmp $@

# Check for non-existence of modules/module_name/README.rst,
# use default content.  If exists, copy file instead.
ifeq (,$(wildcard README.rst))
$(BUILD_DIR)/README.rst: | $(BUILD_DIR)
	@echo "$(MODULE_NAME)\n========\n\nPlease write a one-page description for your module here.\n\nYou may also add some metadata:\n\n:Author: <your name here>\n:License: <your license here>\n:Website: <your website here>\n\n" > $@.tmp
	$(MV) $@.tmp $@
else
$(BUILD_DIR)/README.rst: README.rst | $(BUILD_DIR)
	$(CP) README.rst $@
endif

$(MODULE_OBJS): $(BUILD_DIR)/module_strings.h | $(BUILD_DIR)

$(BUILD_DIR)/$(MODULE_NAME).sym: $(BUILD_DIR)/$(MODULE_NAME).mo | $(BUILD_DIR)
	$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$2 \" \" \$$8;}" | sort > $@.tmp && cat $@.tmp
	$(MV) $@.tmp $@
$(BUILD_DIR)/$(MODULE_NAME).dep: $(BUILD_DIR)/$(MODULE_NAME).mo | $(BUILD_DIR)
	$(READELF) -sW $< | $(GREP) GLOBAL | $(GREP) 00000000 | $(GREP) UND | $(AWK) "{print \$$8;}" | sort > $@.tmp
	$(MV) $@.tmp $@

