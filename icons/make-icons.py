from PIL import Image
import os
import copy

W = 46
H = 42

# do not encode outer pixels, to save some RAM
Xpad = 3
Ypad = 4
Hpadded = H - Ypad*2
Wpadded = W - Xpad*2

icons = [
    'audio.png',
    'expo.png',
    'overlay.png',
    'movie.png',
    'shoot.png',
    'focus.png',
    'display.png',
    'prefs.png',
    'debug.png',
    'info.png',
    'mymenu.png',
    'script.png',
    'Q-forward.png',
    'Q-back.png',
    'forward.png',
    'modules.png',
    'modified.png',
    'games.png',
]


# On low-res screens, the audio and focus icons are aliasing
icons_lowres = copy.deepcopy(icons)
# exchange only low resolution icons
for n,i in enumerate(icons):
    if i=='audio.png':
        icons_lowres[n]='audio-lowres.png'

    if i=='focus.png':
        icons_lowres[n]='focus-lowres.png'


# the 100D has no Q button, instead the Av button has the same functionality
icons_100D = copy.deepcopy(icons)
# exchange only 100D model specific icons
for n,i in enumerate(icons):
    if i=='Q-forward.png':
        icons_100D[n]='Av-forward.png'

    if i=='Q-back.png':
        icons_100D[n]='Av-back.png'

# the 500D has no Q button, instead the LiveView button has the same functionality
icons_500D = copy.deepcopy(icons)
# exchange only 500D model specific icons
for n,i in enumerate(icons):
    if i=='Q-forward.png':
        icons_500D[n]='LiveView-forward.png'

    if i=='Q-back.png':
        icons_500D[n]='LiveView-back.png'

# the 50D has no Q button, instead the FUNC button has the same functionality
icons_50D = copy.deepcopy(icons)
# exchange only 50D model specific icons
for n,i in enumerate(icons):
    if i=='Q-forward.png':
        icons_50D[n]='FUNC-forward.png'

    if i=='Q-back.png':
        icons_50D[n]='FUNC-back.png'


# the 5D Mark II has no Q button, instead the picture style button has the same functionality
icons_5D2 = copy.deepcopy(icons)
# exchange only 5D2 model specific icons
for n,i in enumerate(icons):
    if i=='Q-forward.png':
        icons_5D2[n]='picstyle-forward.png'

    if i=='Q-back.png':
        icons_5D2[n]='picstyle-back.png'


def process(icons, filename, labels):
    # write icons in textual representation to *.in file
    tmp = open(filename+".in", "w")
    k = 0
    for ico in icons:
        print >> tmp, '%s = ' % chr(ord('a') + k)
        k += 1
        im = Image.open(ico).convert("1")
        for y in range(Hpadded):
            for x in range(Wpadded):
                tmp.write("#" if im.getpixel((x+Xpad, y+Ypad)) else " ")
            print >> tmp, ""
        print >> tmp, ""

    tmp.close()
    # convert *.in files to *.c class files
    os.system("perl mkfont-canon < %s.in > %s.c -width %d -height %d -labels %s" % (filename, filename, Wpadded, Hpadded, labels))

# given a list of matching text blocks and a list of ifdefs,
# return a compact representation of those lines
def ifdef(blocks, ifdefs):
    uniq = list(set(blocks))

    # all blocks identical? that's easy :)
    if len(uniq) == 1:
        return uniq[0]

    assert len(blocks) == len(ifdefs)
    N = len(blocks)

    # there is one ifdef that is None - that one goes to the "else" branch
    # assume it's the first one
    assert ifdefs[0] == None

    # print the different blocks with ifdefs
    # todo: if things start growing, we may consider some boolean optimizations
    out = ""
    first = True
    for i in range(N):
        if blocks[i] != blocks[0]:
            out += "#%sif defined(%s)\n" % ("" if first else "el", ifdefs[i])
            first = False
            out += blocks[i]
    out += "#else\n"
    out += blocks[0]
    out += "#endif\n"

    return out

# return a matrix D which summarizes the differences between lines
# D[i,j] means: lines[i] and lines[j] are identical
def diff_matrix(lines):
    N = len(lines)
    D = []
    for i in range(N):
        D.append([False] * N)

    for i in range(N):
        for j in range(N):
            D[i][j] = (lines[i] == lines[j])

    return D

def merge_ifdef(files, ifdefs, output):
    # simplified merging of multiple files, similar to diff --ifdef
    # assumption: all the input files have the same number of lines
    # and they are already matched line-by-line
    # some lines may differ between files, and those have to be merged with ifdefs
    contents = []
    lengths = []
    for f in files:
        lines = open(f).readlines()
        contents.append(lines)
        lengths.append(len(lines))
    assert min(lengths) == max(lengths)

    of = open(output, "w")

    N = lengths[0]
    i = 0
    while i < N:
        # extract just line i from all files
        lines_i = [lines[i] for lines in contents]

        # group lines that are different in the same way
        # starting from line i
        D = diff_matrix(lines_i)
        j = i
        while j < N and diff_matrix([lines[j] for lines in contents]) == D:
            j += 1

        # will group lines from i to j
        lines_g = ["".join(lines[i:j]) for lines in contents]

        # summarize the grouped lines with ifdefs
        print >> of, ifdef(lines_g, ifdefs),
        i = j

    of.close()

def build_files(icon_lists):
    # generate class files from the list of PNG icons
    files = []
    ifdefs = []
    for icon_list, label, ifdef in icon_lists:
        # use the new label only for those characters that were changed
        # from the default list (unchanged characters will be labeled as "all")
        labels = ",".join([label if icon_list[i] != icons[i] else "all" for i in range(len(icons))])
        file_prefix = "ico-" + label
        process(icon_list, file_prefix, labels)

        # build a file list and an ifdef list with what we have processed
        files.append(file_prefix + ".c")
        ifdefs.append(ifdef)

    return files, ifdefs

icon_lists = [
        (icons,         "all",  None),
        (icons_100D,    "100D", "CONFIG_100D"),
        (icons_500D,    "500D", "CONFIG_500D"),
        (icons_50D,     "50D",  "CONFIG_50D"),
        (icons_5D2,     "5D2",  "CONFIG_5D2"),
        (icons_lowres,  "low",  "CONFIG_LOW_RESOLUTION_DISPLAY"),
    ]

# process each icon set individually
files, ifdefs = build_files(icon_lists)

# make a merged #ifdef format output
merge_ifdef(files, ifdefs, "../src/ico.c")

# clean up, remove all generated class files
os.system("rm *.in *.c")