#!/usr/bin/env bash

set -e

QEMU_NAME=${QEMU_NAME:=qemu-2.5.0}
ML=${ML:=magic-lantern}
GREP=${GREP:=grep}

echo
echo "This will setup QEMU for emulating Magic Lantern."
echo "Thou shalt not be afraid of compiling stuff on Linux ;)"
echo -n "Continue? [y/n] "
read answer
if test "$answer" != "Y" -a "$answer" != "y"; then exit 0; fi
echo

function install_gdb {
    [ $(uname) == "Darwin" ] && OS=mac || OS=linux

    echo "*** Will download the recommended 5.4-$OS from gcc-arm-embedded."
    echo

    cd ~ && \
        [ ! -f gcc-arm-none-eabi-5_4-2016q3/bin/arm-none-eabi-gdb ] &&
        wget -c https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q3-update/+download/gcc-arm-none-eabi-5_4-2016q3-20160926-$OS.tar.bz2 && \
        tar -jxf gcc-arm-none-eabi-5_4-2016q3-20160926-$OS.tar.bz2 && \
        rm gcc-arm-none-eabi-5_4-2016q3-20160926-$OS.tar.bz2
    echo "*** Please add gcc binaries to your executable PATH:"
    echo '    PATH=~/gcc-arm-none-eabi-5_4-2016q3/bin:$PATH'
    echo
}

function valid_gdb {
    if ! arm-none-eabi-gdb -v &> /dev/null; then
        # not installed, or not able to run for any reason
        return 1
    fi

    if arm-none-eabi-gdb -v | grep -q "host=x86_64"; then
        # 64-bit version - doesn't work
        return 1
    fi

    # assume it's OK
    # todo: check version number
    return 0
}

if [ $(uname) == "Darwin" ]; then
    echo "*** Installing dependencies for Mac..."
    echo
    # fixme: don't these require sudo?
    # can we check whether they are already installed, as on Ubuntu?
    if ! xcode-select -p &> /dev/null; then
        xcode-select --install
    fi
    # brew is "The missing package manager for macOS"
    # https://brew.sh
    if ! brew -v &> /dev/null; then
        ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    fi
    
    packages="python wget mercurial xz grep pkg-config glib automake libtool pixman mtools"
    for pkg in $packages; do
        brew list $pkg &> /dev/null || brew install $pkg
    done
    
    GREP=ggrep
fi

if apt-get -v &> /dev/null; then
    # apt-based system?
    # install these packages, if not already
    # only request sudo if any of them is missing
    # instead of GTK (libgtk2.0-dev), you may prefer SDL (libsdl1.2-dev)
    # 64-bit arm-none-eabi-gdb does not work - GDB bug?
    # gcc-arm-none-eabi:i386 does not include libnewlib - Ubuntu bug?
    packages="
        build-essential mercurial pkg-config libtool
        git libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev
        libgtk2.0-dev xz-utils mtools netcat-openbsd
        python python-pip python-docutils
        gdb-arm-none-eabi:i386
        gcc-arm-none-eabi libnewlib-arm-none-eabi"
    
    echo "*** Checking dependencies for Ubuntu..."
    echo
    # https://wiki.debian.org/ListInstalledPackages
    # dpkg -l also returns packages that are not installed
    deps_installed=yes
    for package in $packages; do
        if ! dpkg -l $package 2>/dev/null | grep -q '^.i'; then
            echo Not installed: $package
            deps_installed=no
        fi
    done

    if [ "$deps_installed" == "no" ]; then
        echo
        echo "*** Installing dependencies for Ubuntu..."
        echo
        sudo apt-get update
        sudo apt-get install $packages
        echo
    fi

    deps_installed=yes
    for package in $packages; do
        if ! dpkg -l $package 2>/dev/null | grep -q '^.i'; then
            echo Not installed: $package
            deps_installed=no
        fi
    done

    if [ "$deps_installed" == "no" ]; then
        echo
        echo "*** Error: Ubuntu dependencies could not be installed."
        echo
        exit 1
    fi
fi

# all systems (including Mac, or Ubuntu if the installation from PPA failed)
# this one works on old systems as well, but it won't work under WSL
if ! valid_gdb; then
    echo
    echo "*** WARNING: arm-none-eabi-gdb is not installed."
    echo "*** Downloading gcc-arm-embedded toolchain and installing it without the package manager."
    echo "*** Will be installed in your home directory (Makefile.user.default expects it there)."
    echo
    install_gdb
fi

# make sure we have a valid arm-none-eabi-gdb (regardless of operating system)
if ! valid_gdb; then
    echo "*** Please set up arm-none-eabi-gdb before continuing."
    exit 1
fi

echo -n "*** Using GDB: "
command -v arm-none-eabi-gdb
arm-none-eabi-gdb -v | head -n1

# install docutils (for compiling ML modules) and vncdotool (for test suite)
# only request sudo if any of them is missing
for package in docutils vncdotool; do
    pip2 list | grep $package || pip2 install $package
done

function die { echo "${1:-"Unknown Error"}" 1>&2 ; exit 1; }

pwd | grep $ML/contrib/qemu > /dev/null || die "error: we should be in $ML/contrib/qemu"

# go to the parent of magic-lantern folder
cd ../../..
ls | $GREP $ML > /dev/null || die "error: expecting to find $ML here"

mkdir -p qemu
cd qemu

echo
echo "*** Setting up QEMU in `pwd`..."
echo

# get qemu
wget -q --show-progress --progress=dot:giga -c http://wiki.qemu-project.org/download/$QEMU_NAME.tar.bz2
echo
tar jxf $QEMU_NAME.tar.bz2
echo

# initialize a git repo, to make it easy to track changes to QEMU source
cd $QEMU_NAME
if [ ! -d .git ]; then
  git init
  # git requires a valid email; if not setup, add one for this directory only
  git config user.email || git config user.email qemu@magiclantern.fm
  git add .
  git commit -q -m "$QEMU_NAME vanilla"
fi
cd ..

echo "Copying files..."

# copy our helper scripts
cp -r ../$ML/contrib/qemu/scripts/* .
chmod +x *.sh

# copy our testing scripts
mkdir -p tests
cp -r ../$ML/contrib/qemu/tests/* tests/
chmod +x tests/*.sh

# apply our patch
cd ${QEMU_NAME}
mkdir -p hw/eos
cp -r ../../$ML/contrib/qemu/eos/* hw/eos/
cp -r ../../$ML/src/backtrace.[ch] hw/eos/dbi/
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
echo "   ../configure_eos.sh"
echo "   make -j`$GREP -c processor /proc/cpuinfo 2> /dev/null || sysctl -n hw.ncpu 2> /dev/null || echo 1`"
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
echo "3) Mount the included SD (or CF) image (you may use mount.sh)"
echo "   and install ML on it, as usual. The card image must be bootable as well."
echo
echo "   The included card image is bootable and contains a small autoexec.bin"
echo "   that runs on all DIGIC 4/5 cameras and prints some basic info."
echo
echo "   To create your own SD/CF image, you need to copy the raw contents"
echo "   of the entire card, not just one partition. For example:"
echo "   dd if=/dev/mmcblk0 of=sd.img"
echo
echo "4) Start emulation with:"
echo
echo "   cd `pwd`/"
echo "   ./run_canon_fw.sh 60D"
echo
echo "   This will recompile QEMU, but not ML."
echo
echo "   Note: Canon GUI emulation (menu navigation, no LiveView) only works on"
echo -n "   "; $GREP --color=never -oP "(?<=GUI_CAMS=\( ).*(?=\))" tests/run_tests.sh;
echo
echo "5) Tips & tricks:"
echo "   - to enable or disable the boot flag in ROM, use something like:"
echo "     ./run_canon_fw.sh 60D,firmware=\"boot=1\""
echo "   - to use multiple firmware versions, place the ROMs under e.g. 5D3/113/ROM*.BIN and run:"
echo "     ./run_canon_fw.sh 5D3,firmware=\"113;boot=1\""
echo "   - to show MMIO activity (registers) and interrupts, use:"
echo "     ./run_canon_fw.sh 60D -d io,int"
echo "   - to show the executed ASM code, step by step, use:"
echo "     ./run_canon_fw.sh 60D -d exec,int -singlestep"
echo "   - to trace debug messages and various functions in the firmware, use:"
echo "     ./run_canon_fw.sh 60D -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb"
echo "   - if the above is too slow, compile the dm-spy-experiments branch "
echo "     with CONFIG_QEMU=y and CONFIG_DEBUG_INTERCEPT_STARTUP=y and try:"
echo "     ./run_canon_fw.sh 60D,firmware=\"boot=1\" -d io,int"
echo "   - some camera models require GDB patches to bypass tricky code sequences:"
echo "     ./run_canon_fw.sh 700D -s -S & arm-none-eabi-gdb -x 700D/patches.gdb"
echo "   - to trace all function calls and export them to IDA:"
echo "     ./run_canon_fw.sh 60D -d calls -singlestep"
echo "   - you may enable additional debug code (such as printing to QEMU console)"
echo "     by compiling ML with CONFIG_QEMU=y in your Makefile.user (also run make clean)."
echo "   - caveat: you cannot run autoexec.bin compiled with CONFIG_QEMU on the camera." 

echo
echo "Enjoy!"
