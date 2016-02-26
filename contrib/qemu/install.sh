#!/bin/bash

QEMU_NAME=${QEMU_NAME:=qemu-2.3.0}
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
git init
git add .
git commit -m "$QEMU_NAME vanilla" 
cd ..

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
echo "   ./configure --target-list=arm-softmmu --disable-docs --enable-sdl"
echo "   make -j`grep -c processor /proc/cpuinfo`"
echo
echo "2) Grab a copy of the Canon firmware from your own camera"
echo "   (don't request one and don't share it online - it's copyrighted)"
echo
echo "   Look on your SD card - you should find ML/LOGS/ROM0.BIN and ROM1.BIN"
echo "   Copy those in `pwd`/ and then run (for 60D):"
echo
echo "   cat ROM0.BIN ROM1.BIN > ROM-60D.BIN"
echo
echo "3) Create a sdcard image named sd.img, from the entire card,"
echo "   NOT just one partition. It's easiest if you have a tiny card."
echo "   On my system, I used this command on a 256MB card:"
echo "   dd if=/dev/mmcblk0 of=sd.img"
echo
echo "4) Enable CONFIG_QEMU=y in your Makefile.user"
echo "   from magic-lantern directory, then run 'make clean' to make sure"
echo "   you will rebuild ML from scratch."
echo
echo "   Caveat: you can't run autoexec.bin compiled with CONFIG_QEMU on the camera,"
echo "   and neither a vanilla autoexec in QEMU (yet), so be careful not to mix them."
echo
echo "5) Mount the sd image (you may use mount.sh) and install ML on it, as usual."
echo "   The card image must be bootable as well, so, if you didn't have ML installed"
echo "   on the card from which you made the image, you may use make_bootable.sh."
echo
echo "6) Start emulation with:"
echo
echo "   cd `pwd`/"
echo "   ./run_canon_fw.sh 60D"
echo
echo "   This will recompile QEMU, but not ML."
echo "   Note: Canon GUI emulation (well, a small part of it) only works on 60D."
echo
echo "7) Tips & tricks:"
echo "   - to run plain Canon firmware, either make the card image non-bootable,"
echo "     or patch the ROM at 0xF8000004 from eos.c to disable the bootflag."
echo "   - to turn off most log messages, return early from io_log (in eos.c)."
echo
echo "Enjoy!"
