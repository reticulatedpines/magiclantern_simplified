TOP_DIR?=..

ML_SETUP_EXTENSION=modules
include $(TOP_DIR)/Makefile.setup

make_dynamic_modules = $(call make_loop, $(ML_MODULES_DYNAMIC), $(1))

make_all_modules = $(call make_loop, $(ML_MODULES_STATIC) $(ML_MODULES_DYNAMIC), $(1))

all:
	$(call make_all_modules,)

clean:
	$(call make_all_modules,clean)

prepare_install:
	mkdir $(INSTALL_MODULES_DIR) || echo "no problem"

install: prepare_install all
	$(call make_dynamic_modules, install)
	$(CP) $(MODULES_DIR)/*/*.mo $(INSTALL_MODULES_DIR)/ || echo "no problem"
