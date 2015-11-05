#!/bin/bash
# Converts Dual ISO RAW videos to AVI MJPEG, with tone mapping
# usage: dualisovideo-enfuse.sh foo.raw
# output: video.avi
# You may want to tweak exposure or whatever other ufraw settings

# working directories (you may delete them afterwards, or inspect/use the individual frames)
mkdir tmp tmp/raw tmp/hi tmp/lo tmp/out
rm tmp/* tmp/raw/* tmp/hi/* tmp/lo/* tmp/out/*

# raw processing
raw2dng $1 tmp/raw/
cr2hdr tmp/raw/*.dng --same-levels

# develop the raw files at two exposures (for shadows and highlights)
ufraw-batch --out-type=jpg tmp/raw/*.DNG --out-path=tmp/hi --exposure=4 --clip=film
ufraw-batch --out-type=jpg tmp/raw/*.DNG --out-path=tmp/lo --exposure=-1 --clip=film --restore=hsv

# tonemap with enfuse
cd tmp/hi/
for f in *.jpg ; do enfuse $f ../lo/$f -o ../out/$f ; done
cd ../..

# encode the video
# make sure you use ffmpeg from the ffmpeg web site, not the libav fork that comes with ubuntu
rm video.avi
ffmpeg -i tmp/out/%6d.jpg -vcodec mjpeg -qscale 1 video.avi
