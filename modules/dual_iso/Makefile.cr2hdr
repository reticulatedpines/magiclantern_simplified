# Makefile for the cr2hdr software

CR2HDR_BIN=cr2hdr
HOSTCC=$(HOST_CC)
CR2HDR_CFLAGS=-m32 -mno-ms-bitfields -O2 -Wall -I$(SRC_DIR) -D_FILE_OFFSET_BITS=64 -fno-strict-aliasing -msse -msse2 -std=gnu99
CR2HDR_LDFLAGS=-lm -m32 
CR2HDR_DEPS=$(SRC_DIR)/chdk-dng.c dcraw-bridge.c exiftool-bridge.c adobedng-bridge.c amaze_demosaic_RT.c dither.c timing.c kelvin.c
HOST=host

# Find the latest version of exiftool
EXIFTOOL_VERS=$(shell echo $$(curl -s https://www.sno.phy.queensu.ca/~phil/exiftool/ | grep -Em 1 'Download|Version' | grep -Eo '[0-9]+([.][0-9]+)?') | cut -d ' ' -f 1)

ifdef CROSS
	HOSTCC=$(MINGW_GCC)
	CR2HDR_BIN=cr2hdr.exe
endif

$(CR2HDR_BIN): cr2hdr.c $(CR2HDR_DEPS) $(MODULE_STRINGS)
	$(call build,$(notdir $(HOSTCC)),$(HOSTCC) $(CR2HDR_CFLAGS) cr2hdr.c $(CR2HDR_DEPS) -o $@ $(CR2HDR_LDFLAGS))

$(CR2HDR_BIN).exe: cr2hdr.c $(CR2HDR_DEPS) $(MODULE_STRINGS)
	CROSS=1 $(MAKE) $@

clean::
	$(call rm_files, cr2hdr cr2hdr.exe dcraw dcraw.c dcraw.exe exiftool.exe exiftool.tar.gz exiftool exiftool.zip cr2hdr.zip cr2hdr-win.zip cr2hdr-win_exiftool-perl-script.zip)
	rm -rf lib

dcraw.c:
	wget https://www.cybercom.net/~dcoffin/dcraw/dcraw.c
# Sometimes there is a problem with this domain's issuer authority--go insecure if you dare.
#	wget --no-check-certificate https://www.cybercom.net/~dcoffin/dcraw/dcraw.c
# Alternate site in case cybercom.net is completely down
#	wget https://raw.githubusercontent.com/LibRaw/LibRaw/master/dcraw/dcraw.c

dcraw: dcraw.c
# This works on Mac and Linux to create a dcraw binary
# It also works on Cygwin but it requires cygwin1.dll.
# Does not work on msys/MinGW or msys2/i686-w64-mingw32.
	$(call build,$(notdir $(HOSTCC)),$(HOSTCC) -o dcraw -O4 dcraw.c -lm -DNODEPS )

dcraw.exe: dcraw.c
# This works on Mac, Linux, msys2/i686-w64-mingw32 and Cygwin/i686-w64-mingw32.
# Does not work on msys/MinGW.
	$(call build,$(notdir $(MINGW_GCC)),$(MINGW_GCC) -o dcraw.exe -O4 dcraw.c -lm -lws2_32 -DNODEPS )

exiftool.tar.gz:
# Mac and Linux
	wget http://www.sno.phy.queensu.ca/~phil/exiftool/Image-ExifTool-$(EXIFTOOL_VERS).tar.gz -O exiftool.tar.gz

exiftool.zip:
# Windows
	wget http://www.sno.phy.queensu.ca/~phil/exiftool/exiftool-$(EXIFTOOL_VERS).zip -O exiftool.zip

exiftool: exiftool.tar.gz
# exiftool perl script - Mac and Linux
	tar -zxf exiftool.tar.gz Image-ExifTool-$(EXIFTOOL_VERS)/exiftool Image-ExifTool-$(EXIFTOOL_VERS)/lib
	mv Image-ExifTool-$(EXIFTOOL_VERS)/exiftool exiftool
	mv Image-ExifTool-$(EXIFTOOL_VERS)/lib lib
	rm -rf Image-ExifTool-$(EXIFTOOL_VERS)

exiftool.exe: exiftool.zip
# Windows binary
	unzip exiftool.zip
	mv exiftool\(-k\).exe exiftool.exe

cr2hdr.zip: cr2hdr dcraw exiftool
# Use for Mac and Linux
#
# Can also can be used with msys2/MinGW-64 and Cygwin/MinGW if exiftool perl script is preferred.
# - run “make cr2hdr.exe && make dcraw.exe && make cr2hdr.zip”
ifeq ($(OS),Windows_NT)
	zip -rq cr2hdr-win_exiftool-perl-script cr2hdr.exe dcraw.exe exiftool lib
else
	zip -rq $@ $< $^ lib
endif

cr2hdr-win.zip: cr2hdr.exe dcraw.exe exiftool.exe
# Windows
	zip $@ $< $^
