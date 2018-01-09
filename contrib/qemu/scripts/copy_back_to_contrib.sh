#!/usr/bin/env bash

# run this if you make changes to qemu and want to commit them back into ML tree

QEMU_PATH=${QEMU_PATH:=qemu-2.5.0}
ML_PATH=${ML_PATH:=../magic-lantern}

cp -v *.sh *.py *.gdb gdbopts $ML_PATH/contrib/qemu/scripts
cp -v --parents */*.gdb $ML_PATH/contrib/qemu/scripts
cp -v $QEMU_PATH/hw/eos/* $ML_PATH/contrib/qemu/eos
cp -v $QEMU_PATH/hw/eos/mpu_spells/* $ML_PATH/contrib/qemu/eos/mpu_spells
cp -v $QEMU_PATH/hw/eos/dbi/* $ML_PATH/contrib/qemu/eos/dbi
cp -v --parents tests/*.sh tests/*.py $ML_PATH/contrib/qemu/
cp -v --parents tests/*/*.md5 $ML_PATH/contrib/qemu/

cd $QEMU_PATH
git diff > ../$ML_PATH/contrib/qemu/$QEMU_PATH.patch
cd ..

cd $ML_PATH/contrib/qemu/
hg diff .
