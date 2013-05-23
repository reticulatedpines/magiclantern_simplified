
# this is a very simple makefile for modules
TOP_DIR=../..

# define the module name - make sure name is max 8 characters
MODULE_NAME=tinypy
MODULE_OBJS=tinypy-main.o

# some compile hacks
MODULE_CFLAGS += -fno-short-double

# include make environment
include $(TOP_DIR)/modules/Makefile.modules


tinypy-desktop: tinypy-desktop.c tinypy.c
	gcc tinypy-desktop.c -o tinypy-desktop -lm -g

all:: tinypy-desktop

clean::
	$(call build,CLEAN,$(RM) -f tinypy-desktop)
