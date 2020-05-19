 ###################################################################################
 #                                                                                 #
 #                          _____     _       _                                    #
 #                         |  ___|   | |     | |                                   #
 #                         |___ \  __| |_ __ | |_   _ ___                          #
 #                             \ \/ _` | '_ \| | | | / __|                         #
 #                         /\__/ / (_| | |_) | | |_| \__ \                         #
 #                         \____/ \__,_| .__/|_|\__,_|___/                         #
 #                                     | |                                         #
 #                                     |_|                                         #
 #                                                                                 #
 ###################################################################################

name=AUTOEXEC

CFLAGS=-nostdlib -march=armv5te -mthumb-interwork
CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
AR=arm-none-eabi-ar
OBJCOPY=arm-none-eabi-objcopy


S_OBJS = entry.o

C_OBJS = bootdisk.o

OBJS   = $(S_OBJS) $(C_OBJS)

all: $(name).BIN

$(name).BIN: $(name).arm.elf
	$(OBJCOPY) -O binary $(name).arm.elf $(name).BIN
	rm -f $(name).arm.elf
	cp $(name).BIN ./build_scripts
	@echo;echo;ls -l AUTOEXEC.BIN

$(name).arm.elf:$(OBJS) link.script
	$(CC) $(CFLAGS) -Wl,-T,link.script -o$@ $^

clean:
	rm -f *.o
	rm -f *.d
	rm -f *.a
	rm -f *.elf
	rm -f .*.o.d