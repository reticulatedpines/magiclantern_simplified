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

all::
	$(MAKE) -C $(PLATFORM_PATH)/7D_MASTER.203

# special build_fir target
build_fir: all
	$(MAKE) -C $(PLATFORM_PATH)/7D_MASTER.203 autoexec-fir.bin
	python $(TOP_DIR)/../dumper/build_fir7.py -r \
		-s $(PLATFORM_PATH)/7D.203/autoexec-fir.bin \
		-m $(PLATFORM_PATH)/7D_MASTER.203/autoexec-fir.bin \
		$(TOP_DIR)/../dumper/7D000203.FIR \
		$(UPDATE_NAME_FIR)
