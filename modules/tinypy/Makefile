
# this is a very simple makefile for modules
TOP_DIR=$(PWD)/../..

# define the module name - make sure name is max 8 characters
MODULE_NAME=tinypy
MODULE_OBJS=tinypy-main.o

# include make environment
include $(TOP_DIR)/Makefile.top
include $(TOP_DIR)/Makefile.inc

# some compile hacks
CFLAGS += -fno-short-double

tinypy-desktop: tinypy-desktop.c tinypy.c
	gcc tinypy-desktop.c -o tinypy-desktop -lm -g

all: tinypy.mo tinypy-desktop

clean::
	$(call build,CLEAN,$(RM) -f tinypy-desktop)
