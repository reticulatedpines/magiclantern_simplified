#!/usr/bin/env bash

# Backtrace tests
# This requires recompiling QEMU

# this script runs from qemu/tests/ so we have to go up one level
cd ..

QEMU_PATH=${QEMU_PATH:=qemu-2.5.0}
BKT_H=$QEMU_PATH/hw/eos/dbi/backtrace.h

function bkt_restore {
  trap '' SIGINT
  echo
  echo "Restoring $BKT_H..."
  mv $BKT_H.bak $BKT_H
  trap - SIGINT
}

# make a backup and restore it if the script is interrupted
# disable CTRL-C during this operation
trap '' SIGINT
cp $BKT_H $BKT_H.bak
trap bkt_restore EXIT
trap - SIGINT

# cleanup old test files
rm tests/*/bkt-*

echo
echo "Running the BKT_CROSSCHECK_CALLSTACK test..."
echo "======================================="
echo 

# enable the crosscheck-callstack test
# it makes sure backtrace gives results consistent with callstack
# by comparing the two on every function call
# if we can boot the GUI while running this test, that's a good sign
cp $BKT_H.bak $BKT_H
sed -i 's!//#define BKT_CROSSCHECK_CALLSTACK!#define BKT_CROSSCHECK_CALLSTACK!' $BKT_H
diff $BKT_H.bak $BKT_H

# only check on a few models that can boot the GUI without patches
# this test would fail on a GDB breakpoint
env LOG_NAME='tests/$CAM/bkt-callstack.log' \
    ML_PLATFORMS="500D.111/ 60D.111/ 5D2.212/ 5D3.113/" \
    BOOT=0 \
    QEMU_SCRIPT='sleep 20; echo screendump tests/$CAM/bkt-callstack.ppm' \
    QEMU_ARGS="-d callstack" \
    GDB_SCRIPT=none \
    ./run_ml_all_cams.sh

# todo: cross-check with regular GUI runs (*/gui.md5)
md5sum tests/*/bkt-callstack.ppm




echo
echo "Running the BKT_CROSSCHECK_EXEC test..."
echo "======================================="
echo 

# this makes sure we are interpreting SP- and LR-modifying instructions
# in the same way as QEMU (during the execution of guest code)
# it's a low-level test that doesn't quite pass cleanly yet
# note: this test requires BKT_HANDLE_UNLIKELY_CASES
# if we can boot the GUI while running this test, that's a good sign
cp $BKT_H.bak $BKT_H
sed -i 's!//#define BKT_CROSSCHECK_EXEC!#define BKT_CROSSCHECK_EXEC!' $BKT_H
sed -i 's!//#define BKT_HANDLE_UNLIKELY_CASES!#define BKT_HANDLE_UNLIKELY_CASES!' $BKT_H
diff $BKT_H.bak $BKT_H

env LOG_NAME='tests/$CAM/bkt-exec.log' \
    ML_PLATFORMS="500D.111/ 60D.111/ 5D2.212/ 5D3.113/" \
    BOOT=0 \
    QEMU_SCRIPT='sleep 20; echo screendump tests/$CAM/bkt-exec.ppm' \
    QEMU_ARGS="-d callstack" \
    GDB_SCRIPT=none \
    ./run_ml_all_cams.sh

md5sum tests/*/bkt-exec.ppm
