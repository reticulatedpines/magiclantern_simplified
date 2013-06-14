TOP_DIR?=..

ML_SETUP_EXTENSION=modules

include $(TOP_DIR)/Makefile.setup

MODULES_ALL_TARGETS = $(ML_MODULES_DYNAMIC) $(ML_MODULES_STATIC)

MODULES_CLEAN_ALL_TARGETS = $(call targets_rules, $(MODULES_ALL_TARGETS), clean)

MODULES_INSTALL_DYNAMIC_TARGETS = $(call targets_rules, $(ML_MODULES_DYNAMIC), install)

all: $(MODULES_ALL_TARGETS)

clean: $(MODULES_CLEAN_ALL_TARGETS)

prepare_install: all
	mkdir -p $(INSTALL_MODULES_DIR) || echo "no problem"

install: | prepare_install
install: all
	$(CP) $(MODULES_DIR)/*/*.mo $(INSTALL_MODULES_DIR)/ || echo "no problem"

$(MODULES_INSTALL_DYNAMIC_TARGETS) $(MODULES_CLEAN_ALL_TARGETS) $(ML_MODULES_DYNAMIC):
	$(call call_make_rule, $(MODULES_DIR))

.PHONY: $(MODULES_ALL_TARGETS)