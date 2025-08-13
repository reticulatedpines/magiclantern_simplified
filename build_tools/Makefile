# Makefile for the various tools used in the build process

ifndef TOP_DIR
# if standalone build, suppress some warnings,
# and change config so it builds outside of an ML context
TOP_DIR = ../
include $(TOP_DIR)/Makefile.globals
CFLAGS =
MAKEFLAGS =
LFLAGS =
LDFLAGS =
AFLAGS =
CC = gcc
LD = ld

XOR_CHK:=$(notdir $(XOR_CHK))
endif

$(XOR_CHK): $(XOR_CHK).c
	$(HOST_CC) $< -o xor_chk

clean::
	@rm -f -- xor_chk xor_chk.exe ||:
