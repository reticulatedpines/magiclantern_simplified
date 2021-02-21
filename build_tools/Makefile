# Makefile for the various tools used in the build process

ifndef TOP_DIR
# if this is used as a standalone makefile, we need to include some additional stuff
TOP_DIR=..
include $(TOP_DIR)/Makefile.setup
XOR_CHK:=$(notdir $(XOR_CHK))
endif

$(XOR_CHK): $(XOR_CHK).c
	$(call build,XOR_CHK,$(HOST_CC) $< -o $@)

clean::
	$(call rm_files, $(XOR_CHK) $(XOR_CHK).exe)
