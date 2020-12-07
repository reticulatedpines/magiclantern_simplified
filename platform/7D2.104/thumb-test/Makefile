TOP_DIR=../../..
include $(TOP_DIR)/Makefile.setup

asmtest: FORCE
	$(CC) test.S -nostdlib -o test.o
	$(OBJDUMP) -d test.o
	gcc test.c -o test && ./test

all: asmtest

clean:
	$(call rm_files, *.o test)

FORCE:
