Prebuilt C libraries linked into Magic Lantern
=============================================

ML is built with -nostdlib and does not use the toolchain's libc. Instead,
the build (platform/Makefile, installer/Makefile, modules/lua/Makefile)
extracts single objects from the archives in this directory and links those.
The rest of the usual libc comes from Canon's DryOS (via stubs) or from ML
itself (src/stdio.c, src/posix.c, src/mem.c ...).

The headers for this code are in include/libc (newlib 1.20.0, see below).
Headers and archives belong together: if a library here is replaced, its
headers in include/libc must be replaced as well.


newlib 1.20.0 - arm-none-eabi-O3/
---------------------------------

  source: https://sourceware.org/pub/newlib/newlib-1.20.0.tar.gz
  built:  export CFLAGS="-O3"
          ./configure --target arm-none-eabi --disable-newlib-supplied-syscalls
          make

Used:
  libc.a  only memcpy, memccpy, memset, setjmp / longjmp (and memmove, which
          is extracted but not linked: dietlibc's memmove comes first)
  libm.a  passed as is; the linker takes the math functions ML and its
          modules need (some forced in with -u in platform/Makefile)

The code is ARM mode with Thumb-1 interworking, built for the compiler's
default target, so the same objects run on every DIGIC generation.


dietlibc 0.33 - dietlib-0.33-none-eabi-Os/
------------------------------------------

  built with -Os, for ARM7TDMI / ARMv4T (ARM mode, Thumb-1 interworking);
  no build notes or source in this repository.

Used:
  dietlibc.a  string and number functions: strlen, strcpy, strcmp, strchr,
              strstr, strtol, strtoul, atoi, memcmp, memmove, errno ...
              (see DIET_OBJS in platform/Makefile)
  The lua module links its own set of dietlibc objects (its stdio, strtol,
  strtof, errno ...; see modules/lua/Makefile) and is built against
  dietlibc's headers in modules/lua/dietlibc/include.


Unused
------

arm-elf-O3/, arm-elf-O3-fPIC/ and arm-none-eabi-O3-fPIC/ are the same newlib
1.20.0 built for the old arm-elf ABI and/or with -fPIC. No Makefile uses
them any more.
