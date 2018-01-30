# define the module name - make sure name is max 8 characters
MODULE_NAME=edmac
MODULE_OBJS=edmac.o edmac_util.o edmac_test.o md5.o

# include modules environment
include ../Makefile.modules

# fixme: cleaner way to localize symbols?
edmac.sym: edmac.mo
	$(call build,STRIP,$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$8;}" | sort \
		| grep -v edmac_format_size > localsyms)
	$(call build,STRIP,$(OBJCOPY) edmac.mo edmac.mo --localize-symbols localsyms)
	$(call rm_files, localsyms)
	$(call build,EXPORTS,$(READELF) -sW $< | grep GLOBAL | grep -v UND | grep -v COM | grep -v "__module_.*$(MODULE_NAME)" | $(AWK) "{print \$$2 \" \" \$$8;}" | sort > $@ && cat $@)
