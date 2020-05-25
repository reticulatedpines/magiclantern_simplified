# Extra rules added to Makefile.setup

all:: $(_CONFIGS_to_compile_rules)

_CONFIGS_compile_name = _compile
_CONFIGS_clean_name = _clean
_CONFIGS_install_name = _install

_CONFIGS_to_filter_out = CONFIG_QEMU

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

# some installation media may fail when copying files in parallel
# broken drivers or controller? forcing sequential installation solves the issue...
# fixme: $(_CONFIGS_install) (currently just CONFIG_MODULES_install) does not run $(INSTALL_FINISH)
# it must be executed only once at the end (to unmount the installation media)
install:: prepare_install_dir install_extra_data
	for target in $(_CONFIGS_install); do $(MAKE) $$target; done
	$(INSTALL_FINISH)

install_extra_data: prepare_install_dir
	$(CP) $(FONT_DIR)/*.rbf $(INSTALL_FONTS_DIR)/
	$(CP) $(VRAM_DIR)/*.lut $(INSTALL_DATA_DIR)/
	$(CP) $(CROP_DIR)/*.bmp $(INSTALL_CROPMARKS_DIR)/
	$(INSTALL_ALL_SCRIPTS)

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
	$(eval INSTALL_DIR = $(PWD_DIR)/zip)
	$(eval INSTALL_PREPARE = )
	$(eval INSTALL_FINISH = )
	$(eval WIFI_SD = )
	$(eval export INSTALL_DIR)
	$(eval export INSTALL_MODULES_DIR)
	$(eval export INSTALL_PREPARE)
	$(eval export INSTALL_FINISH)
	$(eval export WIFI_SD)
	$(call rm_dir, $(INSTALL_DIR)/)
	$(call rm_files, $(PLATFORM_PATH)/*/magiclantern.sym) # only check module dependencies on current camera model
	$(warning removing $(PLATFORM_PATH)/*/magiclantern.sym)
	mkdir -p $(INSTALL_DIR)

# zip.txt must be the first item on the list!
magiclantern-$(VERSION).zip: install installer_check installdoc minimal_check
	-$(call rm_files, $@)
	chmod -x $(INSTALL_DIR)/autoexec.bin
	cd $(INSTALL_DIR); zip -z ../$@ < \
		../$(SRC_DIR)/zip.txt -r .
	$(call rm_dir, $(INSTALL_DIR)/)

zip:: | prepare_zip
zip:: magiclantern-$(VERSION).zip print_build_info

install_qemu:: | prepare_zip
install_qemu:: install
	$(call build,MTOOLS,cd $(QEMU_PATH)/; ./mtools_copy_ml.sh $(INSTALL_DIR))

clean::
	$(call rm_dir, zip)
	$(call rm_files, *.zip)

FORCE:

include $(TOP_DIR)/Makefile.obsolete
