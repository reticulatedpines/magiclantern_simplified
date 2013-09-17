#!/bin/bash

# please set them according to your setup in your environment variables.
# if not set, the ones are the default values
ARM_ABI=${ARM_ABI:-none-eabi}
ARM_PATH=${ARM_PATH:-~/gcc-arm-none-eabi-4_7-2012q4}
GCC_VERSION=${GCC_VERSION:-4.7.3}
ARM_BINPATH=${ARM_BINPATH:-$ARM_PATH/bin}
OBJCOPY=${OBJCOPY:-$ARM_BINPATH/arm-$ARM_ABI-objcopy}

TMP_FILE="tmp.bin"

if [[ -z "$1" ]]; then
    echo >&2 "You must supply a module file"
    exit 1
elif [[ ! -f "$1" ]]; then
    echo >&2 "$1 is not a valid module file"
    exit 1
fi

echo "#-------- reading general hg information --------"
$OBJCOPY -O binary -j .module_hginfo $@ $TMP_FILE || exit 0
gunzip < $TMP_FILE
rm $TMP_FILE

echo "#-------- reading hg diff --------"
$OBJCOPY -O binary -j .module_hgdiff $@ $TMP_FILE || exit 0
gunzip < $TMP_FILE
rm $TMP_FILE

echo "#-------- DONE --------"
