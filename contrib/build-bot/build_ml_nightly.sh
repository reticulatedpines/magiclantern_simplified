#!/bin/bash

NIGHTLY_ROOT="$HOME/public_html/nightly"
BUILD_ROOT="$HOME/magic-lantern-official"
BUILD_LOG="$BUILD_ROOT/build.log"
CHANGE_LOG="$BUILD_ROOT/ChangeLog.txt"
FEATURES="$BUILD_ROOT/features.html"

unlink $NIGHTLY_ROOT/build.log
unlink $NIGHTLY_ROOT/ChangeLog.txt
unlink $NIGHTLY_ROOT/features.html
unlink $NIGHTLY_ROOT/magiclantern*.zip

cd $BUILD_ROOT
hg pull
hg update unified
make nightly 2>&1 | tee build.log
if [ -f $BUILD_LOG ]; then
    mv $BUILD_LOG $NIGHTLY_ROOT
fi
if [ -f $CHANGE_LOG ]; then
    mv $CHANGE_LOG $NIGHTLY_ROOT
fi
if [ -f $FEATURES ]; then
    mv $FEATURES $NIGHTLY_ROOT
fi
cd -
