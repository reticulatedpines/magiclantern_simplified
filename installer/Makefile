SUPPORTED_INSTALLERS := 1100D.105 500D.111 50D.109 550D.109 5D2.212 5D3.113 60D.111 600D.102 650D.104 6D.116 700D.115 EOSM.202 7D.203
# this need some more work: 5DC.111

TOP_DIR=..

include $(TOP_DIR)/Makefile.top

INSTALLER_CLEAN_ALL_TARGETS = $(call targets_rules, $(SUPPORTED_INSTALLERS), clean)

INSTALLER_INSTALLER_TARGETS = $(call targets_rules, $(SUPPORTED_INSTALLERS), build_fir)

INSTALLER_INSTALL_TARGETS = $(call targets_rules, $(SUPPORTED_INSTALLERS), install)

fir: $(INSTALLER_INSTALLER_TARGETS)

clean: $(INSTALLER_CLEAN_ALL_TARGETS)

clean_and_fir: | clean
clean_and_fir: fir

install_fir: $(INSTALLER_INSTALLER_TARGETS)


$(INSTALLER_INSTALL_TARGETS) $(INSTALLER_INSTALLER_TARGETS) $(INSTALLER_CLEAN_ALL_TARGETS) $(SUPPORTED_INSTALLERS):
	$(call call_make_rule, $(TOP_DIR)/installer)

.PHONY: $(INSTALLER_INSTALLER_TARGETS) $(INSTALLER_INSTALL_TARGETS) $(INSTALLER_CLEAN_ALL_TARGETS) $(SUPPORTED_INSTALLERS)

# we want ML installers to be built sequentially, to avoid conflicts
# => use .NOTPARALLEL in the upper-level Makefiles only
# parallel build is still used within each platform
.NOTPARALLEL:
