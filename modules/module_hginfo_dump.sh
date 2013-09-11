#!/bin/bash

echo "#-------- reading general hg information --------"
arm-linux-gnueabihf-objcopy -O binary -j .module_hginfo raw_rec.mo out.bin
cat out.bin | gunzip

echo "#-------- reading hg diff --------"
arm-linux-gnueabihf-objcopy -O binary -j .module_hgdiff raw_rec.mo out.bin
cat out.bin | gunzip

rm out.bin