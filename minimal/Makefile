# https://stackoverflow.com/questions/17834582/run-make-in-each-subdirectory/17845120#17845120

TOPTARGETS := all clean

SUBDIRS := $(sort $(patsubst %/Makefile,%,$(wildcard */Makefile)))
ifeq ($(MODEL),$(filter $(MODEL),5D4 5DS 5DSR 6D2 7D2 77D 80D 760D))
# Do not attempt qemu-frsp for these cams, they don't emul far enough
# and the extra stubs needed are not worth finding.  Models should be
# removed from the filter list when qemu is better for each cam.
#
# But still clean it, or we'll keep the version made for the last cam...
ifneq ($(MAKECMDGOALS),clean)
    SUBDIRS := $(filter-out qemu-frsp,$(SUBDIRS))
endif
endif
$(info Minimal targets: $(SUBDIRS))

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
