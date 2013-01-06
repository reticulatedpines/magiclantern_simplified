
This directory contains two types of the newlib libraries of which we only use libm.a.
  source: ftp://sources.redhat.com/pub/newlib/newlib-1.20.0.tar.gz

  
We compiled four versions of the library, one for each ARM_ABI (elf and none-eabi) and -fPIC combination:

 a) One version with flags -O3 and ARM_ABI=elf
        export CFLAGS="-O3"
        make clean; make distclean; ./configure --target arm-elf --disable-newlib-supplied-syscalls && make

 b) another one with -O3 and -fPIC and ARM_ABI=elf
        export CFLAGS="-fPIC -O3"
        make clean; make distclean; ./configure --target arm-elf --disable-newlib-supplied-syscalls && make
 
 c) another one with -O3 and ARM_ABI=none-eabi
        export CFLAGS="-O3"
        make clean; make distclean; ./configure --target arm-none-eabi --disable-newlib-supplied-syscalls && make

 d) the last one with -O3 and -fPIC and ARM_ABI=none-eabi
        export CFLAGS="-fPIC -O3"
        make clean; make distclean; ./configure --target arm-none-eabi --disable-newlib-supplied-syscalls && make

Both versions are supplied to make sure that Magic Lantern behaves the same no matter if compiled with or without -fPIC.

Position Independent Code (PIC) allows Magic Lantern to run properly regardless of its base address.
