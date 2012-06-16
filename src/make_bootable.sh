#! /bin/bash  
# patch the SD/CF card bootsector to make it bootable on Canon DSLR
# See http://chdk.setepontos.com/index.php/topic,4214.0.html
#     http://en.wikipedia.org/wiki/File_Allocation_Table#Boot_Sector

# change this on linux
dev=/dev/sdb1

# Fix for osx, auto detects the card if formatted incamera before using this script
if [[ $OSTYPE == darwin* ]]; then
  dev=/dev/$(diskutil list | grep EOS_DIGITAL | awk '{print $6}' )
  echo $dev
  diskutil unmount $dev
fi

# read the boot sector to determine the filesystem version
DEV32=`dd if=$dev bs=1 skip=82 count=8`
DEV16=`dd if=$dev bs=1 skip=54 count=8`
if [ "$DEV16" != 'FAT16   ' -a "$DEV32" != 'FAT32   ' ]; then
  echo "Error: "$dev" is not a FAT16 or FAT32 device"
  echo "Format your card in camera before using this script on Osx"
  echo debug $dev $DEV16 $DEV32
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
else
  echo "Error: "$dev" is not a FAT16 or FAT32 device"
  exit
fi
echo "Applying "$FS" parameters on "$dev" device:"
echo " writing EOS_DEVELOP at offset" $offset1 "(Volume label)"
echo EOS_DEVELOP | dd of="$dev" bs=1 seek=$offset1 count=11
echo " writing BOOTDISK at offset" $offset2 "(Boot code)"
echo BOOTDISK | dd of="$dev" bs=1 seek=$offset2 count=8
