
# this is a very simple makefile for modules
TOP_DIR=../..

# define the module name - make sure name is max 8 characters
MODULE_NAME=dual_iso
MODULE_OBJS=dual_iso.o

all:: cr2hdr

# include modules environment
include $(TOP_DIR)/modules/Makefile.modules

cr2hdr: $(SRC_DIR)/chdk-dng.c cr2hdr.c
	$(call build,GCC,gcc -c $(SRC_DIR)/chdk-dng.c -m32 -mno-ms-bitfields -O2 -Wall -ggdb -I$(SRC_DIR))
	$(call build,GCC,gcc -c cr2hdr.c -m32 -mno-ms-bitfields -O2 -Wall -ggdb -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64)
	$(call build,GCC,gcc cr2hdr.o chdk-dng.o -o cr2hdr -lm -m32 -ggdb)


clean::
	$(call rm_files, cr2hdr)
