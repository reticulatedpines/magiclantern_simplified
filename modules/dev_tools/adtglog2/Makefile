# define the module name - make sure name is max 8 characters
MODULE_NAME = adtglog2
MODULE_OBJS = $(BUILD_DIR)/adtglog2.o \
              $(BUILD_DIR)/hooks_thumb_200D.o \
              $(BUILD_DIR)/hooks_thumb_6D2.o \
              $(BUILD_DIR)/hooks_arm_70D.o
TOP_DIR = ../../..

# include modules environment
include $(TOP_DIR)/modules/Makefile
CFLAGS_ARCHLESS = $(filter-out -march=armv7-r -march=armv7-a -march=armv5te,$(CFLAGS))
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete

# We want the core module to run on any cam.
# We need Thumb instructions for hooks on D678X cams.
# We link in object files built with the arch we need,
# code in adtglog2.c is responsible for never calling
# Thumb from non-Thumb cam, etc.
$(BUILD_DIR)/hooks_thumb_200D.o: hooks_thumb_200D.c
	$(CC) $(CFLAGS_ARCHLESS) -mthumb -march=armv7 $(CDEPS_FLAGS) -c -o $@ $<

$(BUILD_DIR)/hooks_thumb_6D2.o: hooks_thumb_6D2.c
	$(CC) $(CFLAGS_ARCHLESS) -mthumb -march=armv7 $(CDEPS_FLAGS) -c -o $@ $<

$(BUILD_DIR)/hooks_arm_70D.o: hooks_arm_70D.c
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -c -o $@ $<
