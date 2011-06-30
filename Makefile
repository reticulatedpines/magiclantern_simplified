MAKE=make
RM=rm
MV=mv
MKDIR=mkdir

BINARIES_PATH=binaries
LUA_PATH=

#todo: 
# put sources in src/
# unused things in legacy/
# put Makefile.550 and .60 in their platform dir and do: 
#  cd platform/550D.109; make 
#  -> no .o generated in rootdir. and .o in right platform

all: clean 60D clean 550D

60D: Makefile.60
	$(MAKE) -f Makefile.60
	$(MKDIR) -p $(BINARIES_PATH)/60D.109
	$(MV) autoexec.bin $(BINARIES_PATH)/60D.109

550D: Makefile.550
	$(MAKE) -f Makefile.550
	$(MKDIR) -p $(BINARIES_PATH)/550D.109
	$(MV) autoexec.bin $(BINARIES_PATH)/550D.109

clean:
	-$(RM) \
		*.o \
		*.a \
		.*.d \
		magiclantern.lds \
		$(LUA_PATH)/*.o \
		$(LUA_PATH)/.*.d \
		*.pdf \
	$(RM) -rf  $(BINARIES_PATH)
