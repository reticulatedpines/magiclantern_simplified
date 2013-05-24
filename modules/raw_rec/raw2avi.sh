#!/bin/bash
# Converts RAW videos to AVI MJPEG
# usage: raw2avi foo.raw
# output. video.avi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

${DIR}/raw2dng $1
ufraw-batch --out-type=jpg *.dng
rm *.dng
rm video.avi
ffmpeg -i %6d.jpg -vcodec mjpeg -qscale 1 video.avi
rm *.jpg
