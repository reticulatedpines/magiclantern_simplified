#!/usr/bin/env bash

SYSTEM=`uname`
echo "Setting up QEMU on $SYSTEM..."
 
if [[ $SYSTEM == "Darwin" ]]; then
    # Mac uses clang and Cocoa by default; it doesn't like SDL
    export CC=${CC:=clang}
    GUI_FLAGS="--disable-sdl"
else
    # gcc 4 uses gnu90 by default, but we use C99 code
    # passing C99 to g++ gives warning, but QEMU treats all warnings as errors
    # QEMU_CPPFLAGS is derived from QEMU_CFLAGS in ./configure
    export CC=${CC:=gcc --std=gnu99}

    if [[ $DISPLAY != "" ]]; then
        # gtk is recommended, but sdl works too
        GUI_FLAGS="--enable-gtk"
    fi
fi

if [[ $CC == clang* ]]; then
    # fixme: some warnings about format strings in conditional expressions
    export CXX=${CXX:=clang++}
    EXTRA_CFLAGS="-Wno-format-extra-args -Wno-format-zero-length"
    if [[ $CXX != clang++* ]]; then
        echo "Warning: not using clang++ (check CXX)"
    fi
fi

if [[ $CC == gcc* ]]; then
    # gcc 6 warns about readdir_r
    export CXX=${CXX:=g++}
    EXTRA_CFLAGS="-Wno-error=deprecated-declarations"
    if [[ $CXX != g++* ]]; then
        echo "Warning: not using g++ (check CXX)"
    fi
fi

echo "Using $CC / $CXX with $EXTRA_CFLAGS"
echo "Options: $GUI_FLAGS $*"

./configure --target-list=arm-softmmu --disable-docs --enable-vnc $GUI_FLAGS \
--extra-cflags="$EXTRA_CFLAGS" $*
