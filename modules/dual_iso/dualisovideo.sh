#!/bin/bash
# Converts Dual ISO RAW videos to AVI MJPEG
# usage: dualisovideo.sh foo.raw
# output. video.avi
# You may want to tweak exposure or whatever other ufraw settings, maybe use enfuse for tonemapping...

mkdir tmp
rm tmp/*
raw2dng $1 tmp/
cr2hdr tmp/*.dng --same-levels
ufraw-batch --out-type=jpg tmp/*.DNG --exposure=4 --clip=film
rm video.avi
ffmpeg -i tmp/%6d.jpg -vcodec mjpeg -qscale 1 video.avi
