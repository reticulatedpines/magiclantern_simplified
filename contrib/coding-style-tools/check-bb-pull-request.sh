#!/bin/sh
#
# This script uses experimental bitbacket API!
#
# see details:
#   https://answers.atlassian.com/questions/163739/what-is-the-url-of-a-pull-request?page=1
#

CHECKPATCH=scripts/checkpatch.pl

PREQ_NUM=$1

[ "$PREQ_NUM" -gt 0 ] 2>/dev/null
if [ "$?" != 0 ]; then
	echo "Usage:"
	echo "    scripts/check-bb-pull-request.sh <pull request number>"
	exit 2
fi

if [ ! -x "$CHECKPATCH" ]; then
	echo "no checkpatch utility ($CHECKPATCH)!"
	exit 1
fi

TFILE=$(mktemp)

wget https://bitbucket.org/api/2.0/repositories/hudson/magic-lantern/pullrequests/$PREQ_NUM/patch -O $TFILE 2>/dev/null
if [ "$?" = "0" ]; then
	$CHECKPATCH $TFILE | \
		sed "s/^.*\(has no obvious style pr\)/Pull request #$PREQ_NUM \1/"
else
	echo "Can't download pull requst #$PREQ_NUM"
fi

rm -f $TFILE
