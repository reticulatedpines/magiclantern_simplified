# Extra rules added to Makefile.setup

all:: $(_CONFIGS_to_compile_rules)

_CONFIGS_compile_name = _compile
_CONFIGS_clean_name = _clean
_CONFIGS_install_name = _install

_CONFIGS_to_filter_out = CONFIG_PICOC

_CONFIGS_to_compile = $(filter-out $(_CONFIGS_to_filter_out), $(ML_SETUP_CONFIGS_WITH_DIR))

_CONFIGS_to_compile_rules = $(foreach elem, $(_CONFIGS_to_compile), $(elem)$(_CONFIGS_compile_name))

_CONFIGS_clean=$(patsubst %, %$(_CONFIGS_clean_name), $(_CONFIGS_to_compile))
_CONFIGS_install=$(patsubst %, %$(_CONFIGS_install_name), $(_CONFIGS_to_compile))

clean-all:: clean $(_CONFIGS_clean)

CONFIG_%$(_CONFIGS_compile_name):
	+$(MAKE) -C $($*_DIR)

CONFIG_%$(_CONFIGS_clean_name):
	+$(MAKE) -C $($*_DIR) clean

CONFIG_%$(_CONFIGS_install_name): CONFIG_%$(_CONFIGS_compile_name)
	+$(MAKE) -C $($*_DIR) install

install:: prepare_install_dir install_extra_data $(_CONFIGS_install)
	$(INSTALL_FINISH)

prepare_install_dir:
	mkdir -p $(INSTALL_ML_DIR)
	mkdir -p $(INSTALL_SETTINGS_DIR)
	mkdir -p $(INSTALL_DATA_DIR)
	mkdir -p $(INSTALL_CROPMARKS_DIR)
	mkdir -p $(INSTALL_SCRIPTS_DIR)
	mkdir -p $(INSTALL_LOGS_DIR)
	mkdir -p $(INSTALL_DOC_DIR)

install_extra_data: prepare_install_dir
	$(CP) $(SRC_DIR)/FONTS.DAT $(INSTALL_DATA_DIR)/
	$(CP) $(VRAM_DIR)/*.lut $(INSTALL_DATA_DIR)/
	$(CP) $(CROP_DIR)/*.bmp $(INSTALL_CROPMARKS_DIR)/
#TODO: This should be moved to plugins or something more related
	$(CP) $(SCRIPT_DIR)/*.lua $(INSTALL_SCRIPTS_DIR)/
	$(CP) $(SCRIPT_DIR)/*.c $(INSTALL_SCRIPTS_DIR)/
	$(CP) $(SCRIPT_DIR)/*.py $(INSTALL_SCRIPTS_DIR)/


copy-autoexec: autoexec.bin
	$(CP) autoexec.bin $(INSTALL_DIR)
	$(INSTALL_FINISH)

ptpinstall: autoexec.bin
	gvfs-mount -s gphoto2
	sleep 2
	cat $(TOP_DIR)/src/ptpcam.txt | ptpcam --chdk
	diff autoexec.bin autoexec.test

docs:
	cd $(DOC_DIR) && RST2LATEX=$(RST2LATEX) $(PYTHON) mkdoc.py
	cd $(DOC_DIR) && RST2LATEX=$(RST2LATEX) $(PYTHON) mkdoc-cam.py

docq:
	cd $(DOC_DIR) && RST2LATEX=$(RST2LATEX) $(PYTHON) mkdoc.py
	cd $(DOC_DIR) && RST2LATEX=$(RST2LATEX) $(PYTHON) mkdoc-cam.py quick

installdoc:
	mkdir -p $(INSTALL_DOC_DIR)
	-$(CP) -v $(DOC_DIR)/cam/* $(INSTALL_DOC_DIR)
	-$(CP) $(DOC_DIR)/README.user $(INSTALL_ML_DIR)/README
	-$(CP) $(DOC_DIR)/INSTALL.pdf $(INSTALL_DIR) || echo "warning Unable to copy INSTALL.pdf"
	-$(CP) $(DOC_DIR)/UserGuide.pdf $(INSTALL_DIR) || echo "warning Unable to copy UserGuide.pdf"
	-$(CP) $(SRC_DIR)/logo.bmp $(INSTALL_ML_DIR)/docs || echo "warning Unable to copy logo.bmp"

prepare_zip: FORCE
	$(eval INSTALL_DIR = $(PWD)/zip)
	$(eval INSTALL_FINISH = )
	$(eval export INSTALL_DIR)
	$(eval export INSTALL_MODULES_DIR)
	$(eval export INSTALL_FINISH)
	$(call rm_dir, $(INSTALL_DIR)/)
	$(warning remove $(INSTALL_DIR))
	mkdir -p $(INSTALL_DIR)

# zip.txt must be the first item on the list!
magiclantern-$(VERSION).zip: install installdoc install_fir
	-$(call rm_files, $@)
	chmod -x $(INSTALL_DIR)/autoexec.bin
	cd $(INSTALL_DIR); zip -z ../$@ < \
		../$(SRC_DIR)/zip.txt -r .\
	$(call rm_dir, $(INSTALL_DIR)/)

zip:: | prepare_zip
zip:: magiclantern-$(VERSION).zip

TOP_COMPILE_H=$(TOP_DIR)/include/generated/compile.h

$(TOP_COMPILE_H): FORCE
	mkdir -p $(dir $@)
	$(call build,"VERSION",echo -e '#define ML_VERSION "$(VERSION)"') > $@
	@echo '#define ML_HG_ID "'`hg id`'"' >> $@
	@echo '#define ML_TIMESTAMP "'`$(DATE) "+%Y-%m-%d %H:%M:%S %Z"`'"' >> $@
	@echo '#define ML_COMPILE_BY "'`whoami`@`hostname`'"' >> $@

prepare: $(TOP_COMPILE_H)

clean::
	$(call rm_dir, zip)
	$(call rm_files, *.zip)

FORCE:

include $(TOP_DIR)/Makefile.obsolete
