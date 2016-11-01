#!/bin/bash

# Emulator tests
# This also shows the emulation state on various cameras

EOS_CAMS=( 5D 5D2 5D3 5D4 6D 7D 7D2M
           40D 50D 60D 70D 80D
           400D 450D 500D 550D 600D 650D 700D 750D 760D
           100D 1000D 1100D 1200D EOSM )

POWERSHOT_CAMS=( EOSM3 A1100 )

GUI_CAMS=( 60D 5D3 550D 600D 1200D 1100D )

EOS_SECONDARY_CORES=( 5D4AE 7D2S )

if false ; then
    # to test only specific models
    EOS_CAMS=(5D)
    POWERSHOT_CAMS=()
    GUI_CAMS=(550D)
fi


# We will use mtools to alter and check the SD/CF image contents.
# fixme: hardcoded partition offset
MSD=sd.img@@50688
MCF=cf.img@@50688

# mtools doesn't like our SD image, for some reason
export MTOOLS_SKIP_CHECK=1
export MTOOLS_NO_VFAT=1



# this script runs from qemu/tests/ so we have to go up one level
cd ..

echo "Compiling..."
./run_canon_fw.sh help > build.log

# don't recompile each time (for speed)
export MAKE="echo skipping make"

# All EOS cameras should emulate the bootloader
# and jump to main firmware:
echo
echo "Testing bootloaders..."
for CAM in ${EOS_CAMS[*]} ${EOS_SECONDARY_CORES[*]}; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/boot.log
    # sorry, couldn't get the monitor working together with log redirection...
    # going to wait for red DY (from READY), with 2 seconds timeout, then kill qemu
    (./run_canon_fw.sh $CAM,firmware="boot=0" -display none &> tests/$CAM/boot.log) &
    sleep 0.1
    ( timeout 5 tail -f -n0 tests/$CAM/boot.log & ) | grep --binary-files=text -qP "\x1B\x5B31mD\x1B\x5B0m\x1B\x5B31mY\x1B\x5B0m"
    killall -INT qemu-system-arm &>> tests/$CAM/boot.log
    
    tests/check_grep.sh tests/$CAM/boot.log -E "([KR].* (READY|AECU)|Intercom)"
done

# All cameras should run under GDB and start a few tasks
echo
echo "Testing GDB scripts..."
for CAM in ${EOS_CAMS[*]} ${EOS_SECONDARY_CORES[*]} ${POWERSHOT_CAMS[*]}; do
    printf "%5s: " $CAM

    if [ ! -f $CAM/debugmsg.gdb ]; then
        echo -e "\e[33m$CAM/debugmsg.gdb not present\e[0m"
        continue
    fi

    mkdir -p tests/$CAM/
    rm -f tests/$CAM/gdb.log
    (./run_canon_fw.sh $CAM,firmware="boot=0" -display none -s -S & \
     arm-none-eabi-gdb -x $CAM/debugmsg.gdb &) &> tests/$CAM/gdb.log
    sleep 0.1
    ( timeout 10 tail -f -n0 tests/$CAM/gdb.log & ) | grep --binary-files=text -qP "task_create\("
    sleep 1
    killall -INT qemu-system-arm &>> tests/$CAM/gdb.log
    sleep 0.2

    tac tests/$CAM/gdb.log > tmp
    tests/check_grep.sh tmp -Em1 "task_create\("
done

# The next tests require custom SD/CF card imags.
# The one that comes with our QEMU install script is perfect.
echo
echo "Setting up temporary SD/CF card images..."

function sd_restore {
  trap '' SIGINT
  echo
  echo "Restoring your SD/CF card images..."
  mv sd-user.img sd.img
  mv cf-user.img cf.img
  trap - SIGINT
}

# disable CTRL-C while moving the files
trap '' SIGINT
mv sd.img sd-user.img
mv cf.img cf-user.img
trap sd_restore EXIT
trap - SIGINT

cp -v ../magic-lantern/contrib/qemu/sd.img.xz .
unxz -k sd.img.xz
cp sd.img cf.img

echo
echo "Testing display from bootloader..."

# All EOS cameras should run the portable display test:
for CAM in ${EOS_CAMS[*]}; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/disp.ppm
    rm -f tests/$CAM/disp.log
    (sleep 5; echo screendump tests/$CAM/disp.ppm; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=1" -display none -monitor stdio &> tests/$CAM/disp.log
    
    tests/check_md5.sh tests/$CAM/ disp
done

# EOS M3 is different (PowerShot firmware); let's test it too
echo
echo "Testing EOS M3..."
for CAM in EOSM3; do
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/boot.log
    (./run_canon_fw.sh $CAM -display none -s -S & \
     arm-none-eabi-gdb -x EOSM3/debugmsg.gdb &) &> tests/$CAM/boot.log
    sleep 0.1
    ( timeout 10 tail -f -n0 tests/$CAM/boot.log & ) | grep --binary-files=text -qP "\x1B\x5B31ma\x1B\x5B0m\x1B\x5B31my\x1B\x5B0m"
    killall -INT qemu-system-arm &>> tests/$CAM/boot.log

    printf "SD boot: "; tests/check_grep.sh tests/$CAM/boot.log -om1 "StartDiskboot"
    printf "Display: "; tests/check_grep.sh tests/$CAM/boot.log -om1 "TurnOnDisplay"
done

echo
echo "Testing file I/O (DCIM directory)..."
# Most EOS cameras should be able to create the DCIM directory if missing.
# Currently works only on models that can boot Canon GUI, and also on 100D.
for CAM in ${GUI_CAMS[*]} 100D; do
    printf "%5s: " $CAM
    
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/dcim.log

    # remove the DCIM directory from the card images
    mdeltree -i $MSD ::/DCIM &> /dev/null
    mdeltree -i $MCF ::/DCIM &> /dev/null

    (sleep 15; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=0" -display none -monitor stdio &> tests/$CAM/dcim.log
    
    if (mdir -b -i $MSD | grep -q DCIM) || (mdir -b -i $MCF | grep -q DCIM); then
        echo "OK"
    else
        echo -e "\e[31mFAILED!\e[0m"
    fi
done

echo
echo "Preparing portable ROM dumper..."

# re-create the card images, just in case
rm sd.img
unxz -k sd.img.xz
cp sd.img cf.img

ROM_DUMPER_BIN=tests/test-progs/portable-rom-dumper/autoexec.bin
TMP=tests/tmp

mkdir -p $TMP

if [ ! -f $ROM_DUMPER_BIN ]; then
    mkdir -p `dirname $ROM_DUMPER_BIN`
    wget -o $ROM_DUMPER_BIN https://dl.dropboxusercontent.com/u/4124919/debug/portable-rom-dumper/autoexec.bin
fi

# we don't know whether the camera will use SD or CF, so prepare both
mcopy -o -i $MSD $ROM_DUMPER_BIN ::
mcopy -o -i $MCF $ROM_DUMPER_BIN ::

# save the listing of the root filesystem
mdir -i $MSD > $TMP/sd.lst
mdir -i $MCF > $TMP/cf.lst
if ! diff $TMP/sd.lst $TMP/cf.lst ; then
    echo "Error: SD and CF contents do not match."
    exit
fi

function check_rom_md5 {
    # check ROM dumps from SD/CF card image (the dumper saves MD5 checksums)

    # we don't know yet which image was used (CF or SD)
    if   mdir -i $MSD ::ROM* &> /dev/null; then
        DEV=$MSD
    elif mdir -i $MSD ::ROM* &> /dev/null; then
        DEV=$MCF
    else
        echo -e "\e[31mROMs not saved\e[0m"
        return
    fi

    # only one card should contain the ROMs
    if mdir -i $MCF ::ROM* &> /dev/null && mdir -i $MSD ::ROM* &> /dev/null; then
        echo -e "\e[31mROMs on both CF and SD\e[0m"
        return
    fi

    # copy the ROM files locally to check them
    rm -f $TMP/ROM*
    mcopy -i $DEV ::ROM* $TMP/

    # check the MD5 sums
    cd $TMP/
    if md5sum --strict --status -c *.MD5; then
        # OK: print MD5 output normally
        md5sum -c ROM*.MD5 | tr '\n' '\t'
        echo ""
    else
        # not OK: print the status of each file, in red
        echo -n -e "\e[31m"
        md5sum -c ROM*.MD5 2>/dev/null | tr '\n' '\t'
        echo -e "\e[0m"
    fi
    cd $OLDPWD

    # delete the ROM files from the SD/CF images
    if mdir -i $MSD ::ROM* &> /dev/null; then mdel -i $MSD ::ROM*; fi
    if mdir -i $MCF ::ROM* &> /dev/null; then mdel -i $MCF ::ROM*; fi

    # check whether other files were created/modified (shouldn't be any)
    mdir -i $MSD > $TMP/sd2.lst
    mdir -i $MCF > $TMP/cf2.lst
    diff -q $TMP/sd.lst $TMP/sd2.lst
    diff -q $TMP/cf.lst $TMP/cf2.lst
}

echo "Testing portable ROM dumper..."
# Most EOS cameras should run the portable ROM dumper.
for CAM in ${EOS_CAMS[*]}; do
    printf "%5s: " $CAM

    # The dumper requires the "Open file for write" string present in the firmware.
    if ! grep -q "Open file for write" $CAM/ROM[01].BIN ; then
        echo "skipping"
        continue
    fi
    
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/romdump.ppm
    rm -f tests/$CAM/romdump.log

    # make sure there are no ROM files on the card
    if mdir -i $MSD ::ROM* &> /dev/null; then
        echo "Error: SD image already contains ROM dumps."
        continue
    fi
    if mdir -i $MCF ::ROM* &> /dev/null; then
        echo "Error: CF image already contains ROM dumps."
        continue
    fi

    (sleep 20; echo screendump tests/$CAM/romdump.ppm; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=1" -display none -monitor stdio &> tests/$CAM/romdump.log
    
    check_rom_md5 $CAM
done

# custom SD image no longer needed
sd_restore
trap - EXIT

# These cameras should display some Canon GUI:
echo
echo "Testing Canon GUI..."
for CAM in ${GUI_CAMS[*]}; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/gui.ppm
    rm -f tests/$CAM/gui.log
    (sleep 20; echo screendump tests/$CAM/gui.ppm; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=0" -display none -monitor stdio &> tests/$CAM/gui.log

    tests/check_md5.sh tests/$CAM/ gui
done
