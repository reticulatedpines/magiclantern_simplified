# define the module name - make sure name is max 8 characters
MODULE_NAME = io_crypt
MODULE_OBJS = $(BUILD_DIR)/io_crypt.o \
              $(BUILD_DIR)/crypt_xtea.o \
              $(BUILD_DIR)/crypt_lfsr64.o \
              $(BUILD_DIR)/hash_password.o \
              $(BUILD_DIR)/crypt_rsa.o \
              $(BUILD_DIR)/bigdigits.o \
              $(BUILD_DIR)/bigd.o
TOP_DIR = ../..

# include modules environment
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete

# include host tool makefile rules
include Makefile.io_decrypt

#CFLAGS+=-DTRACE_DISABLED
#HOST_CFLAGS+=-DTRACE_DISABLED
#MINGW_CFLAGS+=-DTRACE_DISABLED
