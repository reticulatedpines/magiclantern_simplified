SUPPORTED_INSTALLERS:= 550D.109 60D.111 600D.102 50D.109 500D.111 5D2.212 1100D.105 650D.101 6D.112

TOP_DIR=..

include $(TOP_DIR)/Makefile.top

make_installer_loop = @$(call make_loop, $(SUPPORTED_INSTALLERS), $(1))

fir:
	$(call make_installer_loop, installer)

clean:
	$(call make_installer_loop,clean)

clean_and_fir: clean fir

install_fir: fir
	$(call make_installer_loop, install)
