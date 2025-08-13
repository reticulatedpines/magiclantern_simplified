#!/bin/bash

MLV_PATH=/media/sf_D_DRIVE/temp/raw/t1/
MLV_FILE=$MLV_PATH/M08-0025.MLV
OUT_FILE=$MLV_PATH/OUT
FRAMES=5
BIT_DEPTH=12
LZMA_LEVEL=9

echo ""
echo "WARNING: This script is meant for internal testing."
echo "         It loads a .mlv, reduces bit depth and"
echo "         reverts the format back again for feeding to"
echo "         raw2dng converter."
echo "         It shows the achieved compression rate."
echo ""

# clean up first
rm -f $MLV_PATH/*.dng
rm -f $MLV_PATH/*.jpg
rm -f $OUT_FILE.*

# process MLV
echo "[1] Create a .mlv with $FRAMES frames only..."
./mlv_dump -f$FRAMES -o $OUT_FILE.snap $MLV_FILE > /dev/null

echo "[2] Reducing bit depth and compressing..."
./mlv_dump -b $BIT_DEPTH -c -l $LZMA_LEVEL -o $OUT_FILE.low $OUT_FILE.snap > /dev/null

echo "[3] Dump to legacy raw..."
#./mlv_dump -d -b 14 -o $OUT_FILE.high $OUT_FILE.low  > /dev/null
./mlv_dump -r -o $OUT_FILE.raw $OUT_FILE.low > /dev/null

ORIG_FILESIZE=$(stat -c%s "$OUT_FILE.snap")
LOW_FILESIZE=$(stat -c%s "$OUT_FILE.low")
PCT_LOW=`bc <<< "scale=4;$LOW_FILESIZE*100/$ORIG_FILESIZE"`

echo "[i] Original size: $ORIG_FILESIZE --> $LOW_FILESIZE (shrinked to $PCT_LOW%)"
echo ""
echo ""

# feed to legacy converter
echo "[4] Running raw2dng..."
./raw2dng $OUT_FILE.raw $MLV_PATH

echo "[5] Converting to JPEG"
ufraw-batch --out-type=jpg $MLV_PATH/*.dng > /dev/null 2>&1

# cleanup
rm -f $MLV_PATH/*.dng
rm -f $OUT_FILE.*

echo "[i] Done"
