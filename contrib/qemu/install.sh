#!/usr/bin/env bash

set -e

QEMU_DIR=${QEMU_DIR:=qemu-eos}

# paths relative to QEMU_DIR (where it will be installed)
QEMU_NAME=${QEMU_NAME:=qemu-2.5.0}
ML_PATH=${ML_PATH:=../magic-lantern}

ML_NAME=${ML_PATH##*/}
GREP=${GREP:=grep}
WGET_OPTS="-c -q --show-progress --progress=dot:giga"
ARM_GDB=

echo
echo "This will setup QEMU for emulating Magic Lantern."
echo "Thou shalt not be afraid of compiling stuff on Linux ;)"
echo -n "Continue? [y/n] "
read answer
if test "$answer" != "Y" -a "$answer" != "y"; then exit 0; fi
echo

function install_gcc {

    UNTAR="tar -jxf"

    if [ $(uname) == "Darwin" ]; then
        # Mac: 64-bit (we'll have to compile a 32-bit GDB)
        TOOLCHAIN=gcc-arm-none-eabi-7-2017-q4-major
        DOWNLOAD=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/7-2017q4/
        MIRROR="$DOWNLOAD"
        TARBALL=$TOOLCHAIN-mac.tar.bz2
    elif [  -n "$(uname -a | grep Microsoft)" ]; then
        # WSL: 64-bit (we'll have to compile a 32-bit GDB)
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

    echo "*** Will download $TOOLCHAIN from:"
    echo "    https://developer.arm.com/open-source/gnu-toolchain/gnu-rm"
    echo

    if [ ! -f ~/$TOOLCHAIN/bin/arm-none-eabi-gdb ]; then
        cd ~
        wget $WGET_OPTS $DOWNLOAD$TARBALL || wget $WGET_OPTS $MIRROR$TARBALL \
            && $UNTAR $TARBALL && rm $TARBALL
        cd -
    else
        echo "*** Toolchain already installed in:"
        echo "    ~/$TOOLCHAIN"
    fi

    echo
    echo "*** Please add GCC binaries to your executable PATH, then run this script again."
    echo "*** Run this command, or paste it into your .profile and reopen the terminal:"
    echo "    export PATH=~/$TOOLCHAIN/bin:\$PATH"
    echo
    exit
}

function gdb_error {
    echo
    echo "*** Last 30 lines from $1:"
    echo
    tail -n 30 $1
    echo
    echo "*** GDB compilation failed; logs saved in $(pwd)"
    echo "*** Please check what went wrong, try to fix it and report back."
    exit 1
}

function install_gdb {
    # we may need to compile a recent GDB
    # the latest pre-built version is buggy (at the time of writing)
    # on Linux, the old 32-bit version works fine, but we can't run it on Mac/WSL
    GDB_DIR=$HOME/gdb-arm-none-eabi-8_1

    if [ ! -f $GDB_DIR/bin/arm-none-eabi-gdb ]; then
        pushd . > /dev/null
        local MIRROR=https://ftp.gnu.org/gnu
        mkdir $GDB_DIR
        cd $GDB_DIR
        echo "*** Setting up GDB in $(pwd)/  ..."
        mkdir src
        cd src
        wget $WGET_OPTS $MIRROR/gdb/gdb-8.1.tar.xz
        echo
        tar xJf gdb-8.1.tar.xz || exit 1
        echo
        mkdir build-gdb
        cd build-gdb
        echo "Configuring arm-none-eabi-gdb... (configure.log)"
        ../gdb-8.1/configure --target=arm-none-eabi --prefix=$GDB_DIR/ &> configure.log || gdb_error configure.log
        echo "Building arm-none-eabi-gdb... (make.log)"
        make &> make.log || gdb_error make.log
        echo "Installing arm-none-eabi-gdb... (install.log)"
        (make install || make install MAKEINFO=true) &> install.log || gdb_error install.log
        popd > /dev/null
    else
        echo "*** GDB already installed in:"
        echo "    $GDB_DIR/bin/"
    fi

    echo
    echo "*** Please add GDB binaries to your executable PATH, then run this script again."
    echo "*** Run this command, or paste it into your .profile and reopen the terminal:"
    echo "    export PATH=$GDB_DIR/bin/:\$PATH"
    echo
    exit
}

function valid_arm_gdb {
    if ! arm-none-eabi-gdb -v &> /dev/null && ! gdb-multiarch -v &> /dev/null; then
        # not installed, or not able to run for any reason
        return 1
    fi

    # 8.0 and earlier is buggy on 64-bit; 8.1 is known to work
    # FIXME: the check will reject 9.x and later

    if arm-none-eabi-gdb -v 2>/dev/null | grep -q " [8]\.[1-9]"; then
        # this one is good, even if compiled for 64-bit
        ARM_GDB="arm-none-eabi-gdb"
        return 0
    fi

    if gdb-multiarch -v 2>/dev/null | grep -q " [8]\.[1-9]"; then
        # this one is just as good
        ARM_GDB="gdb-multiarch"
        return 0
    fi

    if arm-none-eabi-gdb -v 2>/dev/null | grep -q "host=i[3-6]86"; then
        # let's hope it's OK
        ARM_GDB="arm-none-eabi-gdb"
        return 0
    fi

    if gdb-multiarch -v 2>/dev/null | grep -q "host=i[3-6]86"; then
        # let's hope it's OK
        ARM_GDB="gdb-multiarch"
        return 0
    fi

    echo "*** WARNING: old 64-bit GDB is known to have issues."
    return 1
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
        echo "*** Old 64-bit GDB versions are known to have issues."
        echo
        echo "*** You have a few options:"
        echo

        # 32-bit binaries not working under WSL - hide these options
        # fixme: hidden options can still be selected
        if [  -z "$(uname -a | grep Microsoft)" ]; then
            if apt-cache show gdb-arm-none-eabi &>/dev/null; then
                echo "1 - Install gdb-arm-none-eabi:i386 and gcc-arm-none-eabi from Ubuntu repo (recommended)"
                echo "    This will install 32-bit binaries."
            elif apt-cache show gdb-multiarch &>/dev/null; then
                echo "1 - Install gdb-multiarch and gcc-arm-none-eabi from Ubuntu repo (recommended)"
                echo "    GDB 8.1 or later is known to work well."
            else
                echo "1 - Sorry, gdb-arm-none-eabi is not available on your distro."
                echo "    Please pick something else."
            fi
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
            echo "4 - Install gcc-arm-none-eabi from Ubuntu repository"
            echo "    and compile arm-none-eabi-gdb 8.1 from source."
            echo
            echo "5 - Manually install arm-none-eabi-gdb from https://launchpad.net/gcc-arm-embedded"
            echo "    or any other source, make sure it is in PATH, then run this script again."
            echo
        else
            # WSL
            echo "1-3: options not available on Windows 10 WSL (32-bit Linux binaries not supported)."
            echo
            echo "4 - Install gcc-arm-none-eabi from Ubuntu repository (64-bit)"
            echo "    and compile arm-none-eabi-gdb 8.1 from source."
            echo "    Sorry, we don't have a better option yet -> this is the recommended choice."
            echo
            echo "5 - Manually install arm-none-eabi-gdb from https://launchpad.net/gcc-arm-embedded"
            echo "    or any other source (choose 64-bit Linux binaries),"
            echo "    make sure it is in PATH, then run this script again."
            echo "    WARNING: this may not be able to run all our GDB scripts."
        fi

        echo
        echo -n "Your choice? "
        read answer
        echo
        case $answer in
            1)
                if apt-cache show gdb-arm-none-eabi &>/dev/null; then
                    # Ubuntu's 32-bit arm-none-eabi-gdb works fine (if available)
                    packages="$packages gdb-arm-none-eabi:i386 "
                elif apt-cache show gdb-multiarch &>/dev/null; then
                    # Ubuntu Bionic's 64-bit gdb-multiarch works fine (8.1.x); there's no standalone gdb-arm-none-eabi
                    # Ubuntu Artful's 64-bit gdb-multiarch does not work (8.0.x); using gdb-arm-none-eabi:i386 instead
                    packages="$packages gdb-multiarch "
                else
                    # invalid choice
                    exit 1
                fi
                # gcc-arm-none-eabi:i386 does not include libnewlib - Ubuntu bug?
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
                # install native (64 or 32) arm-none-eabi-gcc from package manager
                # and compile arm-none-eabi-gdb 8.1 from source
                packages="$packages gcc-arm-none-eabi libnewlib-arm-none-eabi"
                ;;
            5)
                # user will install arm-none-eabi-gdb and run the script again
                exit 0
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
        sudo apt-get update || true
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
if ! valid_arm_gcc; then
    echo
    echo "*** WARNING: a valid arm-none-eabi-gcc could not be found."
    echo "*** Downloading a toolchain and installing it without the package manager."
    echo "*** Will be installed in your home directory (Makefile.user.default expects it there)."
    echo
    echo -n "Continue? [y/n] "
    read answer
    if test "$answer" != "Y" -a "$answer" != "y"; then exit 0; fi
    echo
    install_gcc
fi

if ! valid_arm_gdb; then
    echo
    echo "*** WARNING: a valid arm-none-eabi-gdb could not be found."
    echo "*** Will compile gdb 8.1 from source and install it under your home directory."
    echo
    echo -n "Continue? [y/n] "
    read answer
    if test "$answer" != "Y" -a "$answer" != "y"; then exit 0; fi
    echo
    install_gdb
fi

# make sure we have a valid arm-none-eabi-gdb (regardless of operating system)
if ! valid_arm_gdb; then
    echo "*** Please set up a valid arm-none-eabi-gdb or gdb-multiarch before continuing."
    exit 1
fi

# same for arm-none-eabi-gcc
if ! valid_arm_gcc; then
    echo "*** Please set up a valid arm-none-eabi-gcc before continuing."
    exit 1
fi

echo
echo -n "*** Using GCC: "
command -v arm-none-eabi-gcc
arm-none-eabi-gcc -v 2>&1 | grep "gcc version"
echo
echo -n "*** Using GDB: "
command -v $ARM_GDB         # set by valid_arm_gcc
$ARM_GDB -v | head -n1
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
echo "*** Setting up QEMU in $(pwd)/ ..."
echo

if [ -d $QEMU_NAME ]; then
  DATE=`date '+%Y-%m-%d_%H-%M-%S'`
  echo "*** Directory $(pwd)/$QEMU_NAME already exists."
  echo "*** To reinstall, please rename or delete it first."
  echo ""
  echo "  - R or r        : rename to $(pwd)/${QEMU_NAME}_$DATE/"
  echo "  - C or c        : make clean & rename to $(pwd)/${QEMU_NAME}_$DATE/"
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
      C|c)
        make -C $QEMU_NAME clean
        mv $QEMU_NAME ${QEMU_NAME}_$DATE
        ;;
      *)
        exit 1
        ;;
  esac
fi

# get qemu
wget $WGET_OPTS http://wiki.qemu-project.org/download/$QEMU_NAME.tar.bz2
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
if gcc -v 2>&1 | grep -q "gcc version [789]"; then
  # hopefully these will also work for gcc 9.x (not tested)
  patch -N -p1 < ../$ML_PATH/contrib/qemu/$QEMU_NAME-gcc78.patch
  git add -u . && git commit -q -m "$QEMU_NAME patched for gcc 7.x and 8.x"
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
        exit 1
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
echo "     ./run_canon_fw.sh 60D -d debugmsg -s -S & $ARM_GDB -x 60D/debugmsg.gdb"
echo "   - some camera models require GDB patches to bypass tricky code sequences:"
echo "     ./run_canon_fw.sh EOSM -s -S & $ARM_GDB -x EOSM/patches.gdb"
echo
echo
echo "Online documentation: "
echo
echo "   https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/README.rst"
echo "   https://bitbucket.org/hudson/magic-lantern/src/qemu/contrib/qemu/HACKING.rst"
echo
echo "Enjoy!"
echo

if [ $(uname) != "Darwin" ] && ! [[ $DISPLAY ]]; then
    echo "P.S. To run the GUI, please make sure you have a valid DISPLAY."
    if [  -n "$(uname -a | grep Microsoft)" ]; then
        echo "Under Windows, you may install either VcXsrv or XMing."
        echo "Start it, then type the following into the terminal:"
        echo "    export DISPLAY=:0"
    fi
fi
