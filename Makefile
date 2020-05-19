#build system for Magic Lantern.

#build and install are working, LUA not tested

#http://www.gnu.org/software/make/manual/make.html#Automatic-Variables
#http://www.gnu.org/software/make/manual/make.html#Variables_002fRecursion

TOP_DIR=.
include Makefile.setup

############################################################################################################
#
# include platform data to build "top level" rules for every model
#
# this allows e.g.
#   make 5D3
#   make 5D3.113
#   make 5D3.123
#   make 5D3_install
#   make 5D3.113_install
# on this top level Makefile
#
include $(PLATFORM_PATH)/Makefile.platform.map
$(foreach _,$(PLATFORM_MAP),$(eval $(call makerule,$(word 1, $(subst ., ,$_)),$_,$(PLATFORM_PATH)/)))
############################################################################################################


all: modules_all platform_all

install: platform_install

clean: platform_clean doxygen_clean modules_clean

############################
# module rules
############################
modules_all:
	$(MAKE) -C modules all

modules_clean:
	$(MAKE) -C modules clean

############################
# fir rules
############################
fir_all:
	$(MAKE) -C installer clean_and_fir

fir_install: fir
	$(MAKE) -C installer install_fir

############################
# platform rules
############################
platform_install:
	$(MAKE) -C $(PLATFORM_PATH) install

platform_clean:
	$(MAKE) -C $(PLATFORM_PATH) clean

platform_all:
	$(MAKE) -C $(PLATFORM_PATH) all

  
############################  
# additional clean rules
############################
clean:
	$(call rm_files, \
		magiclantern.lds \
		$(LUA_PATH)/*.o \
		$(LUA_PATH)/.*.d \
		$(LUA_PATH)/liblua.a \
		doc/Cropmarks550D.png \
		doc/credits.tex \
		doc/install-body.tex \
		doc/install.wiki \
		doc/menuindex.txt \
		src/menuindexentries.h \
		doc/userguide.rst \
		doc/INSTALL.aux \
		doc/INSTALL.log \
		doc/INSTALL.out \
		doc/INSTALL.pdf \
		doc/INSTALL.rst \
		doc/INSTALL.tex \
		doc/INSTALL.toc \
		doc/UserGuide-cam.aux \
		doc/UserGuide-cam.log \
		doc/UserGuide-cam.out \
		doc/UserGuide-cam.pdf \
		doc/UserGuide-cam.tex \
		doc/UserGuide.aux \
		doc/UserGuide.log \
		doc/UserGuide.out \
		doc/UserGuide.pdf \
		doc/UserGuide.tex \
		doc/UserGuide.toc \
		*.pdf \
		platform/*/qemu-helper.bin \
		)
	$(call rm_dir, doc/cam)
	$(call rm_dir, $(BINARIES_PATH))

# We must build the docs first to use fresh doc/menuindex.txt
# during 'make all'. We can't write 'zip: all docs' because
# of possible problem in case of parallel build.
# (see make's '-j' option documentation)
#zip: docs
#	$(MAKE) all
#	cd $(PLATFORM_PATH)/all; $(MAKE) zip

docs:
	cd $(PLATFORM_PATH)/all; $(MAKE) docs

docq:
	cd $(PLATFORM_PATH)/all; $(MAKE) docq

doxygen:
	doxygen

doxygen_clean:
	$(call rm_dir, doxygen-doc)

features.html: FORCE
	cd features; python features-html.py > ../features.html
 
FORCE:

# we want ML platforms to be built sequentially, to avoid conflicts
# => use .NOTPARALLEL in the upper-level Makefiles only
# parallel build is still used within each platform
.NOTPARALLEL:
