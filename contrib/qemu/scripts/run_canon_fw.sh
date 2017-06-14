#!/usr/bin/env bash

QEMU_PATH=${QEMU_PATH:=qemu-2.5.0}
MAKE=${MAKE:=make}

function is_mounted
{
    # better way to check whether a disk image is mounted?
    # or, how to tell QEMU to use exclusive access for the disk images?
    SD_DEV=`losetup -j $1 | grep -Po "(?<=/dev/)[^ ]*(?=:)"`
    if [ $? == 0 ]; then
        if cat /proc/mounts | grep /dev/mapper/$SD_DEV; then
            return 0
        fi
    fi
    return 1
}

if is_mounted sd.img; then
    echo
    echo "Error: please unmount the SD image."
    exit 1
fi

if is_mounted cf.img; then
    echo
    echo "Error: please unmount the CF image."
    exit 1
fi

# recompile QEMU
$MAKE -C $QEMU_PATH || exit

# clear the terminal
# (since the logs are very large, being able to scroll at the beginning is helpful)
tput reset

# run the emulation
$QEMU_PATH/arm-softmmu/qemu-system-arm \
    -drive if=sd,format=raw,file=sd.img \
    -drive if=ide,format=raw,file=cf.img \
    -chardev socket,server,nowait,path=qemu.monitor,id=monsock \
    -mon chardev=monsock,mode=readline \
    -M $*

# note: QEMU monitor is redirected to Unix socket qemu.monitor
# so you can interact with the emulator with e.g. netcat:
#
#    echo "log io" | nc -U qemu.monitor
#
# or, for interactive monitor console:
#
#    socat - UNIX-CONNECT:qemu.monitor
#
# you can, of course, redirect it with -monitor stdio or -monitor vl
# more info: http://nairobi-embedded.org/qemu_monitor_console.html
