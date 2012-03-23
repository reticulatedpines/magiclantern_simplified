#build system for Magic Lantern.

#build and install are working, LUA not tested

#http://www.gnu.org/software/make/manual/make.html#Automatic-Variables
#http://www.gnu.org/software/make/manual/make.html#Variables_002fRecursion

TOP_DIR=$(PWD)
include Makefile.top

all: 60D 550D 600D 50D 500D 5D2 plugins
	$(MAKE) -C $(PLATFORM_PATH)/all clean
	$(MAKE) -C $(PLATFORM_PATH)/all x

60D:
	$(MAKE) -C $(PLATFORM_PATH)/60D.110

550D:
	$(MAKE) -C $(PLATFORM_PATH)/550D.109

600D:
	$(MAKE) -C $(PLATFORM_PATH)/600D.102

1100D:
	$(MAKE) -C $(PLATFORM_PATH)/1100D.105

50D:
	$(MAKE) -C $(PLATFORM_PATH)/50D.109

500D:
	$(MAKE) -C $(PLATFORM_PATH)/500D.111

5D2:
	$(MAKE) -C $(PLATFORM_PATH)/5D2.212

plugins: FORCE
	$(MAKE) -C $(PLUGINS_DIR)

install: all
	cp platform/all/autoexec.bin /media/EOS_DIGITAL/
	cp $(SRC_DIR)/FONTS.DAT /media/EOS_DIGITAL
	cp vram/rectilin.lut /media/EOS_DIGITAL
	umount /media/EOS_DIGITAL

fir:
	cd installer/550D.109/; $(MAKE) clean
	cd installer/60D.110/; $(MAKE) clean
	cd installer/600D.102/; $(MAKE) clean
	cd installer/50D.109/; $(MAKE) clean
	cd installer/500D.111/; $(MAKE) clean
	cd installer/5D2.212/; $(MAKE) clean
	$(MAKE) -C installer/550D.109/
	$(MAKE) -C installer/60D.110/
	$(MAKE) -C installer/600D.102/
	$(MAKE) -C installer/50D.109/
	$(MAKE) -C installer/500D.111/
	$(MAKE) -C installer/5D2.212/

install_fir: fir
	cp installer/550D.109/ml-550d-109.fir /media/EOS_DIGITAL/
	cp installer/60D.110/ml-60d-110.fir /media/EOS_DIGITAL/
	cp installer/600D.102/ml-600d-102.fir /media/EOS_DIGITAL/
	cp installer/50D.109/ml-50d-102.fir /media/EOS_DIGITAL/
	cp installer/500D.111/ml-500d-111.fir /media/EOS_DIGITAL/
	cp installer/5D2.212/ml-5D2-212.fir /media/EOS_DIGITAL/

clean:
	$(call build,CLEAN,$(RM) -f \
		$(SRC_DIR)/*.o \
		$(SRC_DIR)/.*.d \
		magiclantern.lds \
		$(LUA_PATH)/*.o \
		$(LUA_PATH)/.*.d \
		$(LUA_PATH)/liblua.a \
		*.pdf)
	$(RM) -f \
		$(LUA_PATH)/*.o \
		$(LUA_PATH)/.*.d \
		$(LUA_PATH)/liblua.a
	cd $(PLATFORM_PATH)/550D.109/; $(MAKE) clean
	cd $(PLATFORM_PATH)/60D.110/; $(MAKE) clean
	cd $(PLATFORM_PATH)/600D.102/; $(MAKE) clean
	cd $(PLATFORM_PATH)/50D.109/; $(MAKE) clean
	cd $(PLATFORM_PATH)/500D.111/; $(MAKE) clean
	cd $(PLATFORM_PATH)/5D2.212/; $(MAKE) clean
	$(MAKE) -C $(PLUGINS_DIR) clean
	$(RM) -rf  $(BINARIES_PATH)

zip: all
	cd $(PLATFORM_PATH)/all; $(MAKE) docs
	cd $(PLATFORM_PATH)/all; $(MAKE) zip
	cd $(PLATFORM_PATH)/all; $(MAKE) zip

dropbox: all
	cp $(PLATFORM_PATH)/all/autoexec.bin ~/Dropbox/Public/bleeding-edge/

FORCE:
