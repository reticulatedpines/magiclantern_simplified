
# this is a very simple makefile for plugins

TOP ?= .
include $(TOP)/../../Makefile.user.default
include $(TOP)/../../Makefile.user

CFLAGS=-Os

    
%.o: %.c $(LIBTCC_INC)
	$(CC) -o $@ -c $< $(CFLAGS)
    
%.mo: %.o
	$(LD_REAL) -r -o $@ $^


all: testplug.mo