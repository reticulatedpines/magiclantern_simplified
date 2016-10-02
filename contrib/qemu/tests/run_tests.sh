#!/bin/bash

# Emulator tests
# This also shows the emulation state on various cameras

EOS_CAMS=( 5D 5D2 5D3 5D4 6D 7D 7D2M 7D2S
           40D 50D 60D 70D 80D
           400D 450D 500D 550D 600D 650D 700D 750D 760D
           100D 1000D 1100D 1200D EOSM )

POWERSHOT_CAMS=( EOSM3 A1100 )

GUI_CAMS=( 60D 5D3 600D 1200D 1100D )

if false ; then
    # to test only specific models
    EOS_CAMS=(5D)
    POWERSHOT_CAMS=()
    GUI_CAMS=()
fi

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
for CAM in ${EOS_CAMS[*]}; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/boot.log
    # sorry, couldn't get the monitor working together with log redirection...
    # going to wait for red DY (from READY), with 2 seconds timeout, then kill qemu
    (./run_canon_fw.sh $CAM,firmware="boot=0" -nographic -monitor none &> tests/$CAM/boot.log) &
    sleep 0.1
    ( timeout 2 tail -f -n0 tests/$CAM/boot.log & ) | grep --binary-files=text -qP "\x1B\x5B31mD\x1B\x5B0m\x1B\x5B31mY\x1B\x5B0m"
    killall -INT qemu-system-arm &>> tests/$CAM/boot.log
    
    tests/check_grep.sh tests/$CAM/boot.log -E "([KR].* READY|Intercom)"
done

# All cameras should run under GDB and start a few tasks
echo
echo "Testing GDB scripts..."
for CAM in ${EOS_CAMS[*]} ${POWERSHOT_CAMS[*]}; do
    printf "%5s: " $CAM

    if [ ! -f $CAM/debugmsg.gdb ]; then
        echo -e "\e[33m$CAM/debugmsg.gdb not present\e[0m"
        continue
    fi

    mkdir -p tests/$CAM/
    rm -f tests/$CAM/gdb.log
    (./run_canon_fw.sh $CAM,firmware="boot=0" -nographic -monitor none -s -S & \
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
  echo
  echo "Restoring your SD/CF card images..."
  mv sd-user.img sd.img
  mv cf-user.img cf.img
}

# disable CTRL-C while moving the files
trap '' SIGINT
mv sd.img sd-user.img
mv cf.img cf-user.img
trap sd_restore EXIT
trap - SIGINT

cp -v ../magic-lantern/contrib/qemu/sd.img.xz .
unxz sd.img.xz
cp sd.img cf.img

echo
echo "Testing display from bootloader..."

# All EOS cameras should run the portable display test:
for CAM in ${EOS_CAMS[*]}; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/disp.ppm
    rm -f tests/$CAM/disp.log
    (sleep 4; echo screendump tests/$CAM/disp.ppm; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=1" -nographic &> tests/$CAM/disp.log
    
    tests/check_md5.sh tests/$CAM/ disp
done

# EOS M3 is different (PowerShot firmware); let's test it too
echo
echo "Testing EOS M3..."
for CAM in EOSM3; do
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/boot.log
    (./run_canon_fw.sh $CAM -nographic -monitor none -s -S & \
     arm-none-eabi-gdb -x EOSM3/debugmsg.gdb &) &> tests/$CAM/boot.log
    sleep 0.1
    ( timeout 10 tail -f -n0 tests/$CAM/boot.log & ) | grep --binary-files=text -qP "\x1B\x5B31ma\x1B\x5B0m\x1B\x5B31my\x1B\x5B0m"
    killall -INT qemu-system-arm &>> tests/$CAM/boot.log

    printf "SD boot: "; tests/check_grep.sh tests/$CAM/boot.log -om1 "StartDiskboot"
    printf "Display: "; tests/check_grep.sh tests/$CAM/boot.log -om1 "TurnOnDisplay"
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
      | ./run_canon_fw.sh $CAM,firmware="boot=0" -nographic &> tests/$CAM/gui.log

    tests/check_md5.sh tests/$CAM/ gui
done
