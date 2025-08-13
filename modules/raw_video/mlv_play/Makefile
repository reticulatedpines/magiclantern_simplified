# define the module name - make sure name is max 8 characters
MODULE_NAME = mlv_play
MODULE_OBJS = $(BUILD_DIR)/mlv_play.o $(BUILD_DIR)/video.bmp.rsc
TOP_DIR = ../../..

# include modules environment
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete

$(BUILD_DIR)/video.bmp.rsc: video.bmp
	$(LD) -r -b binary -o $@ $<
