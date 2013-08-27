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
    'FEATURE_BEEP'  :  'http://www.magiclantern.fm/forum/index.php?topic=3820.msg20553#msg20553',
    'FEATURE_DIGITAL_GAIN'  :  'http://wiki.magiclantern.fm/userguide#l-digitalgain_and_r-digitalgain_db',
    'FEATURE_HEADPHONE_MONITORING'  :  'http://wiki.magiclantern.fm/userguide#headphone_monitoring',
    'FEATURE_HEADPHONE_OUTPUT_VOLUME'  :  'http://wiki.magiclantern.fm/userguide#output_volume_db',
    'FEATURE_INPUT_SOURCE'  :  'http://wiki.magiclantern.fm/userguide#input_source',
    'FEATURE_MIC_POWER'  :  'http://wiki.magiclantern.fm/userguide#mic_power',
    'FEATURE_VOICE_TAGS'  :  'http://www.magiclantern.fm/forum/index.php?topic=3820.msg20553#msg20553',
    'FEATURE_WIND_FILTER'  :  'http://wiki.magiclantern.fm/userguide#wind_filter',
    'FEATURE_EXPO_APERTURE'  :  'http://wiki.magiclantern.fm/userguide#aperture',
    'FEATURE_EXPO_ISO'  :  'http://wiki.magiclantern.fm/userguide#iso',
    'FEATURE_EXPO_LOCK'  :  'http://www.magiclantern.fm/forum/index.php?topic=3820.msg20554#msg20554',
    'FEATURE_EXPO_OVERRIDE'  :  'http://wiki.magiclantern.fm/userguide#expoverride',
    'FEATURE_EXPO_PRESET'  :  'http://www.magiclantern.fm/forum/index.php?topic=3820.msg20554#msg20554',
    'FEATURE_EXPO_SHUTTER'  :  'http://wiki.magiclantern.fm/userguide#shutter',
    'FEATURE_PICSTYLE'  :  'http://wiki.magiclantern.fm/userguide#picturestyle',
    'FEATURE_REC_PICSTYLE'  :  'http://wiki.magiclantern.fm/userguide#rec_picstyle',
    'FEATURE_WHITE_BALANCE'  :  'http://wiki.magiclantern.fm/userguide#whitebalance',
    'FEATURE_CROPMARKS'  :  'http://wiki.magiclantern.fm/userguide#cropmarks',
    'FEATURE_FALSE_COLOR'  :  'http://wiki.magiclantern.fm/userguide#false_color',
    'FEATURE_FOCUS_PEAK'  :  'http://wiki.magiclantern.fm/userguide#focus_peak',
    'FEATURE_GHOST_IMAGE'  :  'http://wiki.magiclantern.fm/userguide#ghost_image',
    'FEATURE_GLOBAL_DRAW'  :  'http://wiki.magiclantern.fm/userguide#global_draw',
    'FEATURE_HISTOGRAM'  :  'http://wiki.magiclantern.fm/userguide#histogram_and_waveform',
    'FEATURE_MAGIC_ZOOM'  :  'http://wiki.magiclantern.fm/userguide#magic_zoom',
    'FEATURE_RAW_HISTOGRAM'  :  'http://www.magiclantern.fm/forum/index.php?topic=5149.msg31247#msg31247',
    'FEATURE_SPOTMETER'  :  'http://wiki.magiclantern.fm/userguide#spotmeter',
    'FEATURE_VECTORSCOPE'  :  'http://wiki.magiclantern.fm/userguide#vectorscope',
    'FEATURE_WAVEFORM'  :  'http://wiki.magiclantern.fm/userguide#histogram_and_waveform',
    'FEATURE_ZEBRA'  :  'http://wiki.magiclantern.fm/userguide#zebras',
    'FEATURE_FORCE_LIVEVIEW'  :  'http://wiki.magiclantern.fm/userguide#force_liveview',
    'FEATURE_FPS_OVERRIDE'  :  'http://wiki.magiclantern.fm/userguide#fps_override',
    'FEATURE_FPS_RAMPING'  :  'http://www.magiclantern.fm/forum/index.php?topic=2963.0',
    'FEATURE_GRADUAL_EXPOSURE'  :  'http://www.magiclantern.fm/forum/index.php?topic=3820.msg20558#msg20558',
    'FEATURE_HDR_VIDEO'  :  'http://wiki.magiclantern.fm/userguide#hdr_video',
    'FEATURE_IMAGE_EFFECTS'  :  'http://www.magiclantern.fm/forum/index.php?topic=2120.0',
    'FEATURE_MOVIE_LOGGING'  :  'http://wiki.magiclantern.fm/userguide#movie_logging',
    'FEATURE_MOVIE_RECORDING_50D'  :  'http://wiki.magiclantern.fm/userguide#movie_record_50d',
    'FEATURE_MOVIE_REC_KEY'  :  'http://wiki.magiclantern.fm/userguide#movie_rec_key',
    'FEATURE_MOVIE_RESTART'  :  'http://wiki.magiclantern.fm/userguide#movie_restart',
    'FEATURE_REC_INDICATOR'  :  'http://wiki.magiclantern.fm/userguide#time_indicator',
    'FEATURE_REC_NOTIFY'  :  'http://wiki.magiclantern.fm/userguide#rec_stby_notify',
    'FEATURE_SHUTTER_LOCK'  :  'http://wiki.magiclantern.fm/userguide#shutter_lock',
    'FEATURE_VIDEO_HACKS'  :  'http://www.magiclantern.fm/forum/index.php?topic=3404.0',
    'FEATURE_VIGNETTING_CORRECTION'  :  'http://www.magiclantern.fm/forum/index.php?topic=4598',
    'FEATURE_AUDIO_REMOTE_SHOT'  :  'http://wiki.magiclantern.fm/userguide#audio_remoteshot',
    'FEATURE_BULB_RAMPING'  :  'http://wiki.magiclantern.fm/userguide#bulb_focus_ramping',
    'FEATURE_BULB_TIMER'  :  'http://wiki.magiclantern.fm/userguide#bulb_timer',
    'FEATURE_FLASH_TWEAKS'  :  'http://wiki.magiclantern.fm/userguide#flash_tweaks',
    'FEATURE_HDR_BRACKETING'  :  'http://wiki.magiclantern.fm/userguide#hdr_bracketing',
    'FEATURE_INTERVALOMETER'  :  'http://wiki.magiclantern.fm/userguide#intervalometer',
    'FEATURE_LCD_SENSOR_REMOTE'  :  'http://wiki.magiclantern.fm/userguide#lcdsensor_remote',
    'FEATURE_MLU'  :  'http://wiki.magiclantern.fm/userguide#mirror_lockup',
    'FEATURE_MOTION_DETECT'  :  'http://wiki.magiclantern.fm/userguide#motion_detect',
    'FEATURE_POST_DEFLICKER' : 'http://www.magiclantern.fm/forum/index.php?topic=5705',
    'FEATURE_SILENT_PIC'  :  'http://wiki.magiclantern.fm/userguide#silent_pictures',
    'FEATURE_AFMA_TUNING'  :  'http://www.magiclantern.fm/forum/index.php?topic=4648.0',
    'FEATURE_FOCUS_STACKING'  :  'http://wiki.magiclantern.fm/userguide#stack_focus',
    'FEATURE_FOLLOW_FOCUS'  :  'http://wiki.magiclantern.fm/userguide#follow_focus',
    'FEATURE_RACK_FOCUS'  :  'http://wiki.magiclantern.fm/userguide#rack_focus',
    'FEATURE_TRAP_FOCUS'  :  'http://wiki.magiclantern.fm/userguide#trap_focus',
    'FEATURE_CLEAR_OVERLAYS'  :  'http://wiki.magiclantern.fm/userguide#clear_overlays',
    'FEATURE_COLOR_SCHEME'  :  'http://wiki.magiclantern.fm/userguide#color_scheme',
    'FEATURE_FORCE_HDMI_VGA'  :  'http://wiki.magiclantern.fm/userguide#force_hdmi-vga',
    'FEATURE_LEVEL_INDICATOR'  :  'http://wiki.magiclantern.fm/userguide#level_indicator_60d',
    'FEATURE_LV_BRIGHTNESS_CONTRAST'  :  'http://wiki.magiclantern.fm/userguide#lv_contrast',
    'FEATURE_LV_DISPLAY_GAIN'  :  'http://wiki.magiclantern.fm/userguide#lv_display_gain',
    'FEATURE_LV_SATURATION'  :  'http://wiki.magiclantern.fm/userguide#lv_saturation',
    'FEATURE_SCREEN_LAYOUT'  :  'http://wiki.magiclantern.fm/userguide#screen_layout_settings',
    'FEATURE_ARROW_SHORTCUTS'  :  'http://wiki.magiclantern.fm/userguide#arrow_set_shortcuts',
    'FEATURE_AUTO_BURST_PICQ'  :  'http://wiki.magiclantern.fm/userguide#auto_burstpicquality',
    'FEATURE_FLEXINFO'  :  'http://www.magiclantern.fm/forum/index.php?topic=4157.0',
    'FEATURE_IMAGE_REVIEW_PLAY'  :  'http://wiki.magiclantern.fm/userguide#image_review_settings',
    'FEATURE_LV_DISPLAY_PRESETS'  :  'http://www.magiclantern.fm/forum/index.php?topic=1729.0',
    'FEATURE_POWERSAVE_LIVEVIEW'  :  'http://wiki.magiclantern.fm/userguide#powersave_in_liveview',
    'FEATURE_QUICK_ERASE'  :  'http://wiki.magiclantern.fm/userguide#image_review_settings',
    'FEATURE_QUICK_ZOOM'  :  'http://wiki.magiclantern.fm/userguide#image_review_settings',
    'FEATURE_STICKY_DOF'  :  'http://wiki.magiclantern.fm/userguide#misc_key_settings',
    'FEATURE_STICKY_HALFSHUTTER'  :  'http://wiki.magiclantern.fm/userguide#misc_key_settings',
    'FEATURE_WARNINGS_FOR_BAD_SETTINGS'  :  'http://www.magiclantern.fm/forum/index.php?topic=3820.msg20562#msg20562',
    'FEATURE_DONT_CLICK_ME'  :  'http://wiki.magiclantern.fm/userguide#don_t_click_me',
    'FEATURE_SCREENSHOT'  :  'http://wiki.magiclantern.fm/userguide#screenshot_10_s',
    'FEATURE_SHOW_CMOS_TEMPERATURE'  :  'http://wiki.magiclantern.fm/userguide#cmos_temperature',
    'FEATURE_SHOW_CPU_USAGE'  :  'http://wiki.magiclantern.fm/userguide#save_cpu_usage_log',
    'FEATURE_SHOW_FREE_MEMORY'  :  'http://wiki.magiclantern.fm/userguide#free_memory',
    'FEATURE_SHOW_IMAGE_BUFFERS_INFO'  :  'http://www.magiclantern.fm/forum/index.php?topic=3820.msg20563#msg20563',
    'FEATURE_SHOW_SHUTTER_COUNT'  :  'http://wiki.magiclantern.fm/userguide#shutter_count',
    'FEATURE_SHOW_TASKS'  :  'http://wiki.magiclantern.fm/userguide#show_tasks',
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

