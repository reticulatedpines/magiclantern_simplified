#build system for Magic Lantern.

#build and install are working, LUA not tested

#http://www.gnu.org/software/make/manual/make.html#Automatic-Variables
#http://www.gnu.org/software/make/manual/make.html#Variables_002fRecursion

TOP_DIR=$(PWD)
include Makefile.top
include Makefile.user.default
-include Makefile.user

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


all: $(SUPPORTED_MODELS)
	$(MAKE) -C $(PLATFORM_PATH)/all clean
	$(MAKE) -C $(PLATFORM_PATH)/all x
	#~ $(MAKE) -C $(PLUGINS_DIR)

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

7D_MASTER:
	$(MAKE) -C $(PLATFORM_PATH)/7D_MASTER.203
    
7D: 7D_MASTER
	$(MAKE) -C $(PLATFORM_PATH)/7D.203

7DFIR: 7D_MASTER 7D
	dd if=$(PLATFORM_PATH)/7D.203/autoexec.bin of=$(PLATFORM_PATH)/7D.203/autoexec.fir bs=288 skip=1 >/dev/null 2>&1
	dd if=$(PLATFORM_PATH)/7D_MASTER.203/autoexec.bin of=$(PLATFORM_PATH)/7D_MASTER.203/autoexec.fir bs=288 skip=1 >/dev/null 2>&1
	./build_fir7.py -r -s $(PLATFORM_PATH)/7D.203/autoexec.fir -m $(PLATFORM_PATH)/7D_MASTER.203/autoexec.fir $(PLATFORM_PATH)/7D.203/7D000203.FIR $(PLATFORM_PATH)/7D.203/MAGIC.FIR >/dev/null

5DC:
	$(MAKE) -C $(PLATFORM_PATH)/5DC.111

40D:
	$(MAKE) -C $(PLATFORM_PATH)/40D.111

5D3:
	$(MAKE) -C $(PLATFORM_PATH)/5D3.113

EOSM:
	$(MAKE) -C $(PLATFORM_PATH)/EOSM.106

650D:
	$(MAKE) -C $(PLATFORM_PATH)/650D.101

6D:
	$(MAKE) -C $(PLATFORM_PATH)/6D.113

plugins: FORCE
	$(MAKE) -C $(PLUGINS_DIR)

install: all
	mkdir -p $(CF_CARD)/ML/
	mkdir -p $(CF_CARD)/ML/settings/
	mkdir -p $(CF_CARD)/ML/data/
	mkdir -p $(CF_CARD)/ML/cropmks/
	cp $(PLATFORM_PATH)/all/autoexec.bin $(CF_CARD)
	cp $(SRC_DIR)/FONTS.DAT $(CF_CARD)/ML/data/
	cp $(VRAM_DIR)/*.lut $(CF_CARD)/ML/data/
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
	cd installer/EOSM.106/; $(MAKE) clean
	cd installer/650D.101/; $(MAKE) clean
	cd installer/6D.112/; $(MAKE) clean
	$(MAKE) installer -C installer/550D.109/
	$(MAKE) installer -C installer/60D.111/
	$(MAKE) installer -C installer/600D.102/
	$(MAKE) installer -C installer/50D.109/
	$(MAKE) installer -C installer/500D.111/
	$(MAKE) installer -C installer/5D2.212/
	$(MAKE) installer -C installer/1100D.105/
	$(MAKE) installer -C installer/EOSM.106/
	$(MAKE) installer -C installer/650D.101/
	$(MAKE) installer -C installer/6D.112/

install_fir: fir
	cp installer/550D.109/$(UPDATE_NAME) $(CF_CARD)
	cp installer/60D.111/$(UPDATE_NAME) $(CF_CARD)
	cp installer/600D.102/$(UPDATE_NAME) $(CF_CARD)
	cp installer/50D.109/$(UPDATE_NAME) $(CF_CARD)
	cp installer/500D.111/$(UPDATE_NAME) $(CF_CARD)
	cp installer/5D2.212/$(UPDATE_NAME) $(CF_CARD)
	cp installer/1100D.105/$(UPDATE_NAME) $(CF_CARD)
	cp installer/EOSM.106/$(UPDATE_NAME) $(CF_CARD)
	cp installer/650D.101/$(UPDATE_NAME) $(CF_CARD)
	cp installer/6D.112/$(UPDATE_NAME) $(CF_CARD)

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
	cd $(PLATFORM_PATH)/7D_MASTER.203/; $(MAKE) clean
	cd $(PLATFORM_PATH)/7D.203/; $(MAKE) clean
	cd $(PLATFORM_PATH)/50D.109/; $(MAKE) clean
	cd $(PLATFORM_PATH)/500D.111/; $(MAKE) clean
	cd $(PLATFORM_PATH)/5D2.212/; $(MAKE) clean
	cd $(PLATFORM_PATH)/5D3.113/; $(MAKE) clean
	cd $(PLATFORM_PATH)/5DC.111/; $(MAKE) clean
	cd $(PLATFORM_PATH)/1100D.105/; $(MAKE) clean
	cd $(PLATFORM_PATH)/40D.111/; $(MAKE) clean
	cd $(PLATFORM_PATH)/EOSM.106/; $(MAKE) clean	
	cd $(PLATFORM_PATH)/650D.101/; $(MAKE) clean
	cd $(PLATFORM_PATH)/6D.113/; $(MAKE) clean	
	$(MAKE) -C $(PLUGINS_DIR) clean
	$(RM) -rf  $(BINARIES_PATH)
	$(RM) -rf doxygen-doc/*

zip: all
	cd $(PLATFORM_PATH)/all; $(MAKE) docs
	cd $(PLATFORM_PATH)/all; $(MAKE) zip

docs:
	cd $(PLATFORM_PATH)/all; $(MAKE) docs

docq:
	cd $(PLATFORM_PATH)/all; $(MAKE) docq

doxygen:
	doxygen

dropbox: all
	cp $(PLATFORM_PATH)/all/autoexec.bin ~/Dropbox/Public/bleeding-edge/


# for changelog
HG_TEMPLATE=--template '{node|short} | {author|user}: {desc|strip|firstline} \n'
HG_DATE=`date -d '$(1)' +'%Y-%m-%d %H:%M:%S'`
HG_DATE_RANGE=--date "$(call HG_DATE, $(1)) to $(call HG_DATE, $(2))"
HG_CHANGESET_BEFORE_DATE=$(shell hg log --date "<$(call HG_DATE, $(1) )" --template '{date|localdate}:{node|short} \n' | sort | tail -1 | cut -d: -f2 )

DIFFSTAT_FILTER=python -c 'import sys; \
	from textwrap import wrap; \
	L = sys.stdin.readlines() or ["No changes."]; \
	L,last = L[:-1],L[-1]; \
	L.sort(key=lambda l: -len(l)); \
	F = [l.split("|")[0].strip() for l in L]; \
	brk = max(len(L)/20, 3); \
	sys.stdout.writelines(L[:brk]); \
	sys.stdout.write("\n ".join(wrap(", ".join(F[brk:]), width=100, break_on_hyphens=False, break_long_words=False, initial_indent=" "))); \
	sys.stdout.write("\n\n" if brk < len(L) else ""); \
	sys.stdout.write(last); \
	'

# today's changes are considered in last 24 hours, before compilation time
# yesterday changes: between 24 and 48 hours
changelog:
	echo "Change log for magiclantern-$(VERSION)zip" > ChangeLog.txt
	echo "compiled from https://bitbucket.org/hudson/magic-lantern/changeset/`hg id -i -r .` " >> ChangeLog.txt
	echo "===============================================================================" >> ChangeLog.txt
	echo "" >> ChangeLog.txt
	echo "Today's changes:" >> ChangeLog.txt
	echo "----------------" >> ChangeLog.txt
	echo "" >> ChangeLog.txt
	hg log $(call HG_DATE_RANGE, today - 1 days, today) $(HG_TEMPLATE) >> ChangeLog.txt ;
	echo "" >> ChangeLog.txt
	COLUMNS=80 hg diff --stat -r $(call HG_CHANGESET_BEFORE_DATE, today - 1 days ) -r tip | $(DIFFSTAT_FILTER) >> ChangeLog.txt ;
	echo "" >> ChangeLog.txt
	echo "" >> ChangeLog.txt
	echo "Yesterday's changes:" >> ChangeLog.txt
	echo "--------------------" >> ChangeLog.txt
	echo "" >> ChangeLog.txt
	hg log $(call HG_DATE_RANGE, today - 2 days, today - 1 days) $(HG_TEMPLATE) >> ChangeLog.txt ;
	echo "" >> ChangeLog.txt
	COLUMNS=80 hg diff --stat -r $(call HG_CHANGESET_BEFORE_DATE, today - 2 days) -r $(call HG_CHANGESET_BEFORE_DATE, today - 1 days) | $(DIFFSTAT_FILTER) >> ChangeLog.txt ;
	echo "" >> ChangeLog.txt
	echo "" >> ChangeLog.txt
	echo "Changes for last 30 days:" >> ChangeLog.txt
	echo "-------------------------" >> ChangeLog.txt
	echo "" >> ChangeLog.txt
	hg log $(call HG_DATE_RANGE, today - 30 days, today - 2 days) $(HG_TEMPLATE) >> ChangeLog.txt ;
	echo "" >> ChangeLog.txt
	COLUMNS=80 hg diff --stat -r $(call HG_CHANGESET_BEFORE_DATE, today - 30 days) -r $(call HG_CHANGESET_BEFORE_DATE, today - 2 days) | $(DIFFSTAT_FILTER) >> ChangeLog.txt ;
 
nightly: clean all changelog
	mkdir -p $(NIGHTLY_DIR)
	cd $(PLATFORM_PATH)/all; $(MAKE) zip
	cd $(PLATFORM_PATH)/all; mv *.zip $(NIGHTLY_DIR)
	touch build.log
	mv build.log $(NIGHTLY_DIR)
	mv ChangeLog.txt $(NIGHTLY_DIR)
	-unlink $(NIGHTLY_ROOT)/build.log
	-unlink $(NIGHTLY_ROOT)/ChangeLog.txt
	-unlink $(NIGHTLY_ROOT)/magiclantern*.zip
	ln -s $(NIGHTLY_DIR)/build.log $(NIGHTLY_ROOT)
	ln -s $(NIGHTLY_DIR)/ChangeLog.txt $(NIGHTLY_ROOT)
	ln -s $(NIGHTLY_DIR)/magiclantern*.zip $(NIGHTLY_ROOT)

FORCE:

