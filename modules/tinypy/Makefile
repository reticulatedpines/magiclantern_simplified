# FIXME this module is broken.  First build error relates to
# missing GetFileSize().  Unclear what this should be now,
# or what include is missing.

# define the module name - make sure name is max 8 characters
MODULE_NAME = tinypy
MODULE_OBJS = $(BUILD_DIR)/tinypy-main.o $(BUILD_DIR)/tinypy-desktop
TOP_DIR = ../..

MODULE_CFLAGS += -I../../tcc/

# include make environment
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete

$(BUILD_DIR)/tinypy-desktop: tinypy-desktop.c tinypy.c
	gcc tinypy-desktop.c -o $(BUILD_DIR)/tinypy-desktop -lm -g
