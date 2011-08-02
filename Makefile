#build system for Magic Lantern.

#build and install are working, LUA not tested

#http://www.gnu.org/software/make/manual/make.html#Automatic-Variables
#http://www.gnu.org/software/make/manual/make.html#Variables_002fRecursion

TOP_DIR=$(PWD)
include Makefile.top

all: 60D 550D 600D
	$(MAKE) -C $(PLATFORM_PATH)/all clean
	$(MAKE) -C $(PLATFORM_PATH)/all x

60D:
	$(MAKE) -C $(PLATFORM_PATH)/60D.110 

550D:
	$(MAKE) -C $(PLATFORM_PATH)/550D.109 

600D:
	$(MAKE) -C $(PLATFORM_PATH)/600D.101


install: all
	cp platform/all/autoexec.bin /media/EOS_DIGITAL/
	umount /media/EOS_DIGITAL

install_fir:
	$(MAKE) -C installer/550D.109/
	$(MAKE) -C installer/60D.110/
	$(MAKE) -C installer/600D.101/
	cp installer/550D.109/ml-550d-109.fir /media/EOS_DIGITAL/
	cp installer/60D.110/ml-60d-110.fir /media/EOS_DIGITAL/
	cp installer/600D.101/ml-600d-101.fir /media/EOS_DIGITAL/

clean:
	-$(RM) \
		$(SRC_DIR)/*.o \
		$(SRC_DIR)/.*.d \
		magiclantern.lds \
		$(LUA_PATH)/*.o \
		$(LUA_PATH)/.*.d \
		*.pdf 
	cd $(PLATFORM_PATH)/550D.109/; $(MAKE) clean
	cd $(PLATFORM_PATH)/60D.110/; $(MAKE) clean
	cd $(PLATFORM_PATH)/600D.101/; $(MAKE) clean
	$(RM) -rf  $(BINARIES_PATH)

zip:
	cd $(PLATFORM_PATH)/550D.109/; $(MAKE) zip
	cd $(PLATFORM_PATH)/60D.110/; $(MAKE) zip
	cd $(PLATFORM_PATH)/600D.101/; $(MAKE) zip

dropbox: all
	cp $(PLATFORM_PATH)/550D.109/autoexec.bin ~/Dropbox/Public/bleeding-edge/550d/
	cp $(PLATFORM_PATH)/60D.110/autoexec.bin ~/Dropbox/Public/bleeding-edge/60d/
	cp $(PLATFORM_PATH)/600D.101/autoexec.bin ~/Dropbox/Public/bleeding-edge/600d/
