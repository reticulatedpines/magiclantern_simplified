
# this is a very simple makefile for modules
TOP_DIR=$(PWD)/../..

# define the module name - make sure name is max 8 characters
MODULE_NAME=lv_rec
MODULE_OBJS=lv_rec.o

# include make environment
include $(TOP_DIR)/Makefile.top
include $(TOP_DIR)/Makefile.inc

all: lv_rec.mo raw2dng

# RAW to DNG converter for PC
raw2dng: FORCE
	$(call build,GCC,gcc -c $(SRC_DIR)/chdk-dng.c -m32 -O2 -Wall -I$(SRC_DIR))
	$(call build,GCC,gcc -c raw2dng.c -m32 -O2 -Wall -I$(SRC_DIR))
	$(call build,GCC,gcc raw2dng.o chdk-dng.o -o raw2dng -lm -m32)

clean::
	$(call build,CLEAN,$(RM) -f raw2dng *.o *.d)
