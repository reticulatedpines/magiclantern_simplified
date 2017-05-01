#!/bin/bash

# Export various stubs from ML, for QEMU's internal use.

[ "$0" = "$BASH_SOURCE" ] && echo "This script must be 'sourced':
   . $0 5D3.113" && exit

cd ../magic-lantern/platform/$1
export QEMU_EOS_ML_MEMCPY=`nm -a magiclantern | sort | grep " memcpy$" | cut -d " " -f 1`
export QEMU_EOS_ML_MEMCCPY=`nm -a magiclantern | sort | grep " memccpy$" | cut -d " " -f 1`
echo "memcpy $QEMU_EOS_ML_MEMCPY - $QEMU_EOS_ML_MEMCCPY"
cd $OLDPWD
