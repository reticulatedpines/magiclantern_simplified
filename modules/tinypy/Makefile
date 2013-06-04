
# define the module name - make sure name is max 8 characters
MODULE_NAME=tinypy
MODULE_OBJS=tinypy-main.o

# some compile hacks
MODULE_CFLAGS += -fno-short-double

# include make environment
include ../Makefile.modules

all:: tinypy-desktop

clean::
	$(call build,CLEAN,$(RM) -f tinypy-desktop)

tinypy-desktop: tinypy-desktop.c tinypy.c
	gcc tinypy-desktop.c -o tinypy-desktop -lm -g
