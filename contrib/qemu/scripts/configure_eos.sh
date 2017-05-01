#!/bin/bash

# gcc 6 warns about readdir_r
# gcc 4 uses gnu90 by default, but we use C99 code
# passing C99 to g++ gives warning, but QEMU treats all warnings as errors
# QEMU_CPPFLAGS is derived from QEMU_CFLAGS in ./configure
# gcov is enabled by default (no obvious difference in emulation speed)
CC="gcc --std=gnu99" \
    ./configure --target-list=arm-softmmu --disable-docs --enable-sdl \
    --extra-cflags="-Wno-error=deprecated-declarations --coverage" $*
