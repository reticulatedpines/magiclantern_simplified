
echo "This will mount sd.img as loopback, using kpartx."
echo "Please enter your password (of course, after reviewing what this script does)."
sudo kpartx -av sd.img


echo "Done."
echo "To remove the device mapping, run: sudo kpartx -dv sd.img"
