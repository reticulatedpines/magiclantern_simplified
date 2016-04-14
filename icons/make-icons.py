from PIL import Image
import os
import copy
import time

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


def process(icons, outfile):
    tmp = open("ico.in", "w")
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

    os.system("perl mkfont-canon < ico.in > %s -width %d -height %d" % (outfile, Wpadded, Hpadded))

process(icons, "ico.c")
process(icons_50D, "ico-50D.c")
process(icons_5D2, "ico-5D2.c")
process(icons_lowres, "ico-lowres.c")

# make a merged #ifdef format output
# (see also http://www.gnu.org/software/diffutils/manual/html_node/If_002dthen_002delse.html#If_002dthen_002delse )

# merge icons with low resolution versions
os.system("diff -D CONFIG_LOW_RESOLUTION_DISPLAY ico.c ico-lowres.c > ico_with_lowres_merged.c")

# merge 50D specific icons
os.system("diff -D CONFIG_50D ico_with_lowres_merged.c ico-50D.c > ../src/ico.c")

# clean up, remove all generated class files
time.sleep(2)
os.remove("ico.c")
os.remove("ico.in")
os.remove("ico-50d.c")
os.remove("ico-5D2.c")
os.remove("ico-lowres.c")
os.system("rm *.c")
