#!/usr/bin/env bash

set -e

QEMU_DIR=${QEMU_DIR:=qemu-eos}

# paths relative to QEMU_DIR (where it will be installed)
QEMU_NAME=${QEMU_NAME:=qemu-2.5.0}
ML_PATH=${ML_PATH:=../magic-lantern}

ML_NAME=${ML_PATH##*/}
GREP=${GREP:=grep}
ALLOW_64BIT_GDB=n

echo
echo "This will setup QEMU for emulating Magic Lantern."
echo "Thou shalt not be afraid of compiling stuff on Linux ;)"
echo -n "Continue? [y/n] "
read answer
if test "$answer" != "Y" -a "$answer" != "y"; then exit 0; fi
echo

function install_gdb {
    echo
    echo "*** Will download gcc-arm-none-eabi-5_4-2016q3 from:"
    echo "    https://developer.arm.com/open-source/gnu-toolchain/gnu-rm"
    echo

    UNTAR="tar -jxf"

    if [ $(uname) == "Darwin" ]; then
        # 64-bit (fixme: compile a 32-bit GDB)
        TOOLCHAIN=gcc-arm-none-eabi-7-2017-q4-major
        DOWNLOAD=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/7-2017q4/
        MIRROR="$DOWNLOAD"
        TARBALL=$TOOLCHAIN-mac.tar.bz2
    elif [  -n "$(uname -a | grep Microsoft)" ]; then
        # WSL
        TOOLCHAIN=gcc-arm-none-eabi-7-2017-q4-major
        DOWNLOAD=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/7-2017q4/
        MIRROR="$DOWNLOAD"
        TARBALL=$TOOLCHAIN-linux.tar.bz2
    else
        # Linux (other than WSL) - use a 32-bit build (preferred, even though it's older)
        TOOLCHAIN=gcc-arm-none-eabi-5_4-2016q3
        DOWNLOAD=https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q3-update/+download/
        MIRROR=https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/
        TARBALL=$TOOLCHAIN-20160926-linux.tar.bz2
    fi

    if [ ! -f ~/$TOOLCHAIN/bin/arm-none-eabi-gdb ]; then
        cd ~
        wget -c $DOWNLOAD$TARBALL || wget -c $MIRROR$TARBALL \
            && $UNTAR $TARBALL && rm $TARBALL
        cd -
    else
        echo "*** Toolchain already installed in:"
        echo "    ~/$TOOLCHAIN"
        echo
    fi

    echo "*** Please add gcc binaries to your executable PATH."
    echo "*** Run this command, or paste it into your .profile and reopen the terminal:"
    echo "    export PATH=~/$TOOLCHAIN/bin:\$PATH"
    echo
}

function valid_arm_gdb {
    if ! arm-none-eabi-gdb -v &> /dev/null; then
        # not installed, or not able to run for any reason
        return 1
    fi

    if [ "$ALLOW_64BIT_GDB" != "y" ]; then
        if arm-none-eabi-gdb -v | grep -q "host=x86_64"; then
            # 64-bit version - doesn't work well

            if [ $(uname) == "Darwin" ] || [  -n "$(uname -a | grep Microsoft)" ]; then
                # we don't have a 64-bit option on these systems
                # just warn about it (--strict is used for that), but consider it valid
                if [ "$1" != "--strict" ]; then
                    return 0
                fi
            fi

            # systems assumed to be able to run a 32-bit GDB
            # consider the 64-bit one invalid
            echo "*** WARNING: 64-bit GDB is known to have issues."
            return 1
        fi
    fi

    # assume it's OK
    # todo: check version number
    return 0
}

function valid_arm_gcc {
    if ! arm-none-eabi-gcc -v &> /dev/null; then
        # not installed, or not able to run for any reason
        return 1
    fi

    echo "#include <stdlib.h>" > arm-gcc-test.c
    if ! arm-none-eabi-gcc -c arm-gcc-test.c; then
        echo "*** WARNING: your arm-none-eabi-gcc is unable to compile a simple program."
        rm arm-gcc-test.c
        return 1
    fi

    rm arm-gcc-test.c
    return 0
}

if [ $(uname) == "Darwin" ]; then
    echo "*** Installing dependencies for Mac..."
    echo
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

if [  -n "$(lsb_release -i 2>/dev/null | grep Ubuntu)" ]; then
    # Ubuntu-based system? (including WSL)
    # install these packages, if not already
    # only request sudo if any of them is missing
    # instead of GTK (libgtk2.0-dev), you may prefer SDL (libsdl1.2-dev)
    packages="
        build-essential mercurial pkg-config libtool
        git libglib2.0-dev libpixman-1-dev zlib1g-dev
        libgtk2.0-dev xz-utils mtools netcat-openbsd
        python python-pip python-docutils"

    # if a valid arm-none-eabi-gcc/gdb is already in PATH, try to use that
    # otherwise, we'll try to install something
    if ! valid_arm_gdb || ! valid_arm_gcc; then
        echo "*** You do not seem to have an usable arm-none-eabi-gcc and/or gdb installed."
        echo "*** 64-bit GDB is known to have issues, so you may want to install a 32-bit version."
        echo
        echo "*** You have a few options:"
        echo

        # 32-bit binaries not working under WSL - hide these options
        # fixme: hidden options can still be selected
        if [  -z "$(uname -a | grep Microsoft)" ]; then
            echo "1 - Install gdb-arm-none-eabi:i386 and gcc-arm-none-eabi from Ubuntu repo (recommended)"
            echo "    This will install 32-bit binaries."
            echo
            echo "2 - Download a 32-bit gcc-arm-embedded and install it without the package manager."
            echo "    Will be installed in your home directory; to move it, you must edit the Makefiles."
            echo "    This will install 32-bit binaries."
            echo
            if dpkg -l binutils-arm-none-eabi 2>/dev/null | grep -q '^.i'; then
                echo "3 - Remove Ubuntu toolchain and install the one from gcc-arm-embedded PPA (gcc 6.x)"
                echo "    This will:"
                echo "    - sudo apt-get remove gcc-arm-none-eabi gdb-arm-none-eabi \\"
                echo "           binutils-arm-none-eabi libnewlib-arm-none-eabi"
            else
                echo "3 - Install the toolchain from gcc-arm-embedded PPA (gcc 6.x)"
                echo "    This will:"
            fi
            echo "    - sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa"
            echo "    - install the gcc-arm-embedded:i386 package."
            echo "    This will install 32-bit binaries."
            echo
            echo "4 - Install gdb-arm-none-eabi and gcc-arm-none-eabi from Ubuntu repository (64-bit)"
            echo "    WARNING: this may not be able to run all our GDB scripts."
            echo
            echo "5 - Manually install arm-none-eabi-gdb from https://launchpad.net/gcc-arm-embedded"
            echo "    or any other source, make sure it is in PATH, then run this script again."
            echo
        else
            # WSL
            echo "1-3: options not available on Windows 10 WSL (32-bit Linux binaries not supported)."
            echo
            echo "4 - Install gdb-arm-none-eabi and gcc-arm-none-eabi from Ubuntu repository (64-bit)"
            echo "    WARNING: this may not be able to run all our GDB scripts."
            echo "    Sorry, we don't have a better option yet -> this is the recommended choice."
            echo
            echo "5 - Manually install arm-none-eabi-gdb from https://launchpad.net/gcc-arm-embedded"
            echo "    or any other source (choose 64-bit Linux binaries),"
            echo "    make sure it is in PATH, then run this script again."
            echo "    WARNING: this may not be able to run all our GDB scripts."
            echo
            echo "    You may also try the Win32 GDB build from gcc-arm-embedded,"
            echo "    but getting it to work is not straightforward (contribution welcome)."
            echo
        fi

        if arm-none-eabi-gdb -v &> /dev/null; then
            echo "6 - Just use the current 64-bit toolchain."
            echo "    WARNING: this will not be able to run all our GDB scripts."
        fi

        echo
        echo -n "Your choice? "
        read answer
        echo
        case $answer in
            1)
                # Ubuntu's 32-bit arm-none-eabi-gdb works fine
                # gcc-arm-none-eabi:i386 does not include libnewlib - Ubuntu bug?
                packages="$packages gdb-arm-none-eabi:i386 "
                packages="$packages gcc-arm-none-eabi libnewlib-arm-none-eabi"
                ;;
            2)
                # 32-bit gdb will be downloaded after installing these packages
                packages="$packages libc6:i386 libncurses5:i386"
                ;;
            3)
                # gcc-arm-embedded conflicts with gcc-arm-none-eabi
                # but the dependencies are not configured properly
                # so we have to fix the conflict manually...
                if dpkg -l binutils-arm-none-eabi 2>/dev/null | grep -q '^.i'; then
                    echo
                    echo "*** Please double-check - the following might remove additional packages!"
                    echo
                    sudo apt-get remove gcc-arm-none-eabi gdb-arm-none-eabi \
                         binutils-arm-none-eabi libnewlib-arm-none-eabi
                fi
                packages="$packages gcc-arm-embedded:i386"
                echo
                echo "*** Adding the team-gcc-arm-embedded PPA repository..."
                echo "    sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa"
                echo
                sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
                ;;
            4)
                # Ubuntu's 64-bit arm-none-eabi-gdb is... well... better than nothing
                packages="$packages gdb-arm-none-eabi:amd64"
                packages="$packages gcc-arm-none-eabi:amd64 libnewlib-arm-none-eabi"
                ALLOW_64BIT_GDB=y
                ;;
            5)
                # user will install arm-none-eabi-gdb and run the script again
                exit 0
                ;;
            6)
                # use the installed version, even though it's known not to work well
                ALLOW_64BIT_GDB=y
                ;;
            *)
                # invalid choice
                exit 1
                ;;
        esac
    else
        echo "*** You have a valid ARM GCC/GDB already installed - using that one."
    fi

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
        if [[ "$packages" == *i386* ]]; then
            sudo dpkg --add-architecture i386
        fi
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

# all systems (including Mac, or Ubuntu if the installation from repositories failed)
# this one works on old systems as well, but it won't work under WSL
if ! valid_arm_gdb; then
    echo
    echo "*** WARNING: a valid arm-none-eabi-gdb could not be found."
    echo "*** Downloading a toolchain and installing it without the package manager."
    echo "*** Will be installed in your home directory (Makefile.user.default expects it there)."
    echo
    install_gdb
fi

# make sure we have a valid arm-none-eabi-gdb (regardless of operating system)
if ! valid_arm_gdb --strict; then
    if ! arm-none-eabi-gdb -v &> /dev/null; then
        echo "*** Please set up a valid arm-none-eabi-gdb before continuing."
        exit 1
    else
        # valid_arm_gdb will print why the current one is not good
        echo -n "Continue anyway? [y/N] "
        read answer
        if test "$answer" != "Y" -a "$answer" != "y"; then exit 1; fi
        echo
    fi
fi

# same for arm-none-eabi-gcc
if ! valid_arm_gcc; then
    echo "*** Please set up a valid arm-none-eabi-gcc before continuing."
    exit 1
fi

echo
echo -n "*** Using GDB: "
command -v arm-none-eabi-gdb
arm-none-eabi-gdb -v | head -n1
echo
echo -n "*** Using GCC: "
command -v arm-none-eabi-gcc
arm-none-eabi-gcc -v 2>&1 | grep "gcc version"
echo

# install docutils (for compiling ML modules) and vncdotool (for test suite)
# only install if any of them is missing
pip2 list | grep docutils  || rst2html -h  > /dev/null || pip2 install docutils
pip2 list | grep vncdotool || vncdotool -h > /dev/null || pip2 install vncdotool

function die { echo "${1:-"Unknown Error"}" 1>&2 ; exit 1; }

pwd | grep $ML_NAME/contrib/qemu > /dev/null || die "error: we should be in $ML_NAME/contrib/qemu"

# go to the parent of magic-lantern folder
cd ../../..
ls | $GREP $ML_NAME > /dev/null || die "error: expecting to find $ML_NAME here"

mkdir -p $QEMU_DIR
cd $QEMU_DIR

echo
echo "*** Setting up QEMU in $(pwd)..."
echo

if [ -d $QEMU_NAME ]; then
  DATE=`date '+%Y-%m-%d_%H-%M-%S'`
  echo "*** Directory $(pwd)/$QEMU_NAME already exists."
  echo "*** To reinstall, please rename or delete it first."
  echo ""
  echo "  - R or r        : rename to $(pwd)/${QEMU_NAME}_$DATE/"
  echo "  - uppercase D   : delete $(pwd)/$QEMU_NAME/ without confirmation (!)"
  echo "  - any other key : cancel the operation (exit the script)"
  echo ""
  echo -n "Your choice? "
  read answer
  case "$answer" in
      D)
        rm -Rf $QEMU_NAME
        ;;
      R|r)
        mv $QEMU_NAME ${QEMU_NAME}_$DATE
        ;;
      *)
        exit 1
        ;;
  esac
fi

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
  git config user.email || git config user.email qemu-eos@magiclantern.fm
  git config user.name || git config user.name qemu-eos
  git add . && git commit -q -m "$QEMU_NAME vanilla"
fi
cd ..

echo "Copying files..."

# copy our helper scripts
cp -r $ML_PATH/contrib/qemu/scripts/* .
chmod +x *.sh

# copy our testing scripts
mkdir -p tests
cp -r $ML_PATH/contrib/qemu/tests/* tests/
chmod +x tests/*.sh

# apply our patch
cd ${QEMU_NAME}
mkdir -p hw/eos
cp -r ../$ML_PATH/contrib/qemu/eos/* hw/eos/
cp -r ../$ML_PATH/src/backtrace.[ch] hw/eos/dbi/
if gcc -v 2>&1 | grep -q "gcc version 7"; then
  patch -N -p1 < ../$ML_PATH/contrib/qemu/$QEMU_NAME-gcc7.patch
  git add -u . && git commit -q -m "$QEMU_NAME patched for gcc 7.x"
fi

patch -N -p1 < ../$ML_PATH/contrib/qemu/$QEMU_NAME.patch
# don't commit this one - we'll use "git diff" to update the above patch

cd ..

# setup the card image
if [ ! -f "sd.img" ]; then
    echo "Setting up SD card image..."
    cp -v $ML_PATH/contrib/qemu/sd.img.xz .
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
echo -n "Shall this script attempt to compile QEMU now? [y/n] "
read answer
if test "$answer" == "Y" -o "$answer" == "y"
 then
    cd `pwd`/${QEMU_NAME}
    ../configure_eos.sh
    if make -j`$GREP -c processor /proc/cpuinfo 2> /dev/null || sysctl -n hw.ncpu 2> /dev/null || echo 1`; then
        cd ..
        echo
        echo "*** Done compiling."
        echo
        echo
        echo "Next steps:"
        echo "==========="
        echo
        echo "1) Go to QEMU directory"
        echo
        echo "   cd `pwd`/"
    else
        cd ..
        echo
        echo "*** Compilation failed."
        echo "*** Please check what went wrong, try to fix it and report back."
    fi
fi
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
echo "3) Install Magic Lantern on your SD/CF card image:"
echo
echo "   make -C ../magic-lantern 60D_install_qemu "
echo
echo "   The included card image is bootable and contains a small autoexec.bin"
echo "   that runs on all supported EOS cameras and prints some basic info."
echo
echo "4) Start emulation with:"
echo
echo "   cd `pwd`/"
echo "   ./run_canon_fw.sh 60D"
echo
echo "   This will recompile QEMU, but not ML."
echo
echo "   Note: Canon GUI emulation (menu navigation, no LiveView) only works on:"
echo -n "   "; echo $($GREP --color=never -oPz "(?<=GUI_CAMS=\( )[^()]*(?=\))" tests/run_tests.sh | tr '\0' '\n');
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
echo "     ./run_canon_fw.sh 60D -d debugmsg -s -S & arm-none-eabi-gdb -x 60D/debugmsg.gdb"
echo "   - some camera models require GDB patches to bypass tricky code sequences:"
echo "     ./run_canon_fw.sh EOSM -s -S & arm-none-eabi-gdb -x EOSM/patches.gdb"
echo
echo
echo "Online documentation: "
echo
echo "   https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/"
echo
echo "Enjoy!"
