#!/usr/bin/env bash

QEMU_PATH=${QEMU_PATH:=qemu-2.5.0}
OUT_DIR=`realpath coverage`
EOS_BINDIR=`realpath $QEMU_PATH/arm-softmmu/hw/eos`
EOS_SRCDIR=`realpath $QEMU_PATH/hw/eos`

cd $EOS_BINDIR

if [ "$1" == "-z" ]; then
    lcov -z -d .
    rm -f *.info
    exit
fi

lcov -c -i -d . -o base.info --no-external -b $EOS_SRCDIR
lcov -c -d . -o test.info --no-external -b $EOS_SRCDIR --rc lcov_branch_coverage=1
lcov -a base.info -a test.info -o total.info --rc lcov_branch_coverage=1
genhtml total.info --output-directory $OUT_DIR --branch-coverage
