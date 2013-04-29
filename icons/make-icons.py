from PIL import Image
import os

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
]

# On low-res screens, the audio and focus icons are aliasing
icons_lowres = [
    'audio-lowres.png', 
    'expo.png', 
    'overlay.png', 
    'movie.png', 
    'shoot.png', 
    'focus-lowres.png', 
    'display.png', 
    'prefs.png', 
    'debug.png', 
    'info.png', 
    'mymenu.png', 
    'script.png', 
    'Q-forward.png',
    'Q-back.png',
    'forward.png',
]

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

    os.system("perl mkfont-canon < ico.in > %s -width %d -height %d" % (outfile, Wpadded,Hpadded))

process(icons, "ico.c")
process(icons_lowres, "ico-lowres.c")

os.system("diff -D CONFIG_LOW_RESOLUTION_DISPLAY ico.c ico-lowres.c > ../src/ico.c")
