#build system for Magic Lantern.

#build and install are working, LUA not tested

#http://www.gnu.org/software/make/manual/make.html#Automatic-Variables
#http://www.gnu.org/software/make/manual/make.html#Variables_002fRecursion

TOP_DIR=$(PWD)
include Makefile.top

UNAME:=$(shell uname)

ifeq ($(UNAME), Darwin)
	# Variable declaration for Mac OS X
	UMOUNT=hdiutil unmount
	CF_CARD=/Volumes/EOS_DIGITAL
else
	# Default settings for remaining operating systems
	CF_CARD=/media/EOS_DIGITAL
	UMOUNT=umount
endif


all: 60D 550D 600D 50D 500D 5D2 1100D
	$(MAKE) -C $(PLATFORM_PATH)/all clean
	$(MAKE) -C $(PLATFORM_PATH)/all x
	$(MAKE) -C $(PLUGINS_DIR)

60D:
	$(MAKE) -C $(PLATFORM_PATH)/60D.111

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

7D:
	$(MAKE) -C $(PLATFORM_PATH)/7D.203

5DC:
	$(MAKE) -C $(PLATFORM_PATH)/5DC.111

plugins: FORCE
	$(MAKE) -C $(PLUGINS_DIR)

install: all
	mkdir -p $(CF_CARD)/ML/
	mkdir -p $(CF_CARD)/ML/settings/
	mkdir -p $(CF_CARD)/ML/data/
	mkdir -p $(CF_CARD)/ML/cropmks/
	cp platform/all/autoexec.bin $(CF_CARD)
	cp $(SRC_DIR)/FONTS.DAT $(CF_CARD)/ML/data/
	cp vram/*.lut $(CF_CARD)/ML/data/
	cp $(CROP_DIR)/*.bmp $(CF_CARD)/ML/cropmks/
	$(UMOUNT) $(CF_CARD)

fir:
	cd installer/550D.109/; $(MAKE) clean
	cd installer/60D.111/; $(MAKE) clean
	cd installer/600D.102/; $(MAKE) clean
	cd installer/50D.109/; $(MAKE) clean
	cd installer/500D.111/; $(MAKE) clean
	cd installer/5D2.212/; $(MAKE) clean
	cd installer/1100D.105/; $(MAKE) clean
	$(MAKE) -C installer/550D.109/
	$(MAKE) -C installer/60D.111/
	$(MAKE) -C installer/600D.102/
	$(MAKE) -C installer/50D.109/
	$(MAKE) -C installer/500D.111/
	$(MAKE) -C installer/5D2.212/
	$(MAKE) -C installer/1100D.105/

install_fir: fir
	cp installer/550D.109/ml-550d-109.fir $(CF_CARD)
	cp installer/60D.111/ml-60d-111.fir $(CF_CARD)
	cp installer/600D.102/ml-600d-102.fir $(CF_CARD)
	cp installer/50D.109/ml-50d-102.fir $(CF_CARD)
	cp installer/500D.111/ml-500d-111.fir $(CF_CARD)
	cp installer/5D2.212/ml-5D2-212.fir $(CF_CARD)
	cp installer/1100D.105/ml-1100d-105.fir $(CF_CARD)

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
	cd $(PLATFORM_PATH)/60D.111/; $(MAKE) clean
	cd $(PLATFORM_PATH)/600D.102/; $(MAKE) clean
	cd $(PLATFORM_PATH)/7D.203/; $(MAKE) clean
	cd $(PLATFORM_PATH)/50D.109/; $(MAKE) clean
	cd $(PLATFORM_PATH)/500D.111/; $(MAKE) clean
	cd $(PLATFORM_PATH)/5D2.212/; $(MAKE) clean
	cd $(PLATFORM_PATH)/5DC.111/; $(MAKE) clean
	cd $(PLATFORM_PATH)/1100D.105/; $(MAKE) clean
	$(MAKE) -C $(PLUGINS_DIR) clean
	$(RM) -rf  $(BINARIES_PATH)

zip: all
	cd $(PLATFORM_PATH)/all; $(MAKE) docs
	cd $(PLATFORM_PATH)/all; $(MAKE) zip

docs:
	cd $(PLATFORM_PATH)/all; $(MAKE) docs

docq:
	cd $(PLATFORM_PATH)/all; $(MAKE) docq

dropbox: all
	cp $(PLATFORM_PATH)/all/autoexec.bin ~/Dropbox/Public/bleeding-edge/

FORCE:
