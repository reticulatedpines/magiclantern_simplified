#!/bin/bash

QEMU_PATH=qemu-1.6.0
ML_PATH=../magic-lantern/platform/$1.$2

make -C $QEMU_PATH || exit
make -C $ML_PATH || exit
make qemu-helper.bin -C $ML_PATH || exit
cp $ML_PATH/autoexec.bin .
cp $ML_PATH/qemu-helper.bin .

rm vram.txt
rm vram.png

$QEMU_PATH/arm-softmmu/qemu-system-arm -M ML-$1

convert vram.txt vram.png && eog vram.png
