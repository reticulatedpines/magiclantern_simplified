#!/bin/bash

QEMU_DIR=qemu-1.4.0

echo
echo "This will setup QEMU for emulating Magic Lantern."
echo "Thou shalt not be afraid of compiling stuff on Linux ;)"
echo -n "Continue? [y/n] "
read answer
if test "$answer" != "Y" -a "$answer" != "y";
then exit 0;
fi

function die { echo "${1:-"Unknown Error"}" 1>&2 ; exit 1; }

pwd | grep magic-lantern/contrib/qemu > /dev/null || die "error: we should be in magic-lantern/contrib/qemu"

# go to the parent of magic-lantern folder
cd ../../..
ls | grep magic-lantern > /dev/null || die "error: expecting to find magic-lantern here"

mkdir qemu
cd qemu

echo
echo "*** Setting up QEMU in `pwd`..."
echo

# get qemu
wget -c http://wiki.qemu-project.org/download/qemu-1.4.0.tar.bz2
tar jxf qemu-1.4.0.tar.bz2

# apply our patch
patch -N -p1 < ../magic-lantern/contrib/qemu/qemu-scripts.patch
chmod +x run_ml*.sh
cd ${QEMU_DIR}
patch -N -p1 < ../../magic-lantern/contrib/qemu/qemu-1.4.0.patch
cd ..

echo ""
echo "Next steps:"
echo "==========="
echo
echo "1) Compile QEMU"
echo
echo "   cd `pwd`/${QEMU_DIR}"
echo "   ./configure --target-list=arm-softmmu"
echo "   make"
echo
echo "2) Grab a copy of the Canon firmware from your own camera"
echo "   (don't request one and don't share it online - it's copyrighted)"
echo
echo "   Look on your SD card - you should find ML/LOGS/ROM0.BIN and ROM1.BIN"
echo "   Copy those in `pwd`/ and then run (for 60D):"
echo
echo "   cat ROM0.BIN ROM1.BIN > ROM-60D.BIN"
echo
echo "3) Enable CONFIG_QEMU=y in your Makefile.user from magic-lantern directory,"
echo "   then run 'make clean' to make sure you will rebuild ML from scratch."
echo
echo "4) Start emulation with:"
echo
echo "   cd `pwd`/"
echo "   ./run_ml_60D.sh"
echo
echo "   (this will recompile ML and QEMU - handy if you edit the sources often)"
echo
echo "Enjoy!"
