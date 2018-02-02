# compile with e.g.
# MODEL=60D make

ifeq ($(MODEL),5D3)
   FW_VERSION=113
   CFLAG_USER += -DCONFIG_5D3_113=1
endif

CFLAG_USER += -DCONFIG_QEMU

ML_SRC_EXTRA_OBJS=raw.o vram.o propvalues.o stdio.o imgconv.o dialog_test.o
include ../Makefile.minimal
