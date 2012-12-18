#! /bin/bash  
# patch the SD/CF card bootsector to make it bootable on Canon DSLR
# See http://chdk.setepontos.com/index.php/topic,4214.0.html
#     http://en.wikipedia.org/wiki/File_Allocation_Table#Boot_Sector

# Fix for osx, auto detects the card if formatted incamera before using this script
if [[ $OSTYPE == darwin* ]]; then   # Existing: OS X
  dev=/dev/$(diskutil list | grep EOS_DIGITAL | awk '{print $6}' )
  echo $dev
  diskutil unmount $dev
elif [[ $OSTYPE == linux* ]]; then   # New: Linux
  dev=$(mount | grep EOS_DIGITAL | awk '{print $1}' )
  if [ "x$dev" = "x" ]; then
    echo "The EOS_DIGITAL card should be mounted before running the script."
    exit
  fi
  echo "Found $dev"
  if [ $(id -u) != 0 ]; then
    echo "dd operations require you to have access to the device, run script as root to be sure"
    exit
  fi
  umount $dev
fi

# read the boot sector to determine the filesystem version
EXFAT=`dd if=$dev bs=1 skip=3 count=8`
DEV32=`dd if=$dev bs=1 skip=82 count=8`
DEV16=`dd if=$dev bs=1 skip=54 count=8`
if [ "$DEV16" != 'FAT16   ' -a "$DEV32" != 'FAT32   ' -a "$EXFAT" != 'EXFAT   ' ]; then
  echo "Error: $dev is not a FAT16, FAT32 of EXFAT device"
  echo "Format your card in camera before using this script"
  echo debug $dev $DEV16 $DEV32 $EXFAT
  exit
fi
if [ "$DEV16" = 'FAT16   ' ]; then
  offset1=43
  offset2=64
  FS='FAT16'
elif [ "$DEV32" = 'FAT32   ' ]; then
  offset1=71
  offset2=92
  FS='FAT32'
elif [ "$EXFAT" = 'EXFAT   ' ]; then
  offset1=130
  offset1b=6274
  offset2=122
  offset2b=6266
  FS='EXFAT'
else
  echo "Error: "$dev" is not a FAT16, FAT32 or EXFAT device"
  exit
fi
echo "Applying "$FS" parameters on "$dev" device:"
echo " writing EOS_DEVELOP at offset" $offset1 "(Volume label)"
echo EOS_DEVELOP | dd of="$dev" bs=1 seek=$offset1 count=11
echo " writing BOOTDISK at offset" $offset2 "(Boot code)"
echo BOOTDISK | dd of="$dev" bs=1 seek=$offset2 count=8
if [ "$FS" = 'EXFAT' ]; then
echo " writing EOS_DEVELOP at offset" $offset1b "(Volume label)"
echo EOS_DEVELOP | dd of="$dev" bs=1 seek=$offset1b count=11
echo " writing EOS_DEVELOP at offset" $offset2b "(Boot code)"
echo EOS_DEVELOP | dd of="$dev" bs=1 seek=$offset2b count=8
fi