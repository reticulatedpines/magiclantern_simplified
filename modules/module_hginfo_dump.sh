#!/bin/bash

# please set them according to your setup in your environment variables.
# if not set, the ones are the default values
ARM_ABI=${ARM_ABI:-none-eabi}
ARM_PATH=${ARM_PATH:-~/gcc-arm-none-eabi-4_8-2013q4}
GCC_VERSION=${GCC_VERSION:-4.8.3}
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

#~ echo "#-------- reading general hg information --------"
#~ $OBJCOPY -O binary -j .module_hginfo --set-section-flags .module_hginfo=load $@ $TMP_FILE || exit 0
#~ gunzip < $TMP_FILE
#~ rm $TMP_FILE

$OBJCOPY -O binary -j .module_strings --set-section-flags .module_strings=load $@ $TMP_FILE || exit 0
python2 `dirname $0`/module_strings_dump.py $TMP_FILE
echo
#echo "#-------- reading hg diff --------"
$OBJCOPY -O binary -j .module_hgdiff --set-section-flags .module_hgdiff=load $@ $TMP_FILE || exit 0
gunzip < $TMP_FILE
rm $TMP_FILE
#echo "#-------- DONE --------"
