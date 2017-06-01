#!/usr/bin/env bash

# Export various stubs from ML, for QEMU's internal use.

[ "$0" = "$BASH_SOURCE" ] && echo "This script must be 'sourced':
   . $0 5D3.113" && exit

export QEMU_ML_PATH=
export QEMU_EOS_ML_MEMCPY=
export QEMU_EOS_ML_MEMCCPY=
export QEMU_EOS_DEBUGMSG=

if [ -d ../magic-lantern/platform/$1 ]; then
    export QEMU_ML_PATH=../magic-lantern/platform/$1
elif [ -d ../magic-lantern/$1 ]; then
    export QEMU_ML_PATH=../magic-lantern/$1
else
    echo "invalid ML path"
    return
fi

echo "Symbols from $QEMU_ML_PATH"
cd $QEMU_ML_PATH
export QEMU_EOS_ML_MEMCPY=`nm -a magiclantern | sort | grep " memcpy$" | cut -d " " -f 1`
export QEMU_EOS_ML_MEMCCPY=`nm -a magiclantern | sort | grep " memccpy$" | cut -d " " -f 1`
export QEMU_EOS_DEBUGMSG=`nm -a magiclantern | sort | grep " DryosDebugMsg$" | cut -d " " -f 1`
echo "memcpy $QEMU_EOS_ML_MEMCPY - $QEMU_EOS_ML_MEMCCPY"
echo "DebugMsg $QEMU_EOS_DEBUGMSG"
cd $OLDPWD
