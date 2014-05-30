#!/bin/bash

QEMU_PATH=${QEMU_PATH:=qemu-1.6.0}
ML=${ML:=magic-lantern}
ML_PATH=../${ML}/platform/$1.$2

make -C $QEMU_PATH || exit
make -C $ML_PATH || exit
make qemu-helper.bin -C $ML_PATH || exit
cp $ML_PATH/autoexec.bin .
cp $ML_PATH/qemu-helper.bin .
cp $ML_PATH/magiclantern .

rm -f vram*.txt
rm -f vram*.png

$QEMU_PATH/arm-softmmu/qemu-system-arm -M ML-$1

for f in `ls vram*.txt`;
do
    echo "Processing $f..."
    convert $f `basename $f .txt`.png
done

eog vram00.png

