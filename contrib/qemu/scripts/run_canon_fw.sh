#!/bin/bash

QEMU_PATH=${QEMU_PATH:=qemu-2.5.0}
MAKE=${MAKE:=make}

$MAKE -C $QEMU_PATH || exit
$QEMU_PATH/arm-softmmu/qemu-system-arm \
    -drive if=sd,format=raw,file=sd.img \
    -drive if=ide,format=raw,file=cf.img \
    -M $*
