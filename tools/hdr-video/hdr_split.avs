# Magic Lantern HDR splitter
# 
# Splits the odd and even streams in two image sequences
#
# Installation:
#
# * Avisynth 2.6 Alpha 3 - http://avisynth.org/
# * InterFrame plugin - http://www.spirton.com/interframe/
# * VirtualDub - http://www.virtualdub.org/
#
# Usage:
# 
# * Place RAW.MOV in the same directory as this script
# * Create an empty subdirectory named "frames"
# * Open this script in VirtualDub
# * Play the video in VirtualDub. During this process, Avisynth will
#   split the two streams and create two image sequences, starting from 
#   A000000.jpg and B000000.jpg.
# * Batch-process the two streams in your favorite HDR image software.
# * Load the blended stream into VirtualDub and save it as AVI.
#
#
# Tip: if you would like a click-and-run workflow, support Magic Lantern development
# and you'll receive a complete package, ready to unzip and run.
#
# https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=3DALYKMQTP6SN

SetMemoryMax(1024)

A = FFVideoSource("RAW.MOV")
A = ConvertToRGB(A, matrix="PC.601", interlaced=false)
A = Converttoyv12(A)           # convert to yv12 needed for Interframe
A = selecteven(A)              # select even or odd frames and interpolate them 
A = assumefps(A, 25)           # Interframe doesn't work with 12,5 fps 
A = InterFrame(A, NewNum=50, NewDen=1, GPU=false, FlowPath="C:\Program Files\AviSynth 2.5\plugins\")
A = assumefps(A, 25)
A = trim(A, 1, 0)
A = ConvertToRGB(A)
A = ImageWriter(A, "frames\A", type = "jpg")

B = FFVideoSource("RAW.MOV")
B = ConvertToRGB(B, matrix="PC.601", interlaced=false)
B = Converttoyv12(B)          # convert to yv12 needed for Interframe
B = selectodd(B)              # select even or odd frames and interpolate them 
B = assumefps(B, 25)          # Interframe doesn't work with 12,5 fps 
B = InterFrame(B, NewNum=50, NewDen=1, GPU=false, FlowPath="C:\Program Files\AviSynth 2.5\plugins\")
B = assumefps(B, 25)
B = ConvertToRGB(B)
B = ImageWriter(B, "frames\B", type = "jpg")

return Interleave(A,B)
