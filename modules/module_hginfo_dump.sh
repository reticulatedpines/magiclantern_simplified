#!/bin/bash

TMP_FILE="tmp.bin"

if [[ -z "$@" ]]; then
    echo >&2 "You must supply a module file"
    exit 1
elif [[ ! -f "$@" ]]; then
    echo >&2 "$@ is not a valid module file"
    exit 1
fi

echo "#-------- reading general hg information --------"
$OBJCOPY -O binary -j .module_hginfo $@ $TMP_FILE
gunzip < $TMP_FILE

echo "#-------- reading hg diff --------"
$OBJCOPY -O binary -j .module_hgdiff $@ $TMP_FILE
gunzip < $TMP_FILE

rm $TMP_FILE
