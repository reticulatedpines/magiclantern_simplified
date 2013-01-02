#!/bin/bash
# This script assumes that you have cloned the repository under 
# $HOME/magic-lantern-official
cd $HOME/magic-lantern-official
hg pull
hg update unified
make nightly 2>&1 | tee build.log
cd -
