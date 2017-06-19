#!/usr/bin/env bash

echo "This will mount sd.img and cf.img as a loopback device."

if [ $(uname) == "Darwin" ]; then
    hdiutil attach sd.img
    hdiutil attach cf.img
    echo "Done."
    echo "To remove the device mappings, run:"
    echo '   hdiutil detach "/Volumes/EOS_DIGITAL"'
    echo '   hdiutil detach "/Volumes/EOS_DIGITAL 1"'
else
    echo "Please enter your password (of course, after reviewing what this script does)."
    sudo kpartx -av sd.img
    sudo kpartx -av cf.img
    echo "Done."
    echo "To remove the device mappings, run:"
    echo "   sudo kpartx -dv sd.img"
    echo "   sudo kpartx -dv cf.img"
fi
