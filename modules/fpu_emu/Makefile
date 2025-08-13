# define the module name - make sure name is max 8 characters
MODULE_NAME = fpu_emu
MODULE_OBJS = $(BUILD_DIR)/fpu_emu.o \
              $(BUILD_DIR)/nwfpe/fpopcode.o \
              $(BUILD_DIR)/nwfpe/fpa11.o \
              $(BUILD_DIR)/nwfpe/fpa11_cprt.o \
              $(BUILD_DIR)/nwfpe/fpa11_cpdt.o \
              $(BUILD_DIR)/nwfpe/fpa11_cpdo.o \
              $(BUILD_DIR)/nwfpe/single_cpdo.o \
              $(BUILD_DIR)/nwfpe/double_cpdo.o \
              $(BUILD_DIR)/nwfpe/softfloat.o
TOP_DIR = ../..

MODULE_CFLAGS += -I$(shell pwd) -D__LINUX_ARM_ARCH__=5 -D__pure=__attribute__\(\(const\)\) -D__user= -D__ARMEL__ -mabi=apcs-gnu
# include modules environment
include $(TOP_DIR)/modules/Makefile
.DEFAULT_GOAL := $(BUILD_DIR)/module_complete
