# define the module name - make sure name is max 8 characters
MODULE_NAME = mlv_lite
MODULE_OBJS = $(BUILD_DIR)/mlv_lite.o \
              $(BUILD_DIR)/mlv.o \
              $(BUILD_DIR)/lossless.o
TOP_DIR = ../../..

# include modules environment
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete

R2D_CFLAGS = -I$(SRC_DIR) -m32 -mno-ms-bitfields -D_FILE_OFFSET_BITS=64
R2D_LFLAGS = -lm -m32

$(BUILD_DIR)/mlv.o: ../mlv_rec/mlv.c
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -c -o $@ $<

$(BUILD_DIR)/lossless.o: ../../silent/lossless.c
	$(CC) $(CFLAGS) $(CDEPS_FLAGS) -c -o $@ $<

# RAW to DNG converter for PC
$(BUILD_DIR)/raw2dng: $(SRC_DIR)/chdk-dng.c ../../lv_rec/raw2dng.c
	gcc -c $(SRC_DIR)/chdk-dng.c -o $(BUILD_DIR)/chdk-dng.o $(HOST_CFLAGS) $(R2D_CFLAGS)
	gcc -c ../../lv_rec/raw2dng.c -o $(BUILD_DIR)/raw2dng.o $(HOST_CFLAGS) $(R2D_CFLAGS)
	gcc $(BUILD_DIR)/raw2dng.o $(BUILD_DIR)/chdk-dng.o -o $(BUILD_DIR)/raw2dng $(HOST_LFLAGS) $(R2D_LFLAGS)

# debug tool
$(BUILD_DIR)/dng2raw: dng2raw.c
	gcc dng2raw.c $(HOST_CFLAGS) $(R2D_CFLAGS) -o $(BUILD_DIR)/dng2raw

$(BUILD_DIR)/raw2dng.exe: $(SRC_DIR)/chdk-dng.c ../../lv_rec/raw2dng.c
	$(MINGW_GCC) -c $(SRC_DIR)/chdk-dng.c -o $(BUILD_DIR)/chdk-dng.o $(HOST_CFLAGS) $(R2D_CFLAGS)
	$(MINGW_GCC) -c ../../lv_rec/raw2dng.c -o $(BUILD_DIR)/raw2dng.o $(HOST_CFLAGS) $(R2D_CFLAGS)
	$(MINGW_GCC) $(BUILD_DIR)/raw2dng.o $(BUILD_DIR)/chdk-dng.o -o $(BUILD_DIR)/raw2dng.exe $(HOST_LFLAGS) $(R2D_LFLAGS)

$(BUILD_DIR)/dng2raw.exe: dng2raw.c
	$(MINGW_GCC) dng2raw.c $(HOST_CFLAGS) $(R2D_CFLAGS) -o $(BUILD_DIR)/dng2raw.exe
