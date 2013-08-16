#
# Tiny C Compiler Makefile
#

TOP ?= .

# this is a very simple makefile for modules
TOP_DIR=$(TOP)/../..

# define the module name - make sure name is max 8 characters
MODULE_NAME=tcc
MODULE_OBJS=libtccx.o

# include modules environment
include $(TOP_DIR)/modules/Makefile.modules

include $(TOP)/config.mak

VPATH = $(top_srcdir)

CPPFLAGS = -I$(TOP) # for config.h

ifeq (-$(findstring gcc,$(CC))-,-gcc-)
ifeq (-$(findstring $(GCC_MAJOR),01)-,--)
CFLAGS+=-fno-strict-aliasing
ifeq (-$(findstring $(GCC_MAJOR),23)-,--)
CFLAGS+=-Wno-pointer-sign -Wno-sign-compare
ifeq (-$(GCC_MAJOR)-$(findstring $(GCC_MINOR),56789)-,-4--)
CFLAGS+=-D_FORTIFY_SOURCE=0
else
CFLAGS+=-Wno-unused-result
endif
endif
endif
else # not GCC
ifeq (-$(findstring clang,$(CC))-,-clang-)
# make clang accept gnuisms in libtcc1.c
CFLAGS+=-fheinous-gnu-extensions
endif
endif

CPPFLAGS_P=$(CPPFLAGS) -DCONFIG_TCC_STATIC
CFLAGS_P=$(CFLAGS) -pg -static
LIBS_P=
LDFLAGS_P=$(LDFLAGS)

ifdef CONFIG_WIN64
CONFIG_WIN32=yes
endif

ifndef CONFIG_WIN32
LIBS=-lm
endif

# make libtcc as static or dynamic library?
ifdef DISABLE_STATIC
LIBTCC=libtcc.so.1.0
LINK_LIBTCC=-Wl,-rpath,"$(libdir)"
ifdef DISABLE_RPATH
LINK_LIBTCC=
endif
else
LIBTCC=libtcc.a
LINK_LIBTCC=
endif

CONFIG_$(ARCH) = yes
NATIVE_DEFINES_$(CONFIG_i386) += -DTCC_TARGET_I386
NATIVE_DEFINES_$(CONFIG_x86-64) += -DTCC_TARGET_X86_64
NATIVE_DEFINES_$(CONFIG_WIN32) += -DTCC_TARGET_PE
NATIVE_DEFINES_$(CONFIG_uClibc) += -DTCC_UCLIBC
NATIVE_DEFINES_$(CONFIG_arm) += -DTCC_TARGET_ARM -DWITHOUT_LIBTCC
NATIVE_DEFINES_$(CONFIG_arm_eabihf) += -DTCC_ARM_EABI -DTCC_ARM_HARDFLOAT
NATIVE_DEFINES_$(CONFIG_arm_eabi) += -DTCC_ARM_EABI
NATIVE_DEFINES_$(CONFIG_arm_vfp) += -DTCC_ARM_VFP
NATIVE_DEFINES += $(NATIVE_DEFINES_yes)

ifeq ($(TOP),.)

PROGS=tcc$(EXESUF)
I386_CROSS = i386-tcc$(EXESUF)
WIN32_CROSS = i386-win32-tcc$(EXESUF)
WIN64_CROSS = x86_64-win32-tcc$(EXESUF)
WINCE_CROSS = arm-win32-tcc$(EXESUF)
X64_CROSS = x86_64-tcc$(EXESUF)
ARM_FPA_CROSS = arm-fpa-tcc$(EXESUF)
ARM_FPA_LD_CROSS = arm-fpa-ld-tcc$(EXESUF)
ARM_VFP_CROSS = arm-vfp-tcc$(EXESUF)
ARM_EABI_CROSS = arm-eabi-tcc$(EXESUF)
ARM_CROSS = $(ARM_FPA_CROSS) $(ARM_FPA_LD_CROSS) $(ARM_VFP_CROSS) $(ARM_EABI_CROSS)
C67_CROSS = c67-tcc$(EXESUF)

CORE_FILES = tcc.c libtcc.c tccpp.c tccgen.c tccelf.c tccasm.c tccrun.c
CORE_FILES += tcc.h config.h libtcc.h tcctok.h
I386_FILES = $(CORE_FILES) i386-gen.c i386-asm.c i386-asm.h i386-tok.h
WIN32_FILES = $(CORE_FILES) i386-gen.c i386-asm.c i386-asm.h i386-tok.h tccpe.c
WIN64_FILES = $(CORE_FILES) x86_64-gen.c i386-asm.c x86_64-asm.h tccpe.c
WINCE_FILES = $(CORE_FILES) arm-gen.c tccpe.c
X86_64_FILES = $(CORE_FILES) x86_64-gen.c i386-asm.c x86_64-asm.h
ARM_FILES = $(CORE_FILES) arm-gen.c
C67_FILES = $(CORE_FILES) c67-gen.c tcccoff.c

ifdef CONFIG_WIN64
PROGS+=tiny_impdef$(EXESUF) tiny_libmaker$(EXESUF)
NATIVE_FILES=$(WIN64_FILES)
PROGS_CROSS=$(WIN32_CROSS) $(I386_CROSS) $(X64_CROSS) $(ARM_CROSS) $(C67_CROSS)
LIBTCC1_CROSS=lib/i386-win32/libtcc1.a
LIBTCC1=libtcc1.a
else ifdef CONFIG_WIN32
PROGS+=tiny_impdef$(EXESUF) tiny_libmaker$(EXESUF)
NATIVE_FILES=$(WIN32_FILES)
PROGS_CROSS=$(WIN64_CROSS) $(I386_CROSS) $(X64_CROSS) $(ARM_CROSS) $(C67_CROSS)
LIBTCC1_CROSS=lib/x86_64-win32/libtcc1.a
LIBTCC1=libtcc1.a
else ifeq ($(ARCH),i386)
NATIVE_FILES=$(I386_FILES)
PROGS_CROSS=$(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(C67_CROSS)
LIBTCC1_CROSS=lib/i386-win32/libtcc1.a lib/x86_64-win32/libtcc1.a
LIBTCC1=libtcc1.a
else ifeq ($(ARCH),x86-64)
NATIVE_FILES=$(X86_64_FILES)
PROGS_CROSS=$(I386_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(C67_CROSS)
LIBTCC1_CROSS=lib/i386-win32/libtcc1.a lib/x86_64-win32/libtcc1.a lib/i386/libtcc1.a
LIBTCC1=libtcc1.a
else ifeq ($(ARCH),arm)
NATIVE_FILES=$(ARM_FILES)
PROGS_CROSS=$(I386_CROSS) $(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(C67_CROSS)
endif

ifeq ($(TARGETOS),Darwin)
PROGS+=tiny_libmaker$(EXESUF)
endif

ifdef CONFIG_USE_LIBGCC
LIBTCC1=
endif

TCCLIBS = $(LIBTCC1) $(LIBTCC)
TCCDOCS = tcc.1 tcc-doc.html tcc-doc.info

ifdef CONFIG_CROSS
PROGS+=$(PROGS_CROSS)
TCCLIBS+=$(LIBTCC1_CROSS)
endif

#all: $(PROGS) $(TCCLIBS) $(TCCDOCS)

#~ all: libtccx.o

# Host Tiny C Compiler
tcc$(EXESUF): tcc.o $(LIBTCC)
	$(CC) -o $@ $^ $(LIBS) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(LINK_LIBTCC)

# Cross Tiny C Compilers
%-tcc$(EXESUF): tcc.c
	$(CC) -o $@ $< -DONE_SOURCE $(DEFINES) $(CPPFLAGS) $(CFLAGS) $(LIBS) $(LDFLAGS)

# profiling version
tcc_p$(EXESUF): $(NATIVE_FILES)
	$(CC) -o $@ $< -DONE_SOURCE $(NATIVE_DEFINES) $(CPPFLAGS_P) $(CFLAGS_P) $(LIBS_P) $(LDFLAGS_P)

$(I386_CROSS): DEFINES = -DTCC_TARGET_I386 \
    -DCONFIG_TCCDIR="\"$(tccdir)/i386\""
$(X64_CROSS): DEFINES = -DTCC_TARGET_X86_64
$(WIN32_CROSS): DEFINES = -DTCC_TARGET_I386 -DTCC_TARGET_PE \
    -DCONFIG_TCCDIR="\"$(tccdir)/win32\"" \
    -DCONFIG_TCC_LIBPATHS="\"{B}/lib/32;{B}/lib\""
$(WIN64_CROSS): DEFINES = -DTCC_TARGET_X86_64 -DTCC_TARGET_PE \
    -DCONFIG_TCCDIR="\"$(tccdir)/win32\"" \
    -DCONFIG_TCC_LIBPATHS="\"{B}/lib/64;{B}/lib\""
$(WINCE_CROSS): DEFINES = -DTCC_TARGET_PE
$(C67_CROSS): DEFINES = -DTCC_TARGET_C67
$(ARM_FPA_CROSS): DEFINES = -DTCC_TARGET_ARM
$(ARM_FPA_LD_CROSS)$(EXESUF): DEFINES = -DTCC_TARGET_ARM -DLDOUBLE_SIZE=12
$(ARM_VFP_CROSS): DEFINES = -DTCC_TARGET_ARM -DTCC_ARM_VFP
$(ARM_EABI_CROSS): DEFINES = -DTCC_TARGET_ARM -DTCC_ARM_EABI

$(I386_CROSS): $(I386_FILES)
$(X64_CROSS): $(X86_64_FILES)
$(WIN32_CROSS): $(WIN32_FILES)
$(WIN64_CROSS): $(WIN64_FILES)
$(WINCE_CROSS): $(WINCE_FILES)
$(C67_CROSS): $(C67_FILES)
$(ARM_FPA_CROSS) $(ARM_FPA_LD_CROSS) $(ARM_VFP_CROSS) $(ARM_EABI_CROSS): $(ARM_FILES)

# libtcc generation and test
ifndef ONE_SOURCE
LIBTCC_OBJ = $(filter-out tcc.o,$(patsubst %.c,%.o,$(filter %.c,$(NATIVE_FILES))))
LIBTCC_INC = $(filter %.h,$(CORE_FILES)) $(filter-out $(CORE_FILES),$(NATIVE_FILES))
else
LIBTCC_OBJ = libtcc.o
LIBTCC_INC = $(NATIVE_FILES)
libtcc.o : NATIVE_DEFINES += -DONE_SOURCE
endif

$(LIBTCC_OBJ) tcc.o : %.o : %.c $(LIBTCC_INC)
	$(CC) -o $@ -c $< $(NATIVE_DEFINES) $(CPPFLAGS) $(CFLAGS)

libtcc.a: $(LIBTCC_OBJ)
	$(AR) rcs $@ $^

# I'm sure there's a cleanest solution, but... I just don't know how
localsyms: libtcctmp.o
	$(READELF) $< -Ws | $(AWK) '{print $$8}' | sort | uniq \
		| grep -v "^tcc_new$$" \
		| grep -v "^tcc_delete$$" \
		| grep -v "^tcc_add_file$$" \
		| grep -v "^tcc_relocate$$" \
		| grep -v "^tcc_get_symbol$$" \
		| grep -v "^tcc_add_symbol$$" \
		| grep -v "^tcc_set_options$$" \
		| grep -v "^tcc_set_output_type$$" \
		> $@

#~ libtcc.a: libtcctmp.a localsyms
	#~ $(OBJCOPY) libtcctmp.a libtcc.a --localize-symbols localsyms

libtcctmp.o: libtcc.o tccpp.o tccgen.o tccelf.o tccasm.o tccrun.o arm-gen.o
	$(LD_REAL) -r -o $@ $^

libtccx.o: libtcctmp.o localsyms
	$(OBJCOPY) $< $@ --localize-symbols localsyms


libtcc.so.1.0: $(LIBTCC_OBJ)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ $(LDFLAGS)

libtcc.so.1.0: CFLAGS+=-fPIC

# windows utilities
tiny_impdef$(EXESUF): win32/tools/tiny_impdef.c
	$(CC) -o $@ $< $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)
tiny_libmaker$(EXESUF): win32/tools/tiny_libmaker.c
	$(CC) -o $@ $< $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)

# TinyCC runtime libraries
libtcc1.a : FORCE
	$(MAKE) -C lib native
lib/%/libtcc1.a : FORCE $(PROGS_CROSS)
	$(MAKE) -C lib cross TARGET=$*

FORCE:

# documentation and man page
tcc-doc.html: tcc-doc.texi
	-texi2html -monolithic -number $<

tcc.1: tcc-doc.texi
	-$(top_srcdir)/texi2pod.pl $< tcc.pod
	-pod2man --section=1 --center=" " --release=" " tcc.pod > $@

tcc-doc.info: tcc-doc.texi
	-makeinfo $<

# in tests subdir
export LIBTCC1

%est:
	$(MAKE) -C tests $@

clean::
	rm -vf $(PROGS) tcc_p$(EXESUF) tcc.pod *~ *.o *.a *.so* *.out *.exe libtcc_test$(EXESUF)
	#~ $(MAKE) -C tests $@
ifneq ($(LIBTCC1),)
	$(MAKE) -C lib $@
endif

distclean: clean
	rm -vf config.h config.mak config.texi tcc.1 tcc-doc.info tcc-doc.html

config.mak:
	@echo "Please run ./configure."
	@exit 1

# create release tarball from *current* git branch (including tcc-doc.html
# and converting two files to CRLF)
TCC-VERSION := tcc-$(shell cat $(top_srcdir)/VERSION)
tar:    tcc-doc.html
	mkdir $(TCC-VERSION)
	( cd $(TCC-VERSION) && git --git-dir ../.git checkout -f )
	cp tcc-doc.html $(TCC-VERSION)
	for f in tcc-win32.txt build-tcc.bat ; do \
	    cat win32/$$f | sed 's,\(.*\),\1\r,g' > $(TCC-VERSION)/win32/$$f ; \
	done
	tar cjf $(TCC-VERSION).tar.bz2 $(TCC-VERSION)
	rm -rf $(TCC-VERSION)
	git reset


.PHONY: all clean tar distclean install uninstall FORCE

endif # ifeq ($(TOP),.)
