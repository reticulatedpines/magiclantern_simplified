#!/usr/bin/env bash

# Set up environment variables for accessing SD/CF card images via mtools.
# To be executed in the same shell as the caller, e.g.
# . ./mtools_setup.sh

[ "$0" = "$BASH_SOURCE" ] && echo "This script must be 'sourced':
   . ./mtools_setup.sh"

# We will use mtools to alter and check the SD/CF image contents.
# fixme: hardcoded partition offset
MSD=sd.img@@50688
MCF=cf.img@@50688

# mtools doesn't like our SD image, for some reason
export MTOOLS_SKIP_CHECK=1
export MTOOLS_NO_VFAT=1

# some systems have this on by default; our tests assume it off
export MTOOLS_LOWER_CASE=0
