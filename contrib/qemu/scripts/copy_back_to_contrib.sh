#!/bin/bash

# run this if you make changes to qemu and want to commit them back into ML tree

cp -v *.sh gdbopts ../magic-lantern/contrib/qemu/scripts
cp -v qemu-1.4.0/hw/eos.c qemu-1.4.0/hw/eos.h ../magic-lantern/contrib/qemu/hw

cd ../magic-lantern/contrib/qemu/
hg diff .
