# remap.py
# Remap any image to Magic Lantern BVRAM palette.
# Useful for converting a bitmap in cropmarks fromat.

# Author: Alex Dumitrache <broscutamaker@gmail.com>
# License: GPL

# Usage:
# remap.py image.bmp Palette.jpg mycrop.bmp

# Dependencies: pylab, PIL, imagemagick.

# Pure white (255,255,255) becomes transparent (0).
# Pure black (0,0,0) becomes semi-transparent black (3).
# Only first 80 colors (the safe ones) are used.

from pylab import *
import os, sys
import Image

try:
    a = sys.argv[3]
except:
    print "Usage:\nremap.py image.jpg Palette.jpg mycrop.bmp"
    raise SystemExit

def read_ch(f):
    M = zeros([16,16])[:,:,1]
    im = imread(f)
    h,w = im.shape
    lw = w/16
    lh = h/16
    for i in range(16):
        for j in range(16):
            ia = i*lh+5
            ib = (i+1)*lh-5
            ja = i*lw+5
            jb = (i+1)*lw-5
            M[i,j] =  mean(im[ia:ib,ja:jb])
    return M

def read_rgb(f):
    M = zeros([16,16,3])
    im = flipud(imread(f))
    h,w,c = im.shape
    assert c==3
    lw = w/16
    lh = h/16
    
    for c in range(3):
        for i in range(16):
            for j in range(16):
                ia = i*lh+5
                ib = (i+1)*lh-5
                ja = j*lw+5
                jb = (j+1)*lw-5
                M[i,j,c] = mean(im[ia:ib,ja:jb,c])
    return M

def RGB2HSL(R, G, B):
    fR = float(R / 255.0)
    fG = float(G / 255.0)
    fB = float(B / 255.0)
    ma = max(fR, fG, fB)
    mi = min(fR, fG, fB)
    if ma == mi:
        H = 0
    elif ma == fR:
        H = (60 * ((fG - fB)/(ma - mi))) % 360
    elif ma == fG:
        H = 120 + 60 * ((fB - fR)/(ma - mi))
    elif ma == fB:
        H = 240 + 60 * ((fR - fG)/(ma - mi))
    L = (ma + mi) / 2
    if ma == mi:
        S = 0
    elif L <= 0.5:
        S = (ma - mi) / (2 * L)
    elif L > 0.5:
        S = (ma - mi) / (2 - 2 * L)
    return H, S, L

def remap_rgb(im,M):
    h,w,c = im.shape
    assert c==3
    newim = zeros((h,w))
    mf = M.flatten().reshape((256,3))
    for i in range(h):
        for j in range(w):
            rgb = im[i,j]
            dif = mf - rgb*ones((256,1))
            dif = (dif**2).sum(1)
            ind = argmin(dif[:80]) # only first 80 entries are safe
            #~ print rgb
            if tuple(rgb) == (255,255,255):
                #~ print "transparent"
                ind = 0
            #~ print tuple(rgb)
            if tuple(rgb) == (0,0,0):
                #~ print "semitransparent"
                ind = 3
            newim[i,j] = ind
        if i % 50 == 0: print >> sys.stderr, "%d%%" % round(i * 100.0 / h)
    return newim

def remap_hsl(im,M):
    h,w,c = im.shape
    assert c==3
    newim = zeros((h,w))
    mf = M.flatten().reshape((256,3))
    for i in range(256):
        mf[i,:] = RGB2HSL( *mf[i,:])
    l = mf[:,2]
    #~ print l.shape
    wh = (sin(l*pi)**10)/10
    ws = (sin(l*pi)**10)/50
    for i in range(h):
        for j in range(w):
            rgb = im[i,j]
            hsl = RGB2HSL(*rgb)
            dif = mf - hsl*ones((256,1))
            #~ dh = dif[:,0]
            #~ dh = minimum(abs(dh),abs(dh-360))
            #~ dif[:,0] = dh
            dif[:,0] *= wh
            dif[:,1] *= ws
            dif = (dif**2).sum(1)
            ind = argmin(dif)
            newim[i,j] = ind
        print >> sys.stderr, i
    return newim

def save_img(im,M,file):
    ix = Image.fromarray(flipud(im.astype(uint8)))
    mf = M.flatten().reshape((256,3))
    ix.putpalette(mf.flatten().astype(uint8))
    ix.save(file)

os.system("convert %s -type TrueColor -quality 100 tmp.png" % sys.argv[1])
os.system("convert %s -type TrueColor -quality 100 pal.jpg" % sys.argv[2])
M = read_rgb("pal.jpg")
for i in range(3):
    print M[:,:,i].astype(uint8)
im = flipud(imread("tmp.png"))
if im.max() <= 1: im *= 255
print im.min(), im.max()
imr = remap_rgb(im,M)
#print imr
#save_img(imr,M,"pal.bmp")
save_img(imr[0:480,0:720],M,sys.argv[3])
