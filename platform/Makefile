TOP_DIR=..

include Makefile.platform.map

include $(TOP_DIR)/Makefile.setup

MAKE_SUPPORTED_MODELS:=$(call make_platform_dirs, $(SUPPORTED_MODELS))

all:: all-supported-models all-model

all-supported-models: $(SUPPORTED_MODELS)

clean-supported-models:

all-model: all-supported-models
	+$(MAKE) -C all x

install-all-model:
	+$(MAKE) -C all install

clean-all-model:
	+$(MAKE) -C all clean

clean: clean-all-model $(PLATFORM_CLEAN_ALL_TARGETS)

install:

$(PLATFORM_CLEAN_ALL_TARGETS):
	$(call call_make_platform)

# This rule is able to run make for specific model (defined in ALL_SUPPORTED_MODELS), for example for 60D
$(ALL_SUPPORTED_MODELS):
	$(call call_make_platform)
