# compile with e.g.
# make MODEL=60D

ifeq ($(MODEL),5D3)
   FW_VERSION=113
   CFLAG_USER += -DCONFIG_5D3_113=1
endif

CFLAG_USER += -DCONFIG_QEMU

include ../Makefile.minimal
