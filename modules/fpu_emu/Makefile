
# this is a very simple makefile for modules
TOP_DIR=../..

# define the module name - make sure name is max 8 characters
MODULE_NAME=fpu_emu
MODULE_OBJS=fpu_emu.o nwfpe/fpopcode.o nwfpe/fpa11.o nwfpe/fpa11_cprt.o nwfpe/fpa11_cpdt.o nwfpe/fpa11_cpdo.o nwfpe/single_cpdo.o nwfpe/double_cpdo.o nwfpe/softfloat.o

MODULE_CFLAGS += -I$(shell pwd) -D__LINUX_ARM_ARCH__=5 -D__pure=__attribute__\(\(const\)\) -D__user= -D__ARMEL__ -mabi=apcs-gnu
# include modules environment
include $(TOP_DIR)/modules/Makefile.modules

clean::
	$(call rm_files, nwfpe/*.d)
