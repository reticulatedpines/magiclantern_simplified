#Makefile for 7D

MODEL=7D

include ../Makefile.platform

FIR-MASTER: autoexec-fir-master
FIR-SLAVE: autoexec-fir-slave

# autoexec-fir-master: CFLAGS += -DCONFIG_7D_FIR_MASTER
# autoexec-fir-slave: CFLAGS += -DCONFIG_7D_FIR_SLAVE

# autoexec-fir-master: clean autoexec-fir
	# $(call build,OBJCOPY,$(OBJCOPY) -O binary autoexec-fir $@.bin)
	# $(STAT_CMD) $@.bin;
	# $(READELF) -l autoexec-fir | grep -C 2 MemSiz
	# cp location.map ../../location.master

    
# autoexec-fir-slave: clean autoexec-fir
	# $(call build,OBJCOPY,$(OBJCOPY) -O binary autoexec-fir $@.bin)
	# $(STAT_CMD) $@.bin;
	# $(READELF) -l autoexec-fir | grep -C 2 MemSiz
