#!/usr/bin/env bash

echo "This will mark *all* test results as good."
echo "Will open all screenshots in gthumb so you can review them."
sleep 2
gthumb */*.p[pn][mg] 2>/dev/null

echo -n "Continue? [y/n] "
read answer
if test "$answer" != "Y" -a "$answer" != "y"; then exit 0; fi

rm */*.md5

for dir in */; do
    echo $dir
    cd $dir
    for TEST in gui disp frsp menu format; do
      if ls $TEST*.p[pn][mg] > /dev/null 2>&1; then
        md5sum $TEST*.p[pn][mg] > $TEST.md5
        cat $TEST.md5
      fi
    done
    cd ..
done
