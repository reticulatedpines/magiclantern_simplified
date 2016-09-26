#!/bin/bash

echo "This will mark *all* test results as good."
echo "Will open all screenshots in gthumb so you can review them."
sleep 2
gthumb */*.ppm 2>/dev/null

echo -n "Continue? [y/n] "
read answer
if test "$answer" != "Y" -a "$answer" != "y"; then exit 0; fi

rm */*.md5

for dir in */; do
    echo $dir
    cd $dir
    for PPM in gui disp; do
      if [ -e $PPM.ppm ]; then
        md5sum $PPM.ppm > $PPM.md5
        cat $PPM.md5
      fi
    done
    cd ..
done
