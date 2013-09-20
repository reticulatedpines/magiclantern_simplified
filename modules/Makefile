TOP_DIR?=..

ML_SETUP_EXTENSION=modules

include $(TOP_DIR)/Makefile.setup

MODULES_ALL_TARGETS = $(ML_MODULES_DYNAMIC) $(ML_MODULES_STATIC)

MODULES_CLEAN_ALL_TARGETS = $(call targets_rules, $(shell ls -d */), clean)

MODULES_INSTALL_DYNAMIC_TARGETS = $(call targets_rules, $(ML_MODULES_DYNAMIC), install)

MODULES_ALL_TARGETS_DELETE_IF_FAILED = $(call targets_rules, $(MODULES_ALL_TARGETS), delete_if_failed)

all:
	@make --silent all_silent

all_silent: $(MODULES_ALL_TARGETS_DELETE_IF_FAILED)

clean: $(MODULES_CLEAN_ALL_TARGETS)

prepare_install: all
	mkdir -p $(INSTALL_MODULES_DIR) || echo "no problem"

install: | prepare_install
install: all
	$(CP) $(MODULES_DIR)/*/*.mo $(INSTALL_MODULES_DIR)/ || echo "no problem"

$(MODULES_INSTALL_DYNAMIC_TARGETS) $(MODULES_CLEAN_ALL_TARGETS) $(ML_MODULES_DYNAMIC) $(MODULES_ALL_TARGETS_DELETE_IF_FAILED):
	$(call call_make_rule, $(MODULES_DIR))

.PHONY: $(MODULES_ALL_TARGETS)
