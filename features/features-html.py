#!/usr/bin/python
# make a table with what features are enabled on what camera
import os, sys, string, re
import commands
from mako.template import Template

class Bunch(dict):
    def __init__(self, d):
        dict.__init__(self, d)
        self.__dict__.update(d)

def to_bunch(d):
    r = {}
    for k, v in d.items():
        if isinstance(v, dict):
            v = to_bunch(v)
        r[k] = v
    return Bunch(r)


def run(cmd):
    return commands.getstatusoutput(cmd)[1]

cams = []

for c in os.listdir("../platform"):
    if os.path.isdir(os.path.join("../platform", c)):
        if "_" not in c and "all" not in c and "MASTER" not in c:
            cams.append(c)

cams = sorted(cams)
#~ print len(cams), cams

FD = {}
AF = []

for c in cams:
    cmd = "cpp -I../platform/%s -I../src ../src/config-defines.h -dM | grep FEATURE" % c
    F = run(cmd)
    for f in F.split('\n'):
        f = f.replace("#define", "").strip()
        #print c,f
        FD[c,f] = True
        AF.append(f)

AF = list(set(AF))
AF.sort()

def cam_shortname(c):
    c = c.split(".")[0]
    return c

def cam_longname(cam):
    for c in cams:
        if c.startswith(cam):
            return c

#print "%30s" % "",
#for c in cams:
#    print "%3s" % cam_shortname(c),
#print ""

#for f in AF:
#    print "%30s" % f[8:38],
#    for c in cams:
#        if FD.get((c,f)):
#            print "  *  ",
#        else:
#            print "     ",
#    print ""

shortnames = {}
for c in cams:
    shortnames[c]=cam_shortname(c)


# let's see in which menu we have these features
menus = []
menus.append("Modules")
current_menu = "Other"
MN_DICT = {}
MN_COUNT = {}
af = open("../src/all_features.h").read()
for l in af.split("\n"):
    m = re.match("/\*\* ([a-zA-Z]+) menu \*\*/", l)
    if m:
        current_menu = m.groups()[0]
        menus.append(current_menu)
        continue
    m = re.search("FEATURE_([A-Z0-9_]+)", l)
    if m:
        f = m.groups()[0]
        MN_DICT[f] = current_menu

for f in AF:
    mn = MN_DICT.get(f[8:], "Other")
    MN_COUNT[mn] = MN_COUNT.get(mn,0) + 1

menus.append("Other")

porting_threads = {
    '1100D': 'http://www.magiclantern.fm/forum/index.php?topic=1009.0',
    '5DC': 'http://www.magiclantern.fm/forum/index.php?topic=1010.0',
    '650D': 'http://www.magiclantern.fm/forum/index.php?topic=7473.0',
    '6D': 'http://www.magiclantern.fm/forum/index.php?topic=3904.0',
    '7D': 'http://www.magiclantern.fm/forum/index.php?topic=3974.0',
    '5D3': 'http://www.magiclantern.fm/forum/index.php?topic=2602.0',
    '40D': 'http://www.magiclantern.fm/forum/index.php?topic=1452.0',
    'EOSM': 'http://www.magiclantern.fm/forum/index.php?topic=3648.0',
    '500D': 'http://www.magiclantern.fm/forum/index.php?topic=2317.0',
    '100D': 'http://www.magiclantern.fm/forum/index.php?topic=5529.0',
    '700D': 'http://www.magiclantern.fm/forum/index.php?topic=5951.0',
}

feature_links = {
    'MODULE__raw_rec'   : 'http://www.magiclantern.fm/forum/index.php?board=49.0',
    'MODULE__ettr'      : 'http://www.magiclantern.fm/forum/index.php?topic=5693.0',
    'MODULE__autoexpo'  : 'http://www.magiclantern.fm/forum/index.php?topic=7208.0',
    'MODULE__file_man'  : 'http://www.magiclantern.fm/forum/index.php?topic=5522.0',
    'MODULE__ime_base'  : 'http://www.magiclantern.fm/forum/index.php?topic=6899.0',
    'MODULE__dual_iso'  : 'http://www.magiclantern.fm/forum/index.php?topic=7139.0',
    'MODULE__bolt_rec'  : 'http://www.magiclantern.fm/forum/index.php?topic=6303.0',
    'FEATURE_ANALOG_GAIN'  :  'http://wiki.magiclantern.fm/userguide#analog_gain_db',
    'FEATURE_AUDIO_METERS'  :  'http://wiki.magiclantern.fm/userguide#audio_meters',
    'FEATURE_DIGITAL_GAIN'  :  'http://wiki.magiclantern.fm/userguide#l-digitalgain_and_r-digitalgain_db',
    'FEATURE_HEADPHONE_MONITORING'  :  'http://wiki.magiclantern.fm/userguide#headphone_monitoring',
    'FEATURE_HEADPHONE_OUTPUT_VOLUME'  :  'http://wiki.magiclantern.fm/userguide#output_volume_db',
    'FEATURE_INPUT_SOURCE'  :  'http://wiki.magiclantern.fm/userguide#input_source',
    'FEATURE_MIC_POWER'  :  'http://wiki.magiclantern.fm/userguide#mic_power',
    'FEATURE_WIND_FILTER'  :  'http://wiki.magiclantern.fm/userguide#wind_filter',
    'FEATURE_POST_DEFLICKER' : 'http://www.magiclantern.fm/forum/index.php?topic=5705',
    'FEATURE_LV_DISPLAY_PRESETS': 'http://www.magiclantern.fm/forum/index.php?topic=1729.0',
    'FEATURE_IMAGE_EFFECTS': 'http://www.magiclantern.fm/forum/index.php?topic=2120.0',
    'FEATURE_FLEXINFO': 'http://www.magiclantern.fm/forum/index.php?topic=4157.0',
    'FEATURE_AFMA_TUNING': 'http://www.magiclantern.fm/forum/index.php?topic=4648.0',
    'FEATURE_RAW_HISTOGRAM': 'http://www.magiclantern.fm/forum/index.php?topic=5149.msg31247#msg31247',
    'FEATURE_FPS_RAMPING': 'http://www.magiclantern.fm/forum/index.php?topic=2963.0',
    'FEATURE_VIDEO_HACKS': 'http://www.magiclantern.fm/forum/index.php?topic=3404.0',
    'FEATURE_VIGNETTING_CORRECTION': 'http://www.magiclantern.fm/forum/index.php?topic=4598',
}

# modules

mn = current_menu = "Modules"
modules = os.listdir("../modules/")
modules = [m for m in modules if os.path.isdir(os.path.join("../modules/", m))]
modules.sort()

# only show whitelisted modules, at least for now
modules = [m for m in modules if "MODULE__" + m in feature_links or m in "pic_view"]

# called only for modules that load
def module_get_status(m, cam):
    c = cam_shortname(cam)

    # this loads everywhere, but only works on 5D3 and 7D
    if m == "dual_iso":
        return c in ["5D3", "7D"]
        
    # no idea, assume it's OK if it loads
    return True

def module_check_cams(m):
    out = run("cd ../modules/ && python checkdep.py " + m)
    lines = out.split("\n")
    cameras = []
    for i,l in enumerate(lines):
        if l.strip() == "Will load on:":
            next_line = lines[i+1]
            cams = next_line.split(",")
            cams = [cam_longname(c.strip()) for c in cams if len(c.strip())]
            cams = [(c, module_get_status(m, c)) for c in cams]
            cameras += cams
        if l.strip().startswith("Not checked"):
            next_line = lines[i+1]
            cams = next_line.split(",")
            cams = [cam_longname(c.strip()) for c in cams if len(c.strip())]
            cams = [(c, "?") for c in cams]
            cameras += cams
    return cameras


for m in modules:
    f = "MODULE__" + m
    MN_DICT[m] = current_menu
    MN_COUNT[mn] = MN_COUNT.get(mn,0) + 1
    ok_cams = module_check_cams(m)
    for c,s in ok_cams:
        if s:
            FD[c,f] = s
    AF.append(f)

version = run("LC_TIME=EN date +'%Y%b%d' && hg id")

data = {'FD':FD, 'AF':AF, 'cams':cams, 'shortnames':shortnames, 'menus':menus, 'MN_COUNT': MN_COUNT, 'MN_DICT': MN_DICT,
        'porting_threads': porting_threads, 'feature_links': feature_links, 'version': version}
mytemplate = Template(filename='features.tmpl')
print mytemplate.render(**data)

