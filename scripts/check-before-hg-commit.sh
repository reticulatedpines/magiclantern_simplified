#!/bin/sh

CHECKPATCH=scripts/checkpatch.pl

if [ ! -x "$CHECKPATCH" ]; then
	echo "no checkpatch utility ($CHECKPATCH)!"
	exit 1
fi

TFILE=$(mktemp)

hg diff > $TFILE

$CHECKPATCH $TFILE

rm -f $TFILE
