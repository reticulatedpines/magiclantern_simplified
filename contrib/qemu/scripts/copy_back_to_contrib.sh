#!/bin/bash

# run this if you make changes to qemu and want to commit them back into ML tree

QEMU_PATH=qemu-1.4.0

cp -v *.sh gdbopts ../magic-lantern/contrib/qemu/scripts
cp -v $QEMU_PATH/hw/eos.c $QEMU_PATH/hw/eos.h ../magic-lantern/contrib/qemu/hw

cd ../magic-lantern/contrib/qemu/
hg diff .
