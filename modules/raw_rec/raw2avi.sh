#!/usr/bin/sh

# Converts RAW videos to AVI MJPEG
# usage: raw2avi foo.raw
# output. video.avi

./raw2dng $1
ufraw-batch --out-type=jpg *.dng
rm *.dng
rm video.avi
ffmpeg -i %6d.jpg -vcodec mjpeg -qscale 1 video.avi
rm *.jpg
