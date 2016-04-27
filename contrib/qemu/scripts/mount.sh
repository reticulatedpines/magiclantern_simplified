
echo "This will mount sd.img and cf.img as loopback, using kpartx."
echo "Please enter your password (of course, after reviewing what this script does)."
sudo kpartx -av sd.img
sudo kpartx -av cf.img


echo "Done."
echo "To remove the device mappings, run:"
echo "   sudo kpartx -dv sd.img"
echo "   sudo kpartx -dv cf.img"
