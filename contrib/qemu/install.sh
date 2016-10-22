#!/bin/bash

set -e

QEMU_NAME=${QEMU_NAME:=qemu-2.5.0}
ML=${ML:=magic-lantern}

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

# initialize a git repo, to make it easy to track changes to QEMU source
cd $QEMU_NAME
cd .git && cd .. || (git init && git add . && git commit -m "$QEMU_NAME vanilla")
cd ..

# copy our helper scripts
cp -vr ../$ML/contrib/qemu/scripts/* .
chmod +x *.sh

# copy our testing scripts
mkdir -p tests
cp -vr ../$ML/contrib/qemu/tests/* tests/
chmod +x tests/*.sh

# apply our patch
cd ${QEMU_NAME}
mkdir -p hw/eos
cp -vr ../../$ML/contrib/qemu/eos/* hw/eos
patch -N -p1 < ../../$ML/contrib/qemu/$QEMU_NAME.patch
cd ..

# setup the card image
if [ ! -f "sd.img" ]; then
    echo "Setting up SD card image..."
    cp -v ../$ML/contrib/qemu/sd.img.xz .
    unxz -v sd.img.xz
else
    echo "SD image already exists, skipping."
fi

if [ ! -f "cf.img" ]; then
    echo "Setting up CF card image..."
    cp -v sd.img cf.img
else
    echo "CF image already exists, skipping."
fi

echo ""
echo "Next steps:"
echo "==========="
echo
echo "1) Compile QEMU"
echo
echo "   cd `pwd`/${QEMU_NAME}"
echo "   ./configure --target-list=arm-softmmu --disable-docs --enable-sdl"
echo "   make -j`grep -c processor /proc/cpuinfo`"
echo
echo "2) Grab a copy of the Canon firmware from your own camera"
echo "   (don't request one and don't share it online - it's copyrighted)"
echo
echo "   Look on your SD card - you should find ML/LOGS/ROM0.BIN and ROM1.BIN"
echo "   Copy them under your camera model's subdirectory, for example:"
echo "   `pwd`/60D/"
echo
echo "   For models that use a serial flash, you may have to dump its contents"
echo "   using the sf_dump module, then copy SFDATA.BIN as well."
echo
echo "3) Enable CONFIG_QEMU=y in your Makefile.user"
echo "   from magic-lantern directory, then run 'make clean' to make sure"
echo "   you will rebuild ML from scratch."
echo
echo "   Caveat: you can't run autoexec.bin compiled with CONFIG_QEMU on the camera,"
echo "   and neither a vanilla autoexec in QEMU (yet), so be careful not to mix them."
echo
echo "4) Mount the included SD (or CF) image (you may use mount.sh)"
echo "   and install ML on it, as usual. The card image must be bootable as well."
echo
echo "   The included card image is bootable and contains a small autoexec.bin"
echo "   that runs on all DIGIC 4/5 cameras and prints some basic info."
echo
echo "   To create your own SD/CF image, you need to copy the raw contents"
echo "   of the entire card, not just one partition. For example:"
echo "   dd if=/dev/mmcblk0 of=sd.img"
echo
echo "5) Start emulation with:"
echo
echo "   cd `pwd`/"
echo "   ./run_canon_fw.sh 60D"
echo
echo "   This will recompile QEMU, but not ML."
echo "   Note: Canon GUI emulation (well, a small part of it) only works on"
echo "   60D, 1100D, 1200D, 5D3 1.1.3."
echo
echo "6) Tips & tricks:"
echo "   - to enable or disable the boot flag in ROM, use something like:"
echo "     ./run_canon_fw.sh 60D,firmware=\"boot=1\""
echo "   - to show MMIO activity (registers) and interrupts, use:"
echo "     ./run_canon_fw.sh 60D -d io,int"
echo "   - to show the executed ASM code, step by step, use:"
echo "     ./run_canon_fw.sh 60D -d exec,int -singlestep"
echo "   - to trace debug messages and various functions in the firmware, use:"
echo "     ./run_canon_fw.sh 60D -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb"
echo
echo "Enjoy!"
