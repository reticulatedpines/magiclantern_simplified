# https://stackoverflow.com/questions/17834582/run-make-in-each-subdirectory/17845120#17845120

TOPTARGETS := all clean

SUBDIRS := $(sort $(patsubst %/Makefile,%,$(wildcard */Makefile)))
$(info Minimal targets: $(SUBDIRS))

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
