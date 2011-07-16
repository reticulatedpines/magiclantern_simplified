#build system for Magic Lantern.

#build and install are working, LUA not tested

#http://www.gnu.org/software/make/manual/make.html#Automatic-Variables
#http://www.gnu.org/software/make/manual/make.html#Variables_002fRecursion

TOP_DIR=$(PWD)
include Makefile.top

all: 60D 550D 600D

60D:
	$(MAKE) -C $(PLATFORM_PATH)/60D.109 

550D:
	$(MAKE) -C $(PLATFORM_PATH)/550D.109 

600D:
	$(MAKE) -C $(PLATFORM_PATH)/600D.101


install_60D: $(PLATFORM_PATH)/60D.109/autoexec.bin
	cd $(PLATFORM_PATH)/60D.109; $(MAKE) install

install_550D: $(PLATFORM_PATH)/550D.109/autoexec.bin
	cd $(PLATFORM_PATH)/550D.109; $(MAKE) install

install_600D: $(PLATFORM_PATH)/600D.109/autoexec.bin
	cd $(PLATFORM_PATH)/600D.109; $(MAKE) install

clean:
	-$(RM) \
		$(SRC_DIR)/*.o \
		$(SRC_DIR)/.*.d \
		magiclantern.lds \
		$(LUA_PATH)/*.o \
		$(LUA_PATH)/.*.d \
		*.pdf 
	cd $(PLATFORM_PATH)/550D.109/; $(MAKE) clean
	cd $(PLATFORM_PATH)/60D.109/; $(MAKE) clean
	cd $(PLATFORM_PATH)/600D.101/; $(MAKE) clean
	$(RM) -rf  $(BINARIES_PATH)

zip:
	cd $(PLATFORM_PATH)/550D.109/; $(MAKE) zip
	cd $(PLATFORM_PATH)/60D.109/; $(MAKE) zip
	cd $(PLATFORM_PATH)/600D.101/; $(MAKE) zip

dropbox: all
	cp $(PLATFORM_PATH)/550D.109/autoexec.bin ~/Dropbox/Public/bleeding-edge/550d/
	cp $(PLATFORM_PATH)/60D.109/autoexec.bin ~/Dropbox/Public/bleeding-edge/60d/
	cp $(PLATFORM_PATH)/600D.101/autoexec.bin ~/Dropbox/Public/bleeding-edge/600d/
