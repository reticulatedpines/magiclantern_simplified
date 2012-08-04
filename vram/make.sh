#!/bin/bash

rm *.lut

for f in $(ls *.pto); do
    echo `basename $f .pto`
    nona -o `basename $f .pto`.tif $f xy.tiff
    octave defish-lut16.m `basename $f .pto`
done

mv samyang8-panini-apsc.lut apsc8p.lut
mv samyang8-rectilin-apsc.lut apsc8r.lut
mv samyang8-panini-ff.lut ff8p.lut
mv samyang8-rectilin-ff.lut ff8r.lut
