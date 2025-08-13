# define the module name - make sure name is max 8 characters
MODULE_NAME = edmac
MODULE_OBJS = $(BUILD_DIR)/edmac.o $(BUILD_DIR)/edmac_util.o \
              $(BUILD_DIR)/edmac_test.o $(BUILD_DIR)/md5.o
TOP_DIR = ../../..

# include modules environment
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete

# fixme: cleaner way to localize symbols?
$(BUILD_DIR)/edmac.sym: $(BUILD_DIR)/edmac.mo
	$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$8;}" | sort \
		| grep -v edmac_format_size > $(BUILD_DIR)/localsyms
	$(OBJCOPY) $(BUILD_DIR)/edmac.mo $(BUILD_DIR)/edmac.mo --localize-symbols $(BUILD_DIR)/localsyms
	$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$2 \" \" \$$8;}" | sort > $@ && cat $@
