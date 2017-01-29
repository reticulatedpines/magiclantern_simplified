
# define the module name - make sure name is max 8 characters
MODULE_NAME=lv_rec
MODULE_OBJS=lv_rec.o

# include modules environment
include ../Makefile.modules

# RAW to DNG converter for PC
raw2dng: FORCE
	$(call build,GCC,gcc -c $(SRC_DIR)/chdk-dng.c -m32 -O2 -Wall -I$(SRC_DIR))
	$(call build,GCC,gcc -c raw2dng.c -m32 -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200808L -std=c99)
	$(call build,GCC,gcc raw2dng.o chdk-dng.o -o raw2dng -lm -m32)

raw2dng.exe: FORCE
	$(call build,MINGW,$(MINGW_GCC) -c $(SRC_DIR)/chdk-dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR))
	$(call build,MINGW,$(MINGW_GCC) -c raw2dng.c -m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64 -std=c99)
	$(call build,MINGW,$(MINGW_GCC) raw2dng.o chdk-dng.o -o raw2dng.exe -lm -m32)

clean::
	$(call rm_files, raw2dng raw2dng.exe)
