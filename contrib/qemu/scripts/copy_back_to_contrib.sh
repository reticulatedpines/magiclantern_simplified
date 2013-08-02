#!/bin/bash

# run this if you make changes to qemu and want to commit them back into ML tree

QEMU_PATH=qemu-1.4.0
ML=magic-lantern

cp -v *.sh gdbopts ../$ML/contrib/qemu/scripts
cp -v $QEMU_PATH/hw/eos.c $QEMU_PATH/hw/eos.h ../$ML/contrib/qemu/hw

cd ../$ML/contrib/qemu/
hg diff .
