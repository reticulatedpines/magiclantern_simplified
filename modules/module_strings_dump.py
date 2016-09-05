# print the module_strings section from a ML module (*.mo)
# sorry, not compatible with Python 3 (patches welcome)
import os, sys
from struct import unpack

def getLongLE(d, a):
   return unpack('<L',(d)[a:a+4])[0]

def getString(d, a):
    return d[a : d.index('\0',a)]

strings = open(sys.argv[1], "rb").read()

# iterate through all module strings
a = 0
while getLongLE(strings, a):
    key = getString(strings, getLongLE(strings, a))
    val = getString(strings, getLongLE(strings, a+4)).strip("\n")
    # display max 3 lines, indented to match the other strings
    val = "\n              ".join(val.split("\n")[0:3]) \
        + (" [...]" if val.count("\n") > 3 else "")
    print("%-12s: %s" % (key, val))
    a += 8
