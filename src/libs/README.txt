
This directory contains two types of the newlib libraries of which we only use libm.a.
  source: ftp://sources.redhat.com/pub/newlib/newlib-1.20.0.tar.gz

  
We compiled two versions of the library:

 a) One version with flags -O3 and
        export CFLAGS="-O3"
        make clean; make distclean; ./configure --target arm-elf --disable-newlib-supplied-syscalls && make
 b) another one with -O3 and -fPIC
        export CFLAGS="-fPIC -O3"
        make clean; make distclean; ./configure --target arm-elf --disable-newlib-supplied-syscalls && make
 
Both versions are supplied to make sure that Magic Lantern behaves the same no matter if compiled with or without -fPIC.

 

