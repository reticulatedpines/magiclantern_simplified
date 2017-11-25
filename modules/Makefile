TOP_DIR?=..

ML_SETUP_EXTENSION=modules

include $(TOP_DIR)/Makefile.setup

ifdef ML_MODULES_DYNAMIC
$(error ML_MODULES_DYNAMIC was renamed to ML_MODULES)
endif

MODULES_ALL_TARGETS = $(ML_MODULES)

MODULES_CLEAN_ALL_TARGETS = $(call targets_rules, $(ML_MODULES), clean)

MODULES_INSTALL_ALL_TARGETS = $(call targets_rules, $(ML_MODULES), install)

MODULES_ALL_TARGETS_DELETE_IF_FAILED = $(call targets_rules, $(ML_MODULES), delete_if_failed)

all:
	@make --silent all_silent

all_silent: $(MODULES_ALL_TARGETS_DELETE_IF_FAILED)

# run module-specific clean rules
# also run a generic clean, just in case there are any modules left over
clean: $(MODULES_CLEAN_ALL_TARGETS)
	$(call rm_files, $(addprefix $(MODULES_DIR)/, *.o *.d *.dep *.sym hgstamp))

install: | prepare_install_dir
install: all
	$(CP) $(MODULES_DIR)/*/*.mo $(INSTALL_MODULES_DIR)/ || echo "no problem"

$(MODULES_INSTALL_ALL_TARGETS) $(MODULES_CLEAN_ALL_TARGETS) $(MODULES_ALL_TARGETS) $(MODULES_ALL_TARGETS_DELETE_IF_FAILED):
	$(call call_make_rule, $(MODULES_DIR))

.PHONY: $(MODULES_ALL_TARGETS)
