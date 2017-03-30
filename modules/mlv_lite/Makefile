
# define the module name - make sure name is max 8 characters
MODULE_NAME=mlv_lite
MODULE_OBJS=mlv_lite.o ../mlv_rec/mlv.o ../silent/lossless.o

# include modules environment
include ../Makefile.modules

R2D_CFLAGS = -I$(SRC_DIR) -m32 -mno-ms-bitfields -D_FILE_OFFSET_BITS=64
R2D_LFLAGS = -lm -m32

# RAW to DNG converter for PC
raw2dng: $(SRC_DIR)/chdk-dng.c ../lv_rec/raw2dng.c
	$(call build,GCC,gcc -c $(SRC_DIR)/chdk-dng.c $(HOST_CFLAGS) $(R2D_CFLAGS))
	$(call build,GCC,gcc -c ../lv_rec/raw2dng.c $(HOST_CFLAGS) $(R2D_CFLAGS))
	$(call build,GCC,gcc raw2dng.o chdk-dng.o -o raw2dng $(HOST_LFLAGS) $(R2D_LFLAGS))

# debug tool
dng2raw: dng2raw.c
	$(call build,GCC,gcc dng2raw.c $(HOST_CFLAGS) $(R2D_CFLAGS)) -o dng2raw

raw2dng.exe: $(SRC_DIR)/chdk-dng.c ../lv_rec/raw2dng.c
	$(call build,MINGW,$(MINGW_GCC) -c $(SRC_DIR)/chdk-dng.c $(HOST_CFLAGS) $(R2D_CFLAGS))
	$(call build,MINGW,$(MINGW_GCC) -c ../lv_rec/raw2dng.c $(HOST_CFLAGS) $(R2D_CFLAGS))
	$(call build,MINGW,$(MINGW_GCC) raw2dng.o chdk-dng.o -o raw2dng.exe $(HOST_LFLAGS) $(R2D_LFLAGS))

dng2raw.exe: dng2raw.c
	$(call build,MINGW,$(MINGW_GCC) dng2raw.c $(HOST_CFLAGS) $(R2D_CFLAGS)) -o dng2raw.exe

clean::
	$(call rm_files, raw2dng raw2dng.exe dng2raw dng2raw.exe)
