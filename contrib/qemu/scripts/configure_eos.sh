#!/bin/bash

# gcc 6 warns about readdir_r
# gcc 4 uses gnu90 by default, but we use C99 code
CFLAGS="-Wno-error=deprecated-declarations --std=gnu99" \
    ./configure --target-list=arm-softmmu --disable-docs --enable-sdl

