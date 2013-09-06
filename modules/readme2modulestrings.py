# extract module strings from README.rst

import sys, re
import commands

def run(cmd):
    return commands.getstatusoutput(cmd)[1]

inp = open("README.rst").read().replace("\r\n", "\n")
lines = inp.strip("\n").split("\n")
title = lines[0]

used_lines = []
for l in lines[2:]:
    if l.startswith("..") or l.strip().startswith(":"):
        continue
    
    used_lines.append(l)

inp = "\n".join(used_lines)
inp = inp.split("\n\n")

print('MODULE_STRINGS_START()')
print('    MODULE_STRING("Name", "%s")' % title)

# extract user metadata from RST meta tags
tags = {}
for l in lines[2:]:
    l = l.strip()
    m = re.match("^:([^:]+):([^:]+)$", l)
    if m:
        name = m.groups()[0].strip()
        value = m.groups()[1].strip()
        if value.startswith("<") and value.endswith(">"):
            continue
        print('    MODULE_STRING("%s", "%s")' % (name, value))
        tags[name] = value

if "Author" not in tags and "Authors" not in tags:
    print >> sys.stderr, "Warning: 'Author/Authors' tag is missing. You should tell the world who wrote your module ;)"

if "License" not in tags:
    print >> sys.stderr, "Warning: 'License' tag is missing. Under what conditions we can use your module? Can we publish modified versions?"

if "Summary" not in tags:
    print >> sys.stderr, "Warning: 'Summary' tag is missing. It should be displayed as help in the Modules tab."

# extract readme body:
# intro -> "Description" tag;
# each section will become "Help page 1", "Help page 2" and so on
print('    MODULE_STRING("Description", ')

# render the RST as html -> txt without the metadata tags
txt = run('cat README.rst | grep -v -E "^:([^:])+:([^:])+$" | rst2html --no-xml-declaration | python ../html2text.py -b 700')

help_page_num = 0
lines_per_page = 0
for p in txt.strip("\n").split("\n")[2:]:
    if p.startswith("# "): # new section
        print('    )')
        help_page_num += 1
        print('    MODULE_STRING("Help page %d", ' % help_page_num)
        lines_per_page = 0
        p = p[2:].strip()
    print '        "%s\\n"' % p.replace('"', '\\"')
    lines_per_page += 1
    if lines_per_page > 20:
        print >> sys.stderr, "Too many lines per page\n"
        print('    )')
        exit(1)

print('    )')

# extract version info
# (prints the latest changeset that affected this module)
last_changeset = run("hg log . -l 1 --template '{node|short}'")
last_change_date = run("LC_TIME=EN date -u -d \"`hg log . -l 1 --template '{date|isodate}'`\" '+%Y-%m-%d %H:%M:%S %Z'")
build_date = run("LC_TIME=EN date -u '+%Y-%m-%d %H:%M:%S %Z'")
build_user = run("echo `whoami`@`hostname`")

print('    MODULE_STRING("Last update", "%s (%s)")' % (last_change_date, last_changeset))
print('    MODULE_STRING("Build date", "%s")' % build_date)
print('    MODULE_STRING("Build user", "%s")' % build_user)

print('MODULE_STRINGS_END()')
