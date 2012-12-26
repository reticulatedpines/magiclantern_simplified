#! /bin/bash -x
# v1 : FAT16 and FAT32, based on Trammel version
# v2 : exFAT supported. arm.indiana@gmail.com
# v3 : Osx and Linux auto detect device
#
# patch the SD/CF card bootsector to make it bootable on Canon DSLR
# See http://chdk.setepontos.com/index.php/topic,4214.0.html
#     http://en.wikipedia.org/wiki/File_Allocation_Table#Boot_Sector

# usage: make_bootable.sh (card needs to be formatted on camera or have volume name: EOS_DIGITAL)
# exfat_sum.c must bu compiled first

dump_file=exfat_dump.bin

# Auto detects the card if formatted incamera before using this script
if [[ $OSTYPE == darwin* ]]; then   # OS X
  UNMOUNT='diskutil unmount'
elif [[ $OSTYPE == linux* ]]; then   # Linux
  UNMOUNT='umount'
fi
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
  $UNMOUNT $dev

# read the boot sector to determine the filesystem version
DEV64=`dd if=$dev bs=1 skip=3 count=8 2>/dev/null`
DEV32=`dd if=$dev bs=1 skip=82 count=8 2>/dev/null`
DEV16=`dd if=$dev bs=1 skip=54 count=8 2>/dev/null`
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
elif [ "$DEV64" = 'EXFAT   ' ]; then
  offset1=130
  offset2=122
  FS='EXFAT'
else
  echo "Error: "$dev" is not a FAT16, FAT32 or EXFAT (FAT64) device"
  exit
fi
echo "Applying "$FS" parameters on "$dev" device:"
echo " writing EOS_DEVELOP at offset" $offset1 "(Volume label)"
echo EOS_DEVELOP | dd of="$dev" bs=1 seek=$offset1 count=11
echo " writing BOOTDISK at offset" $offset2 "(Boot code)"
echo BOOTDISK | dd of="$dev" bs=1 seek=$offset2 count=8
if [ "$FS" = 'EXFAT' ]; then
# write them also in backup VBR, at sector 13
echo EOS_DEVELOP | dd of="$dev" bs=1 seek=$(($offset1+512*12)) count=11 2>/dev/null
echo BOOTDISK | dd of="$dev" bs=1 seek=$(($offset2+512*12)) count=8 2>/dev/null
dd if=$dev of="$dump_file" bs=1 skip=0 count=6144 2>/dev/null
echo -n ' recompute checksum. '
./exfat_sum "$dump_file"
# write VBR checksum (from sector 0 to sector 10) at offset 5632 (sector 11) and offset 11776 (sector 23, for backup VBR)
# checksum sector is stored in $dump_file at offset 5632
dd of="$dev" if="$dump_file" bs=1 seek=5632 skip=5632 count=512 2>/dev/null
dd of="$dev" if="$dump_file" bs=1 seek=11776 skip=5632 count=512 2>/dev/null
#dd if=$dev of=verif_dump.bin bs=1 skip=0 count=12288 2>/dev/null
rm -f "$dump_file"
fi