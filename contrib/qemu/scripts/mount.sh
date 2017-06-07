#!/usr/bin/env bash

echo "This will mount sd.img and cf.img as a loopback device."
echo "Please enter your password (of course, after reviewing what this script does)."

if [ $(uname -s) == "Darwin" ]; then
    sudo hdiutil attach sd.img
    sudo hdiutil attach cf.img
    echo "Done."
    echo "To remove the device mappings, run:"
    echo '   sudo hdiutil detach "/Volumes/EOS_DIGITAL"'
    echo '   sudo hdiutil detach "/Volumes/EOS_DIGITAL 1"'
else
    sudo kpartx -av sd.img
    sudo kpartx -av cf.img
    echo "Done."
    echo "To remove the device mappings, run:"
    echo "   sudo kpartx -dv sd.img"
    echo "   sudo kpartx -dv cf.img"
fi
