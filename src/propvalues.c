/** \file
 * Common property values
 */

#include "dryos.h"
#include "bmp.h"

#define _DONT_INCLUDE_PROPVALUES_
#include "property.h"
#include "shoot.h"
#include "zebra.h"
#include <platform/state-object.h>

char __camera_model_short[8] = CAMERA_MODEL;
char camera_model[32];
uint32_t camera_model_id = 0;
char firmware_version[32];
char camera_serial[32];

/* is_camera("5D3", "1.2.3") - will check for a specific camera / firmware version */
/* is_camera("5D3", "*") - will accept all firmware versions */
/* is_camera("DIGIC", "5") - will accept all DIGIC 5 models */
/* todo: possibly other classifications? */
int is_camera(const char * model, const char * version)
{
    if (streq(model, "DIGIC"))
    {
        if (streq(version, "*"))
        {
            /* only DIGIC models supported */
            return 1;
        }

        if (strlen(version) != 1)
        {
            /* only one-digit DIGIC version check is currently supported, i.e. no 4+ or similar */
            return 0;
        }

        #if   defined(CONFIG_DIGIC_II)
        return version[0] == '2';
        #elif defined(CONFIG_DIGIC_III)
        return version[0] == '3';
        #elif defined(CONFIG_DIGIC_IV)
        return version[0] == '4';
        #elif defined(CONFIG_DIGIC_V)
        return version[0] == '5';
        #elif defined(CONFIG_DIGIC_VI)
        return version[0] == '6';
        #elif defined(CONFIG_DIGIC_VII)
        return version[0] == '7';
        #elif defined(CONFIG_DIGIC_VIII)
        return version[0] == '8';
        #else
        #error FIXME: no CONFIG_DIGIC_version defined.
        #endif
    }

    return 
        streq(__camera_model_short, model) &&                         /* check camera model */
        (streq(firmware_version, version) || streq(version, "*"));    /* check firmware version */
}

PROP_HANDLER(PROP_CAM_MODEL)
{
    memcpy((char *)&camera_model_id, (void*)buf + 32, 4);
    snprintf(camera_model, sizeof(camera_model), (const char *)buf);
}

PROP_HANDLER(PROP_BODY_ID)
{
    /* different camera serial lengths */
    if(len == 8)
    {
        snprintf(camera_serial, sizeof(camera_serial), "%X%08X", (uint32_t)(*((uint64_t*)buf) & 0xFFFFFFFF), (uint32_t) (*((uint64_t*)buf) >> 32));
    }
    else if(len == 4)
    {
        snprintf(camera_serial, sizeof(camera_serial), "%08X", *((uint32_t*)buf));
    }
    else
    {
        snprintf(camera_serial, sizeof(camera_serial), "(unknown len %d)", len);
    }
}

PROP_HANDLER(PROP_FIRMWARE_VER)
{
    snprintf(firmware_version, sizeof(firmware_version), (const char *)buf);
}

volatile PROP_INT(PROP_LV_DISPSIZE, lv_dispsize);
volatile PROP_INT(PROP_LIVE_VIEW_VIEWTYPE, _expsim);
volatile PROP_INT(PROP_EFIC_TEMP, efic_temp);
volatile PROP_INT(PROP_GUI_STATE, gui_state);
#ifdef CONFIG_DIGIC_678
// confirmed 750D, 200D, R
volatile PROP_INT(PROP_PIC_QUALITY2, pic_quality);
#else
volatile PROP_INT(PROP_PIC_QUALITY, pic_quality);
#endif
volatile PROP_INT(PROP_AVAIL_SHOT, avail_shot);
#if defined(CONFIG_DIGIC_VIII)
/* R uses PROP_LVAF_MODE. However code suggests that both may be used
 * on DSLRs. When both are enabled, there's a race, so this is left to
 * be tested on 250D/850D/etc. */
volatile PROP_INT(PROP_LVAF_MODE, af_mode);
#else
volatile PROP_INT(PROP_AF_MODE, af_mode);
#endif
volatile PROP_INT(PROP_METERING_MODE, metering_mode);
volatile PROP_INT(PROP_DRIVE, drive_mode);
volatile PROP_INT(PROP_STROBO_FIRING, strobo_firing);
volatile PROP_INT(PROP_IMAGE_REVIEW_TIME, image_review_time);
volatile PROP_INT(PROP_MIRROR_DOWN, mirror_down);
volatile PROP_INT(PROP_LCD_BRIGHTNESS, backlight_level);
volatile PROP_INT(PROP_LV_MOVIE_SELECT, lv_movie_select);
volatile PROP_INT(PROP_ACTIVE_SWEEP_STATUS, sensor_cleaning);
volatile PROP_INT(PROP_BURST_COUNT, burst_count);
volatile PROP_INT(PROP_BATTERY_POWER, battery_level_bars);
volatile PROP_INT(PROP_MOVIE_SOUND_RECORD, sound_recording_mode);
volatile PROP_INT(PROP_DATE_FORMAT, date_format);
volatile PROP_INT(PROP_AUTO_POWEROFF_TIME, auto_power_off_time);
volatile PROP_INT(PROP_VIDEO_SYSTEM, video_system_pal);
volatile PROP_INT(PROP_LV_FOCUS_STATUS, lv_focus_status);
volatile PROP_INT(PROP_ICU_UILOCK, icu_uilock);

#ifdef CONFIG_NO_DEDICATED_MOVIE_MODE
int ae_mode_movie = 1;
#else
volatile PROP_INT(PROP_AE_MODE_MOVIE, ae_mode_movie);
#endif

volatile int shooting_mode;
volatile PROP_INT(PROP_SHOOTING_MODE, shooting_mode_custom);

PROP_HANDLER(PROP_SHOOTING_MODE_2)
{
    shooting_mode = buf[0];

    #ifdef CONFIG_NO_DEDICATED_MOVIE_MODE
    ae_mode_movie = shooting_mode == SHOOTMODE_M;
    #endif
}

volatile int dofpreview;
PROP_HANDLER(PROP_DOF_PREVIEW_MAYBE) // len=2
{
    dofpreview = buf[0] & 0xFFFF;
}

volatile int lv = 0;
volatile int lv_paused = 0; // not a property, but related

int get_expsim();

bool FAST is_movie_mode()
{
    #ifdef CONFIG_NO_DEDICATED_MOVIE_MODE
    return 
            #if defined(CONFIG_5D2) || defined(CONFIG_50D) /* the switch is in the menus */
            lv && 
            #endif
            lv_movie_select == LVMS_ENABLE_MOVIE /* the switch is on the camera body, so you can't be in photo mode when it's enabled */
            #ifdef CONFIG_5D2
            && get_expsim() == 2  // movie enabled, but photo display is considered photo mode
            #endif
        ;
    #else
    return shooting_mode == SHOOTMODE_MOVIE;
    #endif
}

volatile int shutter_count = 0;
volatile int shutter_count_plus_lv_actuations = 0;
volatile int total_shots_count = 0;
#ifdef CONFIG_DIGIC_VIII
//kitor: Confirmed R, RP, M50. Not checked on DSLRs. See comment in property.h
PROP_HANDLER(PROP_MECHA_COUNTER)
{
    shutter_count = buf[0];
    //I don't have access to any DSLR to find if it's stored anywhere.
    //Maybe old property holds for SLRs and new ones are just for MILCs?

#ifdef CONFIG_EOSRP
    // coon: Firmware of RP has a bug which leads into a very huge shutter count number.
    // Regarding to a user of the forum who read out the shutter count on a fresh RP
    // with only one photo taken, the offset seems to be 1086947309 so this value needs
    // to be subtracted to get the real shutter count:

    shutter_count -= 1086947309;
#endif

    shutter_count_plus_lv_actuations = shutter_count;
}
//kitor: New on D8, total counter including silent shoots.
PROP_HANDLER(PROP_RELEASE_COUNTER)
{
    total_shots_count = buf[0];
}
#else
PROP_HANDLER(PROP_SHUTTER_COUNTER)
{
    shutter_count = buf[0];
    shutter_count_plus_lv_actuations = buf[1];
}
#endif

volatile int display_sensor = 0;

PROP_HANDLER(PROP_DISPSENSOR_CTRL)
{
    display_sensor = !buf[0];
}

volatile int video_mode_crop = 0;
volatile int video_mode_fps = 0;
volatile int video_mode_resolution = 0; // 0 if full hd, 1 if 720p, 2 if 480p
PROP_HANDLER(PROP_VIDEO_MODE)
{
    #ifdef CONFIG_500D
    video_mode_resolution = buf[0];
    video_mode_fps = buf[1];
    #else
    video_mode_crop = buf[0];
    video_mode_resolution = buf[1];
    video_mode_fps = buf[2];
    #endif
}

#ifdef CONFIG_LIVEVIEW
PROP_HANDLER( PROP_LV_ACTION )
{
#ifdef CONFIG_750D // TODO: check D78
    lv = !(uint32_t*)buf[0]; // now prop holds pointer to value
#else
    lv = !buf[0];
#endif
}
#endif

/* special case for dual monitor support */
/* external display with mirroring enabled, from ML's standpoint, is identical to built-in LCD */
/* therefore, if hdmi_mirroring is true, the rest of ML code will believe there's no external monitor connected */

static int ext_monitor_hdmi_raw;
static int hdmi_code_raw;
static int hdmi_mirroring;

volatile int ext_monitor_hdmi;
volatile int hdmi_code;

volatile PROP_INT(PROP_USBRCA_MONITOR, _ext_monitor_rca);

static void hdmi_vars_update()
{
    if (!hdmi_mirroring)
    {
        /* regular external monitor */
        ext_monitor_hdmi = ext_monitor_hdmi_raw;
        hdmi_code = hdmi_code_raw;
    }
    else
    {
        /* external monitor with mirroring, overlays are on built-in LCD */
        ext_monitor_hdmi = hdmi_code = 0;
    }
}

PROP_HANDLER(PROP_HDMI_CHANGE)
{
    ext_monitor_hdmi_raw = buf[0];
    hdmi_vars_update();
}

PROP_HANDLER(PROP_HDMI_CHANGE_CODE)
{
    hdmi_code_raw = buf[0];
    hdmi_vars_update();
}

#ifdef CONFIG_50D
int __recording = 0;
int shooting_type = 0;
PROP_HANDLER(PROP_SHOOTING_TYPE)
{
    shooting_type = buf[0];
    __recording = (shooting_type == 4 ? 2 : 0);
}

PROP_HANDLER(PROP_MOVIE_SIZE_50D)
{
    video_mode_resolution = buf[0];
    video_mode_fps = 30;
}
#else
volatile PROP_INT(PROP_MVR_REC_START, __recording);
volatile PROP_INT(PROP_SHOOTING_TYPE, shooting_type);
#endif
int __recording_custom = 0;

void set_recording_custom(int state)
{
    __recording_custom = state;
}

int lv_disp_mode;

#ifndef LV_OVERLAYS_MODE
/*
 * For models with LV_OVERLAYS_MODE defined we update in zebra.c instead.
 * See zebra.c `get_global_draw()` for more details.
 */
PROP_HANDLER(PROP_LV_OUTPUT_TYPE)
{
    #if defined(CONFIG_5D3)
    /* 1 when Canon overlays are present on the built-in LCD, 0 when they are not present (so we can display our overlays) */
    /* 2 on external monitor with mirroring enabled; however, you can't tell when Canon overlays are present (FIXME) */
    /* todo: check whether this snippet is portable */
    lv_disp_mode = (uint8_t)buf[1] & 1;
    hdmi_mirroring = buf[1] & 2;
    hdmi_vars_update();
    #elif defined(EVF_STATE) || defined(CONFIG_50D)
    lv_disp_mode = (uint8_t)buf[1];
    #else
    lv_disp_mode = (uint8_t)buf[0];
    #endif
}
#endif

#if defined(CONFIG_NO_AUTO_ISO_LIMITS)
int auto_iso_range = 0x4868; // no auto ISO in Canon menus; considering it fixed 100-1600.
#else
volatile PROP_INT(PROP_AUTO_ISO_RANGE, auto_iso_range);
#endif

char artist_name[64]="                                                               ";
PROP_HANDLER( PROP_ARTIST_STRING )
{
    if( len > sizeof(artist_name) ) len = sizeof(artist_name);
    memcpy( artist_name, buf, len );
}

char copyright_info[64]="                                                               ";
PROP_HANDLER( PROP_COPYRIGHT_STRING )
{
    if( len > sizeof(copyright_info) ) len = sizeof(copyright_info);
    memcpy( copyright_info, buf, len );
}

char* get_video_mode_name(int include_fps)
{
    static char zoom_msg[12];
    snprintf(zoom_msg, sizeof(zoom_msg), "ZOOM-X%d", lv_dispsize);
    
    char* video_mode = 
        is_pure_play_photo_mode()                   ? "PLAY-PH"  :      /* Playback, reviewing a picture */
        is_pure_play_movie_mode()                   ? "PLAY-MV"  :      /* Playback, reviewing a video */
        is_play_mode()                              ? "PLAY-UNK" :
        lv && lv_dispsize!=1                        ? zoom_msg   :      /* Some zoom in LiveView */
        LV_PAUSED                                   ? "PAUSED-LV":      /* Paused LiveView (similar to playback, used by ML for powersaving, FRSP, various experiments...) */
        lv && lv_dispsize==1 && !is_movie_mode()    ? "PH-LV"    :      /* Photo LiveView */
        !is_movie_mode() && QR_MODE                 ? "PH-QR"    :      /* Photo QuickReview (right after taking a picture) */
        !is_movie_mode() && display_idle()          ? "PH-NOLV"  :      /* Regular photo mode outside LV (ready to take a picture) */
        !is_movie_mode()                            ? "PH-UNK"   :
        video_mode_resolution == 0 && !video_mode_crop && !RECORDING_H264 ? "MV-1080"  :    /* Movie 1080p, standby */
        video_mode_resolution == 1 && !video_mode_crop && !RECORDING_H264 ? "MV-720"   :    /* Movie 720p, standby */
        video_mode_resolution == 2 && !video_mode_crop && !RECORDING_H264 ? "MV-480"   :    /* Movie 480p, standby */
        video_mode_resolution == 0 &&  video_mode_crop && !RECORDING_H264 ? "MVC-1080" :    /* Movie 1080p crop (3x zoom as with 600D), standby */
        video_mode_resolution == 2 &&  video_mode_crop && !RECORDING_H264 ? "MVC-480"  :    /* Movie 480p crop (as with 550D), standby */
        video_mode_resolution == 0 && !video_mode_crop &&  RECORDING_H264 ? "REC-1080" :    /* Movie 1080p, recording */
        video_mode_resolution == 1 && !video_mode_crop &&  RECORDING_H264 ? "REC-720"  :    /* Movie 720p, recording */
        video_mode_resolution == 2 && !video_mode_crop &&  RECORDING_H264 ? "REC-480"  :    /* Movie 480p, recording */
        video_mode_resolution == 0 &&  video_mode_crop &&  RECORDING_H264 ? "RECC1080" :    /* Movie 1080p crop, recording */
        video_mode_resolution == 2 &&  video_mode_crop &&  RECORDING_H264 ? "RECC-480" :    /* Movie 480p crop, recording */
        "MV-UNK";
    
    return video_mode;
}

char* get_display_device_name()
{
    char* display_device = 
        !EXT_MONITOR_CONNECTED                          ? "LCD"      :          /* Built-in LCD */
        ext_monitor_hdmi && hdmi_code == 20             ? "HDMI-MIR" :          /* HDMI with mirroring enabled (5D3 1.2.3) */
        ext_monitor_hdmi && hdmi_code == 5              ? "HDMI1080" :          /* HDMI 1080p (high resolution) */
        ext_monitor_hdmi && hdmi_code == 2              ? "HDMI480 " :          /* HDMI 480p aka HDMI-VGA (use Force HDMI-VGA from ML menu, Display->Advanced; most cameras drop to this mode while recording); */
        _ext_monitor_rca && video_system_pal            ? "SD-PAL"   :          /* SD monitor (RCA cable), PAL selected in Canon menu */
        _ext_monitor_rca && !video_system_pal           ? "SD-NTSC"  : "UNK";   /* SD monitor (RCA cable), NTSC selected in Canon menu */
    
    return display_device;
}
