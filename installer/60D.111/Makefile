#Makefile for 60D firmware 110

TOP_DIR=../..
include $(TOP_DIR)/Makefile.top

#must be defined first because they are used in Makefile.inc, for $(VERSION) for example
FW_VERSION=111
MODEL=60D
UPDATE_NAME=60D-111.fir

ML_FIR=ml-60D-111.fir

include $(TOP_DIR)/installer/Makefile.installer.default

# DryOSmemory map
# RESTARTSTART is selected to be just above the end of the bss
#
ROMBASEADDR		= 0xFF010000
RESTARTSTART		= 0x0005f000

#not used but...
FIRMWARE_ID		= 0x80000286

updater: reboot-ins.o
	$(call build,LD,$(LD) \
		-o $@ \
		-nostdlib \
		-march=armv5te \
		-e _start \
		-Ttext 0x800000 \
		$^ \
	); \

installer: magiclantern.bin updater.bin
	python ../../../dumper/build_fir.py -r ../../../dumper/60D00111.FIR updater.bin $(UPDATE_NAME)

clean:
	rm -f *.fir *.o autoexec* magiclantern* updater* ml.bin

install:
	cp $(UPDATE_NAME) /media/EOS_DIGITAL/ && umount /media/EOS_DIGITAL 
