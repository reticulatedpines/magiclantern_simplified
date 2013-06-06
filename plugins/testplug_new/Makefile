
# this is a very simple makefile for plugins

TOP_DIR=../..

include $(TOP_DIR)/Makefile.setup

CFLAGS=-Os

    
%.o: %.c $(LIBTCC_INC)
	$(CC) -o $@ -c $< $(CFLAGS)
    
%.mo: %.o
	$(LD_REAL) -r -o $@ $^

all: testplug.mo

clean:
	$(call rm_files, *.o *.mo testplug.mo)
