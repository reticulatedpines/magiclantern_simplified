#!/usr/bin/env bash

# Copy ML to QEMU SD/CF card images via mtools (no need to mount the images).
# To be executed from the 'qemu' directory (near magic-lantern).
# TODO: copy only on one card (SD or CF), depending on model.

if [ "$1" == "" ] || [ ! -d "$1" ]; then
  echo "usage: ./mtools_copy_ml.sh /path/to/ml/unzipped"
  exit
fi

# Check whether sd.img or cf.img is mounted
MAKE=false ./run_canon_fw.sh
if [ $? -eq 1 ]; then
    # run_canon_fw.sh will print an error
    exit;
fi

echo "Copying ML from $1 ..."
echo -n "... to $(pwd)/sd.img and cf.img"

. ./mtools_setup.sh
mcopy -o -i $MSD $1/* ::; \
mcopy -o -i $MCF $1/* ::; \
mcopy -o -s -i $MSD $1/ML/ ::; \
mcopy -o -s -i $MCF $1/ML/ ::; \

echo "."
