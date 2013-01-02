#!/bin/bash

processFrames() {
noframes=`ls *.png | wc -l`
echo "$noframes frames extracted..."
n=0
for i in `seq 1 2 $noframes`; do
in1=`printf "%08d.png" "$i"`
in2=`printf "%08d.png" "$((i+1))"`
out=`printf "out%08d.jpg" "$n"`
echo "creating HDR frame $out from $in1 and $in2 ..."
enfuse --compression=95 -o $out $in1 $in2
let n++
done
}
encodeVideo() {
# 2 stage encoding with mpreg4 codec, settings from mencoder wiki
# optimal_bitrate = (40..60) * 25 * width * height / 256
opts="vbitrate=12150000:mbd=2:keyint=132:v4mv:vqmin=3:vlelim=-4:vcelim=7:lumi_mask=0.07:dark_mask=0.10:naq:vqcomp=0.7:vqblur=0.2:mpeg_quant"
mencoder mf://./out*.jpg -mf w=1920:h=1080:fps=15:type=jpg -oac copy -ovc lavc -lavcopts vcodec=mpeg4:vpass=1:$opts -o /dev/null
mencoder mf://./out*.jpg -mf w=1920:h=1080:fps=15:type=jpg -oac copy -ovc lavc -lavcopts vcodec=mpeg4:vpass=2:$opts -o output.avi
}
cleanup() {
rm *.png
rm out*.jpg
}

mplayer -vo png:z=1 $1
processFrames
encodeVideo
cleanup
exit 