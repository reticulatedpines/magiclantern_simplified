#!/bin/bash  
# patch the SD/CF card bootsector to make it bootable on Canon DSLR
# See http://chdk.setepontos.com/index.php/topic,4214.0.html
#     http://en.wikipedia.org/wiki/File_Allocation_Table#Boot_Sector

# change this
dev=/dev/sdc1

# read the boot sector to determine the filesystem version
DEV32=`dd if=$dev bs=1 skip=82 count=8 2>/dev/null`
DEV16=`dd if=$dev bs=1 skip=54 count=8 2>/dev/null`
if [ "$DEV16" != 'FAT16   ' -a "$DEV32" != 'FAT32   ' ]; then
  echo "Error: "$dev" is not a FAT16 or FAT32 device"
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
echo EOS_DEVELOP | dd of="$dev" bs=1 seek=$offset1 count=11 2>/dev/null
echo " writing BOOTDISK at offset" $offset2 "(Boot code)"
echo BOOTDISK | dd of="$dev" bs=1 seek=$offset2 count=8 2>/dev/null
