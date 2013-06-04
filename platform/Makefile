TOP_DIR=..

include Makefile.platform.map
include $(TOP_DIR)/Makefile.setup

MAKE_SUPPORTED_MODELS:=$(call make_platform_dirs, $(SUPPORTED_MODELS))

make_supported_loop = @$(call make_loop, $(MAKE_SUPPORTED_MODELS), $(1))

make_all_loop = @$(call make_loop, $(PLATFORM_MAP), $(1))

all:: all-supported-models

all-supported-models:
	$(call make_supported_loop, all)

all-model:
	+$(MAKE) -C x

install-all-model:
	+$(MAKE) -C all install

clean-all-model:
	+$(MAKE) -C all clean

clean: clean-all-model
	$(call make_all_loop, clean)

install:
	

# This rule is able to run make for specific model (defined in ALL_SUPPORTED_MODELS), for example for 60D
$(ALL_SUPPORTED_MODELS):
	$(call platform_make_run)