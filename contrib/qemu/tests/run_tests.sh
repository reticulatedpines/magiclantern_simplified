#!/bin/bash

# Emulator tests
# This also shows the emulation state on various cameras

cd ..

echo "Compiling..."
./run_canon_fw.sh help > build.log

# don't recompile each time (for speed)
export MAKE="echo skipping make"

# These cameras should emulate the bootloader
# and jump to main firmware:
echo
echo "Testing bootloaders..."
for CAM in 5D 5D2 5D3 6D 7D 7D2M 7D2S \
           50D 60D 70D 80D \
           500D 550D 600D 650D 700D 750D 760D \
           100D 1000D 1100D 1200D EOSM; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/boot.log
    # sorry, couldn't get the monitor working together with log redirection...
    # going to wait for red DY (from READY), with 2 seconds timeout, then kill qemu
    (./run_canon_fw.sh $CAM,firmware="boot=0" -nographic -monitor none &> tests/$CAM/boot.log) &
    sleep 0.1
    ( timeout 2 tail -f -n0 tests/$CAM/boot.log & ) | grep --binary-files=text -qP "\x1B\x5B31mD\x1B\x5B0m\x1B\x5B31mY\x1B\x5B0m"
    killall -INT qemu-system-arm &>> tests/$CAM/boot.log
    
    tests/check_grep.sh tests/$CAM/boot.log -E "[KR].* READY"
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

# These cameras should run the portable display test:
for CAM in 5D 5D2 5D3 6D 7D 7D2M 7D2S \
           50D 60D 70D 80D \
           500D 550D 600D 650D 700D 750D 760D \
           100D 1000D 1100D 1200D EOSM; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/disp.ppm
    rm -f tests/$CAM/disp.log
    (sleep 3; echo screendump tests/$CAM/disp.ppm; echo quit) \
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
for CAM in 60D 5D3 600D 1200D 1100D; do
    printf "%5s: " $CAM
    mkdir -p tests/$CAM/
    rm -f tests/$CAM/gui.ppm
    rm -f tests/$CAM/gui.log
    (sleep 20; echo screendump tests/$CAM/gui.ppm; echo quit) \
      | ./run_canon_fw.sh $CAM,firmware="boot=0" -nographic &> tests/$CAM/gui.log

    tests/check_md5.sh tests/$CAM/ gui
done
