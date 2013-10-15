# extract module strings from README.rst

import sys, re
import commands
from datetime import datetime

from align_string_proportional import word_wrap
from rbf_read import extent_func, rbf_init_font
rbf_init_font("../../data/fonts/argnor23.rbf")

def run(cmd):
    return commands.getstatusoutput(cmd)[1]

# see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=37506 for how to place some strings in a custom section
# (you must declare all strings as variables, not only the pointers)

def c_repr(name):
    if "\n" in name:
        s = "\n"
        for l in name.split("\n"):
            s += "    %s\n" % ('"%s\\n"' % l.replace('"', r'\"'))
        return s
    else:
        return '"%s"' % name.replace('"', r'\"')

strings = []
def add_string(name, value):
    a = chr(ord('a') + len(strings))
    print "static char __module_string_%s_name [] MODULE_STRINGS_SECTION = %s;" % (a, c_repr(name))
    print "static char __module_string_%s_value[] MODULE_STRINGS_SECTION = %s;" % (a, c_repr(value))

    strings.append((name, value))

def declare_string_section():
    print
    print('MODULE_STRINGS_START()')
    for i, s in enumerate(strings):
        a = chr(ord('a') + i)
        print "    MODULE_STRING(__module_string_%s_name, __module_string_%s_value)" % (a, a)
    print('MODULE_STRINGS_END()')

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

add_string("Name", title)

# extract user metadata from RST meta tags
tags = {}
for l in lines[2:]:
    l = l.strip()
    m = re.match("^:([^:]+):(.+)$", l)
    if m:
        name = m.groups()[0].strip()
        value = m.groups()[1].strip()
        if value.startswith("<") and value.endswith(">"):
            continue
        add_string(name, value)
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

# render the RST as html -> txt without the metadata tags
txt = run('cat README.rst | grep -v -E "^:([^:])+:.+$" | rst2html --no-xml-declaration | python ../html2text.py -b 700')

desc = ""
last_str = "Description"
help_page_num = 0
lines_per_page = 0
for p in txt.strip("\n").split("\n")[2:]:
    if p.startswith("# "): # new section
        help_page_num += 1
        add_string(last_str, desc)
        desc = ""
        last_str = "Help page %d" % help_page_num
        lines_per_page = 0
        p = p[2:].strip()
    desc += "%s\n" % p
    lines_per_page += 1
    if lines_per_page > 18:
        print >> sys.stderr, "Too many lines per page\n"
        exit(1)

add_string(last_str, desc)

# extract version info
# (prints the latest changeset that affected this module)
last_change_info = run("LC_TIME=EN hg log . -r $(basename $(hg id -n) +):0 -l 1 --template '{date|hgdate}\n{node|short}\n{author|user}\n{desc|strip|firstline}'")
if len(last_change_info):
    last_change_date, last_changeset, author, commit_msg = last_change_info.split("\n")
    split = last_change_date.split(" ")
    seconds = float(split[0])
    last_change_date = datetime.utcfromtimestamp(seconds).strftime("%Y-%m-%d %H:%M:%S UTC")
    
    # trim changeset to 7 chars, like Bitbucket does
    last_changeset = last_changeset[:7]
    
    # trim commit msg to 700px
    size = extent_func(commit_msg)[0]
    if size > 700:
        new_size = 0
        new_msg = ""
        for c in commit_msg:
            new_size += extent_func(c)[0]
            if new_size > 700:
                break
            new_msg += c
        commit_msg = new_msg + "..."
        
    add_string("Last update", "%s on %s by %s:\n%s" % (last_changeset, last_change_date, author, commit_msg))

build_date = datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S UTC")
build_user = run("echo `whoami`@`hostname`")

add_string("Build date", build_date)
add_string("Build user", build_user)

declare_string_section()
