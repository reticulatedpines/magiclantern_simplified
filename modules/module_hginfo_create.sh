#!/bin/bash

LANG=enUS
echo -ne "\n\n"
echo -ne "char __module_hgdiff[] MODULE_HGDIFF_SECTION = \""
hg diff . | gzip | od -tx1 | sed -e 's/^[0-9]* //' -e '$d' -e 's/^/ /' -e 's/ /\\x/g' | tr -d '\n'
echo -ne "\";\n"
echo -ne "char __module_hginfo[] MODULE_HGINFO_SECTION = \""
(hg paths; hg parent) | gzip | od -tx1 | sed -e 's/^[0-9]* //' -e '$d' -e 's/^/ /' -e 's/ /\\x/g' | tr -d '\n'
echo -ne "\";\n"
echo -ne "\n\n"
