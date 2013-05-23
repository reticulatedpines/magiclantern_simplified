TOP_DIR= ..
include $(TOP_DIR)/Makefile.setup

include $(MODULES_DIR)/Makefile.user.default
-include $(MODULES_DIR)/Makefile.user

make_loop = $(foreach module,$(1), make -C $(MODULES_DIR)/$(module) $(2);)

make_dynamic_modules = $(call make_loop, $(ML_MODULES_DYNAMIC), $(1))

make_all_modules = $(call make_loop, $(ML_MODULES_STATIC) $(ML_MODULES_DYNAMIC), $(1))

all:
	$(call make_all_modules,)

clean:
	$(call make_all_modules,clean)

install:
	mkdir $(CF_CARD_ML_DIR)/modules || echo "no problem"
	$(call make_dynamic_modules, install)
	cp $(MODULES_DIR)/*/*.mo $(CF_CARD_ML_DIR)/modules/ || echo "no problem"
