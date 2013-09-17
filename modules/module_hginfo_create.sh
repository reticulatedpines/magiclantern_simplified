#!/bin/bash

LANG=enUS

# get the history of the current directory. if it is empty, the directory isn't part of the current repository
history=`hg history .`

# encode all information using gzip and hexencode it. the resulting string is formatted e.g. "xdexadxbexef"
if [ ${#history} -gt 0 ]; then
    diff_str=`(hg diff -g .) | gzip | od -tx1 | sed -e 's/^[0-9]* //' -e '$d' -e 's/^/ /' -e 's/ /x/g' | tr -d '\n'`
else
    diff_str=`(echo "# This directory is not in repository") | gzip | od -tx1 | sed -e 's/^[0-9]* //' -e '$d' -e 's/^/ /' -e 's/ /x/g' | tr -d '\n'`
fi
repo_str=`(echo "Repo paths:"; echo -ne "path = "; hg root; hg paths; echo "Log:"; hg log --limit 1) | gzip | od -tx1 | sed -e 's/^[0-9]* //' -e '$d' -e 's/^/ /' -e 's/ /x/g' | tr -d '\n'`

# now print the string and replace all occurences of 'x' with '\x'
echo -ne "\n"
echo -ne 'char __module_hgdiff[] MODULE_HGDIFF_SECTION = "'${diff_str//x/\\\\x}'";\n'
echo -ne 'char __module_hginfo[] MODULE_HGINFO_SECTION = "'${repo_str//x/\\\\x}'";\n'
echo -ne "\n"

# done.
