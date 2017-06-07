#!/usr/bin/env bash

# Export various stubs from ML, for QEMU's internal use.

[ "$0" = "$BASH_SOURCE" ] && echo "This script must be 'sourced':
   . $0 5D3.113" && exit

export QEMU_ML_PATH=
export QEMU_EOS_ML_MEMCPY=
export QEMU_EOS_ML_MEMCCPY=

if [ -d ../magic-lantern/platform/$1 ]; then
    export QEMU_ML_PATH=../magic-lantern/platform/$1
elif [ -d ../magic-lantern/$1 ]; then
    export QEMU_ML_PATH=../magic-lantern/$1
else
    echo "invalid ML path"
    return
fi

cd $QEMU_ML_PATH
export QEMU_EOS_ML_MEMCPY=`nm -a magiclantern | sort | grep " memcpy$" | cut -d " " -f 1`
export QEMU_EOS_ML_MEMCCPY=`nm -a magiclantern | sort | grep " memccpy$" | cut -d " " -f 1`
echo "memcpy $QEMU_EOS_ML_MEMCPY - $QEMU_EOS_ML_MEMCCPY"
cd $OLDPWD
