# FIXME this module is broken.  It wants stubs that it is clear
# old cams had, but they've all been commented out, e.g. PackMem_StartEDmac.
#
# Unfortunately, other modules include a header from this module,
# so we can't simply delete it.
# Probably, we should more generally have a place for cross-module
# header files, it feels wrong to include them direct from another modules
# code.

# define the module name - make sure name is max 8 characters
MODULE_NAME = lv_rec
MODULE_OBJS = $(BUILD_DIR)/lv_rec.o
TOP_DIR = ../..

# include modules environment
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete

# RAW to DNG converter for PC
$(BUILD_DIR)/raw2dng: FORCE
	$(call build,GCC,gcc -c $(SRC_DIR)/chdk-dng.c -m32 -O2 -Wall -I$(SRC_DIR))
	$(call build,GCC,gcc -c raw2dng.c -m32 -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200808L -std=c99)
	$(call build,GCC,gcc raw2dng.o chdk-dng.o -o raw2dng -lm -m32)

$(BUILD_DIR)/raw2dng.exe: FORCE
	$(call build,MINGW,$(MINGW_GCC) -c $(SRC_DIR)/chdk-dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR))
	$(call build,MINGW,$(MINGW_GCC) -c raw2dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64 -std=c99)
	$(call build,MINGW,$(MINGW_GCC) raw2dng.o chdk-dng.o -o raw2dng.exe -lm -m32)
