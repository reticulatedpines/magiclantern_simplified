TOP_DIR?=..

ML_SETUP_EXTENSION=modules

# We force arch to old Digic, so that modules are built as ARM, never Thumb,
# since modules are supposed to be binary portable to other cams.  Currently,
# this is not true, since modules sometimes make assumptions that are only
# true for Digic 4 & 5 (e.g., calling icache_unlock() - D678X doesn't have
# cache locking).
PLATFORM_ARCH = armv5te

include $(TOP_DIR)/Makefile.setup

ifdef ML_MODULES_DYNAMIC
$(error ML_MODULES_DYNAMIC was renamed to ML_MODULES)
endif

MODULES_ALL_TARGETS = $(ML_MODULES)

MODULES_CLEAN_ALL_TARGETS = $(call targets_rules, $(ML_MODULES), clean)

MODULES_INSTALL_ALL_TARGETS = $(call targets_rules, $(ML_MODULES), install)

MODULES_ALL_TARGETS_DELETE_IF_FAILED = $(call targets_rules, $(ML_MODULES), delete_if_failed)

all:
	@+$(MAKE) --silent all_silent

all_silent: $(MODULES_ALL_TARGETS_DELETE_IF_FAILED)

# run module-specific clean rules
# also run a generic clean, just in case there are any modules left over
clean: $(MODULES_CLEAN_ALL_TARGETS)
	$(call rm_files, $(addprefix $(MODULES_DIR)/, */*.mo */*.o */*.d */*.dep */*.sym */module_strings.h))

install: | prepare_install_dir
install: all
	$(PYTHON3) copy_modules_with_satisfied_deps.py $(INSTALL_MODULES_DIR) $(ML_MODULES)

$(MODULES_INSTALL_ALL_TARGETS) $(MODULES_CLEAN_ALL_TARGETS) $(MODULES_ALL_TARGETS) $(MODULES_ALL_TARGETS_DELETE_IF_FAILED):
	$(call call_make_rule, $(MODULES_DIR))

.PHONY: $(MODULES_ALL_TARGETS)
