#!/bin/bash

QEMU_NAME=qemu-1.6.0
ML=magic-lantern

echo
echo "This will setup QEMU for emulating Magic Lantern."
echo "Thou shalt not be afraid of compiling stuff on Linux ;)"
echo -n "Continue? [y/n] "
read answer
if test "$answer" != "Y" -a "$answer" != "y";
then exit 0;
fi

function die { echo "${1:-"Unknown Error"}" 1>&2 ; exit 1; }

pwd | grep $ML/contrib/qemu > /dev/null || die "error: we should be in $ML/contrib/qemu"

# go to the parent of magic-lantern folder
cd ../../..
ls | grep $ML > /dev/null || die "error: expecting to find $ML here"

mkdir -p qemu
cd qemu

echo
echo "*** Setting up QEMU in `pwd`..."
echo

# get qemu
wget -c http://wiki.qemu-project.org/download/$QEMU_NAME.tar.bz2
tar jxf $QEMU_NAME.tar.bz2

# apply our patch
cp -v ../$ML/contrib/qemu/scripts/* .
chmod +x *.sh
cd ${QEMU_NAME}
cp -v ../../$ML/contrib/qemu/hw/* hw/arm
patch -N -p1 < ../../$ML/contrib/qemu/$QEMU_NAME.patch
cd ..

echo ""
echo "Next steps:"
echo "==========="
echo
echo "1) Compile QEMU"
echo
echo "   cd `pwd`/${QEMU_NAME}"
echo "   ./configure --target-list=arm-softmmu --disable-docs"
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
echo "3) Enable CONFIG_QEMU=y in your Makefile.user"
echo "   from magic-lantern directory, then run 'make clean' to make sure"
echo "   you will rebuild ML from scratch."
echo
echo "4) Start emulation with:"
echo
echo "   cd `pwd`/"
echo "   ./run_ml_60D.sh"
echo
echo "   (this will recompile ML and QEMU - handy if you edit the sources often)"
echo
echo "Enjoy!"
