#build system for Magic Lantern.

#build and install are working, LUA not tested

#http://www.gnu.org/software/make/manual/make.html#Automatic-Variables
#http://www.gnu.org/software/make/manual/make.html#Variables_002fRecursion

TOP_DIR=$(PWD)
include Makefile.setup

all: $(SUPPORTED_MODELS)

include $(PLATFORM_PATH)/Makefile.platform.map

# This rule is able to run make for specific model (defined in ALL_SUPPORTED_MODELS)
#60D 550D 600D 1100D 50D 500D 5D2 5DC 40D 5D3 EOSM 650D 6D 7D_MASTER::
$(ALL_SUPPORTED_MODELS)::
	$(call call_make_platform)

7D:: 7D_MASTER
	$(MAKE) -C $(PLATFORM_PATH)/7D.203

7DFIR: 7D_MASTER 7D
	dd if=$(PLATFORM_PATH)/7D.203/autoexec.bin of=$(PLATFORM_PATH)/7D.203/autoexec.fir bs=288 skip=1 >/dev/null 2>&1
	dd if=$(PLATFORM_PATH)/7D_MASTER.203/autoexec.bin of=$(PLATFORM_PATH)/7D_MASTER.203/autoexec.fir bs=288 skip=1 >/dev/null 2>&1
	python ../dumper/build_fir7.py -r -s $(PLATFORM_PATH)/7D.203/autoexec.fir -m $(PLATFORM_PATH)/7D_MASTER.203/autoexec.fir $(PLATFORM_PATH)/7D.203/7D000203.FIR $(PLATFORM_PATH)/7D.203/MAGIC.FIR >/dev/null

platform_all_model:
	$(MAKE) -C $(PLATFORM_PATH) clean-all-model all-model

install_platform_all_model: platform_all_model
	$(MAKE) -C $(PLATFORM_PATH) install-all-model

install: install_platform_all_model

fir:
	$(MAKE) -C installer clean_and_fir

install_fir: fir
	$(MAKE) -C installer install_fir

platform_clean:
	$(MAKE) -C platform clean

clean: platform_clean doxygen_clean
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
zip: docs
	$(MAKE) all
	cd $(PLATFORM_PATH)/all; $(MAKE) zip

docs:
	cd $(PLATFORM_PATH)/all; $(MAKE) docs

docq:
	cd $(PLATFORM_PATH)/all; $(MAKE) docq

doxygen:
	doxygen

doxygen_clean:
	$(call rm_dir, doxygen-doc)

dropbox: all
	cp $(PLATFORM_PATH)/all/autoexec.bin ~/Dropbox/Public/bleeding-edge/

features.html: FORCE
	cd features; python features-html.py > ../features.html
 
FORCE:

