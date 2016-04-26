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


def process(icons, filename, camera_model):
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
    os.system("perl mkfont-canon < %s.in > %s.c -width %d -height %d -camera_model %s" % (filename, filename, Wpadded, Hpadded, camera_model))

# generate class files from the list of PNG icons
process(icons, "ico", "ico")
process(icons_100D, "ico-100D", "100D")
process(icons_50D, "ico-50D", "50D")
process(icons_5D2, "ico-5D2", "5D2")
process(icons_lowres, "ico-lowres", "low")

# make a merged #ifdef format output
# (see also http://www.gnu.org/software/diffutils/manual/html_node/If_002dthen_002delse.html#If_002dthen_002delse )

# merge default with 50D specific icons
os.system("diff --ifdef=CONFIG_50D ico.c ico-50D.c > ico-with-50D.c")

# merge 50D with 5D2 specific icons
os.system("diff --ifdef=CONFIG_5D2 ico-with-50D.c ico-5D2.c > ico-with-50D-5D2.c")

# merge with 100D specific icons
os.system("diff --ifdef=CONFIG_100D ico-with-50D-5D2.c ico-100D.c > ico-with-50D-5D2-100D.c")

# merge icons with low resolution versions
# copy result to class file
os.system("diff --ifdef=CONFIG_LOW_RESOLUTION_DISPLAY ico-with-50D-5D2-100D.c ico-lowres.c > ../src/ico.c")

# clean up, remove all generated class files
os.system("rm *.in *.c")
