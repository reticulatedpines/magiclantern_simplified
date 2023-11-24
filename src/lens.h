#ifndef _lens_h_
#define _lens_h_
/** \file
 * Lens and camera control
 *
 * These are Magic Lantern specific structures that control the camera's
 * shutter speed, ISO and lens aperture.  It also records the focal length,
 * distance and other parameters by hooking the different lens related
 * properties.
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "property.h"

// for movie logging
#define MVR_LOG_BUF_SIZE 8192
#define MVR_LOG_APPEND(...) snprintf(mvr_logfile_buffer + strlen(mvr_logfile_buffer), MVR_LOG_BUF_SIZE - strlen(mvr_logfile_buffer) - 2, ## __VA_ARGS__ );

struct lens_info
{
        void *                  token;
        char                    name[ 32 ];
        unsigned                focal_len; // in mm
        unsigned                focus_dist; // in cm
        unsigned                IS; // PROP_LV_LENS_STABILIZE
        unsigned                aperture;
        int                     ae;        // exposure compensation, in 1/8 EV steps, signed
        unsigned                shutter;
        unsigned                iso;
        unsigned                iso_auto;
        unsigned                iso_analog_raw;
        int                     iso_digital_ev;
        unsigned                iso_equiv_raw;
        unsigned                hyperfocal; // in mm
        unsigned                dof_near; // in mm
        unsigned                dof_far; // in mm
        unsigned                job_state; // see PROP_LAST_JOB_STATE

        unsigned                wb_mode;  // see property.h for possible values
        unsigned                kelvin;   // wb temperature; only used when wb_mode = WB_KELVIN
        unsigned                WBGain_R; // only used when wb_mode = WB_CUSTOM
        unsigned                WBGain_G; // only used when wb_mode = WB_CUSTOM
        unsigned                WBGain_B; // only used when wb_mode = WB_CUSTOM
        int8_t                  wbs_gm;
        int8_t                  wbs_ba;

        unsigned                picstyle; // 1 ... 9: std, portrait, landscape, neutral, faithful, monochrome, user 1, user 2, user 3

        // raw exposure values, in 1/8 EV steps
        uint8_t                 raw_aperture;
        uint8_t                 raw_shutter;
        uint8_t                 raw_iso;
        uint8_t                 raw_iso_auto;
        uint8_t                 raw_picstyle;           /* fixme: move it out */
        uint8_t                 raw_aperture_min;
        uint8_t                 raw_aperture_max;
        int                     flash_ae;
        uint16_t                lens_id;
        uint16_t                dof_flags;
        int                     dof_diffraction_blur;   /* fixme: move those near other DOF fields on next API update */
        //~ float                   lens_rotation;
        //~ float                   lens_step;
        int                     focus_pos;              /* fine steps, starts at 0, range is lens-dependent,
                                                         * only updates when motor moves (will lose position during MF) */

        /* those were retrieved from PROP_LENS property */
        uint8_t                 lens_exists;
        uint16_t                lens_focal_min;
        uint16_t                lens_focal_max;
        uint8_t                 lens_extender;
        uint8_t                 lens_capabilities;
        uint32_t                lens_version;
        uint64_t                lens_serial;
};

extern struct lens_info lens_info;

#define DOF_DIFFRACTION_LIMIT_REACHED 1

#if defined(CONFIG_6D) || defined(CONFIG_5D3_123) || defined(CONFIG_100D) || defined(CONFIG_750D) \
    || defined(CONFIG_80D) || defined(CONFIG_7D2) || defined(CONFIG_70D) || defined(CONFIG_5D4)
struct prop_lv_lens
{
        uint32_t                lens_rotation; // Identical Doesn't Change
        uint32_t                lens_step; // Value Matches initial but doesn't move.
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                off_0x1c;
        uint16_t                off_0x20;
        uint8_t                 off_0x22;
        uint16_t                focus_pos; // off_0x23
        uint8_t                 off_0x25;
        uint16_t                off_0x26;
        uint32_t                off_0x28;
        uint16_t                off_0x2c;
        uint8_t                 off_0x2e;
        uint16_t                focal_len;  // off_0x2f
        uint16_t                off_0x31;
        uint16_t                focus_dist; // off_0x33
        uint32_t                off_0x35;
        uint32_t                off_0x39;
        uint8_t                 off_0x3d;
        uint8_t                 off_0x3e;
        uint8_t                 off_0x3f;

} __attribute__((packed));

SIZE_CHECK_STRUCT( prop_lv_lens, 64 );

#elif defined(CONFIG_200D) || defined(CONFIG_77D)
/* Structure looks exactly like 2nd half of struct on 750D (dumped on both cams)
 * focus_pos was not aligned before and was supposed to be uint16_t.
 * But the first byte is gone, so maybe it was a wrong assumption? */
struct prop_lv_lens
{
        uint8_t                 focus_pos; // off_0x23
        uint8_t                 off_0x25;
        uint16_t                off_0x26;
        uint32_t                off_0x28;
        uint16_t                off_0x2c;
        uint8_t                 off_0x2e;
        uint16_t                focal_len;  // off_0x2f
        uint16_t                off_0x31;
        uint16_t                focus_dist; // off_0x33
        uint32_t                off_0x35;
        uint32_t                off_0x39;
        uint8_t                 off_0x3d;
        uint8_t                 off_0x3e;
        uint8_t                 off_0x3f;
} __attribute__((packed));

SIZE_CHECK_STRUCT( prop_lv_lens, 0x1C );

#elif defined(CONFIG_EOSM)
struct prop_lv_lens
{
        uint32_t                lens_rotation;
        uint32_t                lens_step;
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                off_0x1c;
        uint16_t                off_0x20;
        uint16_t                focus_pos;  // off_0x22; guess (not tested)
        uint32_t                off_0x24;
        uint32_t                off_0x28;
        uint16_t                off_0x2c;
        uint16_t                focal_len;  // off_0x2e
        uint16_t                focus_dist; // One FD; off_0x30
        uint16_t                focus_dist2;// off_0x32
        uint16_t                off_0x34;
        uint16_t                off_0x36;
        uint16_t                off_0x38;
        uint16_t                off_0x3a;
        uint8_t                 off_0x3c;
} __attribute__((packed));

SIZE_CHECK_STRUCT( prop_lv_lens, 61 );

#else
struct prop_lv_lens
{
        uint32_t                lens_rotation; // float in little-endian actually
        uint32_t                lens_step; // float in little-endian actually
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                off_0x1c;
        int16_t                 focus_pos;  /* off_0x20; see lens_info.focus_pos */
        uint16_t                off_0x22;
        uint32_t                off_0x24;
        uint32_t                off_0x28;
        uint16_t                focal_len;      // off_0x2c;
        uint16_t                focus_dist;     // off_0x2e;
        uint32_t                off_0x30;
        uint32_t                off_0x34;
        uint16_t                off_0x38;
} __attribute__((packed));

SIZE_CHECK_STRUCT( prop_lv_lens, 58 );

#endif

#if defined(CONFIG_DIGIC_8X)
/* Digic 8 brings new properties:
 * PROP_LENS_STATIC_DATA  = PROP_LENS + PROP_LENS_NAME + ???
 * PROP_LENS_DYNAMIC_DATA = PROP_LV_LENS + ???
 *
 * Those are quite huge, they size depend on camera.
 * So far we have data from following models:
 *             M50   SX740    R     RP    SX70   250D   850D   R5/R6
 * STATIC     0x138  0x178  0x184  0x184  0x180  0x180  0x1C8  0x1C8
 * DYNAMIC    0x84   0x8C   0x90   0x90   0x90   0x8C   0x90   0x94
 * DryOS ICU   P2     P3     P4     P4     P4     P5     P8     P9
 *
 * A lot of PROP_LENS_STATIC_DATA can be decoded via `readid` evshell function.
 * Some can be decoded via ShootingInfoEx (models like SX70 don't have readid...)
 *
 * Structs seems to have `packed` attribute set, thus fields moving left and
 * right between models. For easier debugging I left those paddings filled in.
 */

#if defined(CONFIG_M50) || defined(CONFIG_SX70) || defined(CONFIG_SX740) || defined(CONFIG_R) || defined(CONFIG_RP) || defined (CONFIG_250D) || defined(CONFIG_XF605)
// variants M50, SX740, R + RP, 250D combined
struct prop_lens_static_data
{
        uint8_t                 attached;
        uint8_t                 type;                         // 90 EF, 91 RF
        uint8_t                _unk_01[38];                   // Not referenced in readid
        uint16_t                lens_id;
        uint16_t                lens_id_ext;
        uint16_t                fl_wide;
        uint16_t                fl_tele;
        uint8_t                 lens_serial[5];
        uint8_t                _unk_02[24];                    // Not referenced in readid
        uint8_t                 extender_id[6];
        uint8_t                 lens_firm_ver[3];
        uint8_t                 field_vision;
        uint8_t                 lens_type;
#if defined(CONFIG_R) || defined(CONFIG_RP)
        uint8_t                _pad_01;                        // padding exists on R,RP
#endif // defined(CONFIG_R) || defined(CONFIG_RP)
        uint8_t                 lens_name_len;
        char                    lens_name[73];
        uint8_t                _unk_03;                        // Not referenced in readid
        uint8_t                 mount_size;
        uint8_t                 lens_switch_exists;
        uint8_t                 lens_is_switch_exists;
        uint8_t                 lens_is_funct_exists;
        uint8_t                 af_speed_setting_available;
#if !defined(CONFIG_M50)
        uint8_t                 dafLimitFno;
        uint8_t                 distortionCorrectionInfo;
        uint8_t                 bcfInfo;
        uint8_t                _unk_04;                        // Not referenced in readid
#if !defined(CONFIG_250D) && !defined(CONFIG_SX70)
        uint8_t                _pad_02;                        // 250D, SX70
#endif // !defined(CONFIG_250D) && !defined(CONFIG_SX70)
#endif // !defined(CONFIG_M50)
        uint16_t                zoom_pos_size;
        uint16_t                focus_pos_size;
        uint16_t                fine_focus_size;
#if !defined(CONFIG_M50)
        uint8_t                 av_dlp_lens;
        uint8_t                 av_slow_enable;
#endif // !defined(CONFIG_M50)
        uint8_t                 av_slow_div;
        uint8_t                _unk_05;                        // Not referenced in readid
        uint16_t                av_max_spd;
        uint16_t                av_silent_spd;
        uint16_t                av_min_spd;
#if defined(CONFIG_M50)
        uint8_t                _unk_06[95];                    // Not referenced in readid
#elif defined(CONFIG_250D) || defined(CONFIG_SX70)
        uint8_t                _unk_06[149];                   // Not referenced in readid
#else // R, RP, looks like additional padding vs 250D exists
        uint8_t                _unk_06[151];                   // Not referenced in readid
#endif
        uint8_t                 colorBalance;
        uint8_t                 pza_exists;
        uint8_t                 pza_id[5];
        uint8_t                 pza_firm_ver[3];
        uint8_t                 pza_firmup;
        uint8_t                 dlAdp_count;
        uint8_t                _unk_07[3];                     // Not referenced in readid
        uint32_t                dlAdpl_id;
        uint8_t                 dlAdpl_funcl;
        uint8_t                 dlAdpl_firm_ver[3];
        uint32_t                dlAdpl2_id;
        uint8_t                 dlAdpl2_funcl;
        uint8_t                 dlAdpl2_firm_ver[3];
        uint8_t                 safemode_info;                 // named only on M50
        uint8_t                 demandWarnDispFromLens;        // unnamed on M50
        uint8_t                 demandWarnDispFromAdp;         // unnamed on M50
#if defined(CONFIG_M50)
        uint8_t                _unk_08;                        // Not referenced in readid
#else
        uint8_t                _unk_08[13];                    // Not referenced in readid
#endif
#if defined(CONFIG_XF605)
// FIXME struct is unknown, all preceding fields may be junk.
// Size is known however.  See prop_request_change() usage
// for PROP_LENS_STATIC_DATA.
        uint8_t                _unk_09[0x14c];
#endif
};

    #if defined(CONFIG_SX740)
        #pragma message "FIXME: SX740 prop_lens_static_data is not implemented"
    #endif
    #if defined(CONFIG_M50)
        SIZE_CHECK_STRUCT( prop_lens_static_data, 0x138 );
        //#elif defined(CONFIG_SX740) kitor FIXME: enable
        //SIZE_CHECK_STRUCT( prop_lens_static_data, 0x178 );
    #elif defined(CONFIG_250D) || defined(CONFIG_SX70)
        SIZE_CHECK_STRUCT( prop_lens_static_data, 0x180 );
    #elif defined(CONFIG_XF605)
        SIZE_CHECK_STRUCT( prop_lens_static_data, 0x2d0 );
    #else  // R, RP, SX70
        SIZE_CHECK_STRUCT( prop_lens_static_data, 0x184 );
    #endif // size check M50, R, RP, 250D

#elif defined(CONFIG_850D) || defined(CONFIG_R6) || defined(CONFIG_R5)
/* new struct variant reorders some fields as compared to previous
 * thus making a separate definition */
struct prop_lens_static_data
{
        uint8_t                 attached;
        uint8_t                 type;
        uint8_t                _unk_01[38];                    // Not referenced in readid
        uint16_t                lens_id;
        uint16_t                lens_id_ext;
        uint16_t                fl_wide;
        uint16_t                fl_tele;
        uint8_t                 lens_serial[5];
        uint8_t                _unk_02[24];                    // Not referenced in readid
        uint8_t                 extender_id[6];
        uint8_t                 lens_firm_ver[3];
        uint8_t                 field_vision;
        uint8_t                 lens_type;
        uint8_t                 extenderMountInfo;
        uint8_t                 lens_name_len;
        char                    lens_name[73];
        uint8_t                _unk_03[65];                    // Not referenced in readid
        uint8_t                 mount_size;
        uint8_t                 lens_switch_exists;
        uint8_t                 lens_af_func_exists;
        uint8_t                 lens_mf_funct_exists;
        uint8_t                 lens_is_switch_exists;
        uint8_t                 lens_is_funct_exists;
        uint8_t                 af_speed_setting_available;
        uint8_t                 dafLimitFno;
        uint8_t                 distortionCorrectionInfo;
        uint8_t                 bcfInfo;
        uint8_t                 lens_id_1292;
        uint8_t                 emd_hot_limit;
#if defined(CONFIG_R6) || defined(CONFIG_R5)
        uint8_t                 aberationControl;              // DNE on 850D
        uint8_t                 _pad_01;
#endif // defined(CONFIG_R6) || defined(CONFIG_R5)
        uint16_t                zoom_pos_size;
        uint16_t                focus_pos_size;
        uint16_t                fine_focus_size;
        uint16_t                av_slow_div;
        uint8_t                 av_slow_enable;
        uint8_t                 av_dlp_lens;
        uint16_t                av_dlp_max_spd;
        uint16_t                av_dlp_silent_spd;
        uint16_t                av_dlp_min_spd;
        uint8_t                _unk_04[150];
        uint8_t                 extendMagnificationVal;
        uint8_t                _unk_05;
        uint16_t                ois_shift_max;
#if defined(CONFIG_R6) || defined(CONFIG_R5)
        uint8_t                 colorBalance;                  // DNE on 850D
#endif // defined(CONFIG_R6) || defined(CONFIG_R5)
        uint8_t                 pza_exists;
        uint8_t                 pza_id[5];
        uint8_t                 pza_firm_ver[3];
        uint8_t                 pza_firmup;
        uint8_t                 dlAdp_count;
#if defined(CONFIG_850D)
        uint8_t                _pad_02[3];
#endif //defined(CONFIG_850D)
        uint32_t                dlAdpl_id;
        uint8_t                 dlAdpl_funcl;
        uint8_t                 dlAdpl_firm_ver[3];
        uint32_t                dlAdpl2_id;
        uint8_t                 dlAdpl2_funcl;
        uint8_t                 dlAdpl2_firm_ver[3];
        uint8_t                _unk_06;
        uint8_t                 demandWarnDispFromLens;
        uint8_t                 demandWarnDispFromAdp;
        uint8_t                _unk_07[13];
};

SIZE_CHECK_STRUCT( prop_lens_static_data, 0x1C8 );
#else  // unknown model
#error No PROP_LENS_STATIC_DATA defined for built cam model
#endif // unknown model

/*
 * PROP_LENS_DYNAMIC_DATA structure
 *
 * For PROP_LENS_DYNAMIC_DATA it is a bit hard to track in decomp. Full content
 * is copied a lot between different subsystems.
 *
 * However after a lot of tracking I found function that names a lot of fields.
 * See _prints_dynamic_data(dynamic*, static*).
 * M50.110: e06e6dbc, R180: e02307b8, 250D.100: e06469f8...
 * Function easy to trace with print starting with "AVEF:0x%x"(...)
 *
 * There's also another function, see R180 e0381a64. On param2 it takes
 * "ShootingInfoEx" structure. This struct is a bundle of multiple structs,
 * including lens dynamic data.
 * Scroll to a debug print "ShootingInfoEx:LensDynamicInfo avef:%x"(...)
 * First field of "ShootingInfoEx" used by that print is also first field of
 * prop_lens_dynamic_data embedded into "ShootingInfoEx".
 */
struct prop_lens_dynamic_data {
        uint16_t                AVEF;             // ShootingInfoEx: avef
        uint16_t                AVO;              // ShootingInfoEx: avo
        uint16_t                AVMAX;            // ShootingInfoEx: avmax
#if !defined(CONFIG_M50)
        uint16_t                AVD;              // Not referenced in M50
#if defined(CONFIG_850D) || defined(CONFIG_R6)  || defined(CONFIG_R5)
        uint16_t                NowAvRF;          // Referenced 850D, R6
#endif
        uint16_t                NowAvEF;          // Not referenced in M50. Before 850D named just NowAv
#endif
        uint8_t                _pad_01[22];       // M50, R, RP, 250D, 850D, R6
        uint16_t                jsstep;
        uint8_t                _pad_02[4];        // M50, R, RP, 250D, 850D, R6
        uint16_t                IDC;
#if !defined(CONFIG_M50) && !defined(CONFIG_250D) && !defined(CONFIG_SX740)
        uint8_t                _pad_03[2];        // R, RP, 850D, R6; not on M50, 250D (alignment?)
#endif
        uint16_t                po;
        uint16_t                po05;
        uint16_t                po10;
        uint16_t                po00;
        uint16_t                po15;
        uint16_t                Npo;
        uint16_t                po0;
        uint16_t                po25;
        uint16_t                po50;
        uint16_t                po75;
        uint16_t                po100;            // po100+ not referenced on 850D
        uint16_t                po125;
        uint16_t                po150;
        uint16_t                po175;
        uint16_t                po200;
        uint8_t                 v0;               // vignetting
        uint8_t                 v1;
        uint8_t                 v2;
        uint8_t                 v3;
        uint16_t                FL;
        uint16_t                focal_len_image;  // ShootingInfoEx R6, R
        uint16_t                focus_far;        // ShootingInfoEx: abs_inf; FarAbs
        uint16_t                focus_near;       // ShootingInfoEx: abs_near; NearAbs
        uint16_t                zoomPos;          // ShootingInfoEx: zoom_pos
        uint16_t                focusPos;         //
        uint16_t                fineFocusPos;     // ShootingInfoEx: fine_focus_pos
        uint16_t                HighResoZoomPos;  // ShootingInfoEx: high_res_zoom_pos
        uint16_t                HighResoFocusPos; // ShootingInfoEx: high_res_focus_pos
#if defined(CONFIG_R5) || defined(CONFIG_R6)
        uint8_t                _r6_01[6];         // only on R6, some extra fields?
#endif
#if defined(CONFIG_SX70) || defined(CONFIG_R) || defined(CONFIG_RP) || defined(CONFIG_R5) || defined(CONFIG_R6)
        uint8_t                 abstat;           // lens abberation related; exists only on R series
#endif
        uint8_t                 st1;
        uint8_t                 st2;              // & 0x80 true -> MF, false -> AF
        uint8_t                 st3;              // & 0x0F looks equv to PROP_LV_LENS_STABILIZE
        uint8_t                 st4;
        uint8_t                 st5;
        uint8_t                _pad_04[3];        // M50, R, RP, 250D, 850D, R6
        uint8_t                 FcsSt1;           // FocusStep?
        uint8_t                 FcsSt2;           // FcsSt* are unnamed on M50
        uint8_t                 FcsSt3;
        uint8_t                 FcsSt4;
        uint8_t                 ZmSt1;            // ZoomStep?
        uint8_t                 ZmSt2;            // ZmSt* are unnamed on M50
        uint8_t                 ZmSt3;
        uint8_t                 ZmSt4;
        uint8_t                _pad_05[4];        // M50, R, RP, 250D, 850D, R6
#if defined(CONFIG_SX70) || defined(CONFIG_R) || defined(CONFIG_RP) || defined(CONFIG_R5) || defined(CONFIG_R6)
        uint8_t                _pad_05a;          // R, RP, R6 (alignment?)
#endif
        uint16_t                ts_shift;         // via ShootingInfoEx
        uint8_t                 ts_tilt;          // via ShootingInfoEx
        uint8_t                 ts_all_revo;      // via ShootingInfoEx
        uint8_t                 ts_ts_revo;       // via ShootingInfoEx
        uint8_t                _pad_06[7];        // M50, R, RP, 250D, 850D, R6
#if defined(CONFIG_M50)
        uint8_t                _m50_01[4];        // M50, some extra fields?
#endif
        uint8_t                 LENSEr;           // not mentioned on 850D
#if defined(CONFIG_M50)
        uint8_t                _pad_07[7];        // M50
#elif defined(CONFIG_R5) || defined(CONFIG_R6)
        uint8_t                _pad_07[11];       // R6
#else
        uint8_t                _pad_07[15];       // 850D, 250D, R, RP
#endif
#if defined(CONFIG_XF605)
        uint8_t                _pad_08[0x136]; // 0x1c4 total
#endif
};

#if defined(CONFIG_R5) || defined(CONFIG_R6)
SIZE_CHECK_STRUCT( prop_lens_dynamic_data, 0x94 );
#elif defined(CONFIG_SX70) || defined(CONFIG_R) || defined(CONFIG_RP) || defined(CONFIG_850D)
SIZE_CHECK_STRUCT( prop_lens_dynamic_data, 0x90);
#elif defined(CONFIG_250D) || defined(CONFIG_SX740)
SIZE_CHECK_STRUCT( prop_lens_dynamic_data, 0x8C);
#elif defined(CONFIG_M50)
SIZE_CHECK_STRUCT( prop_lens_dynamic_data, 0x84);
#elif defined(CONFIG_XF605)
// FIXME the dynamic and static structs for XF605 are almost certainly very wrong.
// Couldn't find a nice debug function that dumped the names of the fields.
// This is just to let a build compile and shouldn't be used.  The sizes are
// believed to be correct.
SIZE_CHECK_STRUCT( prop_lens_dynamic_data, 0x1c4); // see e.g. 1ed6ecae on XF605 1.0.1
#else  // unknown model
#error No PROP_LENS_DYNAMIC_DATA defined for built cam model
#endif // /unknown model

#endif // CONFIG_DIGIC_VIII + CONFIG_DIGIC_X

struct prop_focus
{
        uint8_t                 active;         // off_0x00, must be 1
        uint8_t                 step_hi;        // off_0x01
        uint8_t                 step_lo;        // off_0x02
        uint8_t                 mode;           // off_0x03 unknown, usually 7?
        uint8_t                 unk;
} __attribute__((packed));

SIZE_CHECK_STRUCT( prop_focus, 5 );

struct prop_picstyle_settings
{
        int32_t         contrast;   // -4..4
        uint32_t        sharpness;  // 0..7
        int32_t         saturation; // -4..4
        int32_t         color_tone; // -4..4
        uint32_t        off_0x10;   // deadbeaf?!
        uint32_t        off_0x14;   // deadbeaf?!
} __attribute__((aligned,packed));

SIZE_CHECK_STRUCT( prop_picstyle_settings, 0x18 );

void lens_wait_readytotakepic(int wait);

// return true on success
extern int lens_set_rawaperture( int aperture);
extern int lens_set_rawiso( int iso );
extern int lens_set_rawshutter( int shutter );
extern int lens_set_ae( int ae );
extern int lens_set_flash_ae( int ae );
extern void lens_set_drivemode( int dm );
extern void lens_set_wbs_gm(int value);
extern void lens_set_wbs_ba(int value);

extern int expo_override_active();
extern int bv_set_rawshutter(unsigned shutter);
extern int bv_set_rawaperture(unsigned aperture);
extern int bv_set_rawiso(unsigned iso);

/* private, to be refactored */
extern void bv_update();
extern void bv_toggle(void* priv, int delta);
extern void bv_enable();
extern void bv_disable();
extern void iso_auto_restore_hack();
extern void bv_apply_av();
extern void bv_apply_tv();
extern void bv_apply_iso();
extern void bv_update_lensinfo();
extern void bv_auto_update();

/* these will retry until exposure change is confirmed
 * (used for hdr bracketing; to be renamed, since they are also useful for other purposes)
 * they return true on success
 */
extern int hdr_set_rawshutter(int shutter);
extern int hdr_set_rawiso(int iso);
extern int hdr_set_rawaperture(int aperture);
extern int hdr_set_ae(int ae);
extern int hdr_set_flash_ae(int ae);

int lens_take_picture( int wait_to_finish, int allow_af );
int lens_take_pictures( int wait_to_finish, int allow_af, int duration );

/** Will return 1 on success, 0 on error */
extern int
lens_focus(
        int num_steps,
        int stepsize,
        int wait,
        int extra_delay
);

/** Format a distance in mm into something useful */
/** FIXME: not thread-safe */
const char * lens_format_dist(unsigned mm);

/** Pretty prints the shutter speed given the raw shutter value as input */
/** FIXME: not thread-safe */
const char * lens_format_shutter(int raw_shutter);

/** Pretty prints the shutter speed given the shutter reciprocal (times 1000) as input */
/** FIXME: not thread-safe */
const char * lens_format_shutter_reciprocal(int shutter_reciprocal_x1000, int digits);

/** Pretty prints the aperture given the raw value as input */
/** FIXME: not thread-safe */
const char * lens_format_aperture(int raw_aperture);

/** Pretty prints the ISO given the raw value as input */
/** FIXME: not thread-safe */
const char * lens_format_iso(int raw_iso);

#define KELVIN_MIN 1500
#define KELVIN_MAX 15000
#define KELVIN_STEP 100
void lens_set_kelvin(int k);
void lens_set_kelvin_value_only(int k);
void lens_set_custom_wb_gains(int gain_R, int gain_G, int gain_B);

// todo: move these in lens.c and add friendly getters/setters

// exact ISO values would break the feature of coloring ISO's :)
// sprintf("%d,", round(12800 ./ 2.^([56:-1:0]./8)))
                               //~ 100,109,119,130,141,154,168,183,200,218,238,259,283,308,336,367,400,436,476,519,566,617,673,734,800,872,951,1037,1131,1234,1345,1467,1600,1745,1903,2075,2263,2468,2691,2934,3200,3490,3805,4150,4525,4935,5382,5869,6400,6979,7611,12800,25600};
static const uint16_t values_iso[] = {0,100,110,115,125,140,160,170,185,200,220,235,250,280,320,350,380,400,435,470,500,560,640,700,750,800,860,930,1000,1100,1250,1400,1500,1600,1750,1900,2000,2250,2500,2800,3000,3200,3500,3750,4000,4500,5000,5500,6000,6400,12800,25600};
static const uint8_t  codes_iso[]  = {0, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,  128,  136};

// measured from 5D3 in movie mode with expo override, and rounded manually to match Canon values
// at long exposures, the real durations are 32 seconds and 16 seconds; don't round those, since it may be important to know if you are using the intervalometer
// the others are more or less exact (25.5, 20, 12.5, 10...)
// market values:                            30"            25"  20"  20"            15"            13"  10"  10"             8"             6"   6"   5"             4"             3"2  3"   2"5            2"             1"6  1"5  1"3            1"             0"8  0"7  0"6            0"5            0"4  0"3  0"3            1/4            1/5  1/6 1/6             1/8           1/10 1/10 1/13           1/15           1/20     1/25             30             40   45   50             60             80   90  100            125            160  180  200            250            320  350  400            500            640  750  800           1000           1250 1500 1600           2000           2500 3000 3200           4000           5000 6000 6400           8000
static const uint16_t values_shutter[] = {0, 320, 320, 320, 250, 200, 200, 200, 200, 160, 160, 160, 125, 100, 100, 100, 100,  80,  80,  80,  60,  60,  50,  50,  50,  40,  40,  40,  32,  30,  25,  25,  25,  20,  20,  20,  16,  15,  13,  13,  13,  10,  10,  10,   8,   7,   6,   6,   6,   5,   5,   5,   4,   3,   3,   3,   3,   4,   4,   4,   5,   6,   6,   6,   6,   8,   8,   8,  10,  10,  13,  13,  13,  15,  15,  15,  20,  20,  25,  27,  28,  30,  35,  38,  40,  45,  50,  55,  58,  60,  70,  80,  80,  90, 100, 110, 120, 125, 140, 150, 160, 180, 200, 215, 235, 250, 280, 300, 320, 350, 400, 430, 470, 500, 560, 600, 640, 750, 800, 850, 900,1000,1100,1200,1250,1500,1600,1700,1900,2000,2300,2400,2500,3000,3200,3500,3800,4000,4500,4800,5000,6000,6400,7200,7800,8000};
static const uint8_t  codes_shutter[]  = {0,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160};

// aperture*10
// in 1/8ev, but values different than Canon display:
static const uint16_t values_aperture[] = {0,  10,  11,  11,  12,  12,  13,  14,  14,  15,  16,  16,  17,  18,  19,  20,  20,  21,  22,  23,  24,  25,  27,  28,  29,  30,  32,  33,  35,  36,  38,  40,  41,  43,  45,  47,  49,  51,  54,  56,  59,  61,  64,  67,  70,  73,  76,  80,  83,  87,  91,  95,  99, 103, 108, 113, 118, 123, 128, 134, 140, 146, 153, 160, 167, 174, 182, 190, 198, 207, 216, 226, 236, 246, 257, 269, 281, 293, 306, 320, 334, 348, 364, 380, 397, 414, 433, 452};
static const uint8_t  codes_aperture[] =  {0,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96};

// in 1/2 - 1/3 EV, same values as Canon display:
//~ static const int values_aperture[] = {0,12,13,14,16,18,20,22,25,28,32,35,40,45,50,56,63,67,71,80,90,95,100,110,130,140,160,180,190,200,220,250,270,290,320,360,380,400,450};
//~ static const int codes_aperture[] =  {0,13,14,16,19,21,24,27,29,32,35,37,40,44,45,48,51,52,53,56,59,60, 61, 64, 68, 69, 72, 75, 76, 77, 80, 83, 84, 85, 88, 91, 92, 93, 96};

#define RAW2VALUE(param,rawvalue) ((int)values_##param[raw2index_##param(rawvalue)])
#define VALUE2RAW(param,value) ((int)val2raw_##param(value))

// UNIT_1_8_EV
#define APEX_TV(raw) ((int)(raw) - 56)
#define APEX_AV(raw) ((raw) ? (int)(raw) - 8 : 0)
#define APEX_SV(raw) ((raw) ? (int)(raw) - 32 : 0)

// UNIT APEX * 10
#define APEX10_RAW2TV(raw) RSCALE(APEX_TV(raw), 10, 8)
#define APEX10_RAW2AV(raw) RSCALE(APEX_AV(raw), 10, 8)
#define APEX10_RAW2SV(raw) RSCALE(APEX_SV(raw), 10, 8)
#define APEX10_RAW2EC(raw) RSCALE((raw), 10, 8)

#define APEX10_TV2RAW(apex) -APEX_TV(RSCALE(-(apex), 8, 10))
#define APEX10_AV2RAW(apex) -APEX_AV(RSCALE(-(apex), 8, 10))    /* pathological case at f/0.8 */
#define APEX10_SV2RAW(apex) -APEX_SV(RSCALE(-(apex), 8, 10))    /* pathological case at ISO 3.125 */
#define APEX10_AV2VAL(apex) values_aperture[raw2index_aperture(APEX10_AV2RAW(apex))]

#define APEX1000_RAW2TV(raw) RSCALE(APEX_TV(raw), 1000, 8)
#define APEX1000_RAW2AV(raw) RSCALE(APEX_AV(raw), 1000, 8)
#define APEX1000_RAW2SV(raw) RSCALE(APEX_SV(raw), 1000, 8)
#define APEX1000_RAW2EC(raw) RSCALE((raw), 1000, 8)

#define APEX1000_TV2RAW(apex) -APEX_TV(RSCALE(-(apex), 8, 1000))
#define APEX1000_AV2RAW(apex) -APEX_AV(RSCALE(-(apex), 8, 1000))    /* pathological case at f/0.8 */
#define APEX1000_SV2RAW(apex) -APEX_SV(RSCALE(-(apex), 8, 1000))    /* pathological case at ISO 3.125 */
#define APEX1000_EC2RAW(apex) RSCALE(apex, 8, 1000)

// Conversions
int raw2shutter_ms(int raw_shutter);
int shutter_ms_to_raw(int shutter_ms);
int shutterf_to_raw(float shutterf);
float raw2shutterf(int raw_shutter);
int raw2iso(int raw_iso);
int shutterf_to_raw_noflicker(float shutterf);

int raw2index_iso(int raw_iso);
int raw2index_aperture(int raw_aperture);
int raw2index_shutter(int shutter);

/* round exposure parameters to get the nearest valid value */
int round_expo_comp(int ae);
int round_flash_expo_comp(int ae);
int round_aperture(int av);
int round_shutter(int tv, int slowest_shutter);
int expo_value_rounding_ok(int raw, int is_aperture);

#define SWAP_ENDIAN(x) (((x)>>24) | (((x)<<8) & 0x00FF0000) | (((x)>>8) & 0x0000FF00) | ((x)<<24))

void draw_ml_topbar();
void draw_ml_bottombar();

void SW1(int v, int wait);
void SW2(int v, int wait);

void iso_toggle( void * priv, int sign );
void shutter_toggle(void* priv, int sign);
void aperture_toggle( void* priv, int sign);
void kelvin_toggle( void* priv, int sign );

#define MIN_ISO (get_htp() ? 80 : 72)
#define MAX_ISO 128 // may be better to fine-tune this for each camera

// max iso with expo override
#if defined(CONFIG_6D)
#define MAX_ISO_BV 136 // see ControlIso <= LVGAIN_MAX_ISO
#elif defined(CONFIG_100D)
#define MAX_ISO_BV 120 // 128 will freeze if iso expansion not set
#elif defined(CONFIG_DIGIC_V) //All DigicV except 6D apparently
#define MAX_ISO_BV 199
#elif defined(CONFIG_500D)
#define MAX_ISO_BV (is_movie_mode() ? 104 : 112) // 1600 or 3200
#else
#define MAX_ISO_BV 120
#endif

// max ISO that can be set via FRAME_ISO
// I think it's the same as max analog ISO
// todo: ask Guillermo Luijk :)
#if defined(CONFIG_DIGIC_V)
#define MAX_ANALOG_ISO 128 // iso 12800
#else
#define MAX_ANALOG_ISO 112 // iso 3200
#endif

/* split ISO into analog and "digital" components */
/* (further research showed they are not actually digital, just analog amplification very late in the chain, without improving noise ) */
void split_iso(int raw_iso, unsigned int* analog_iso, int* digital_gain);

/* to be renamed, because "native" is not well defined (more like "useful" iso) */
int is_native_iso(int iso);

/* to be renamed to is_fullstop_iso (also check exceptions) */
int is_round_iso(int iso);

/* don't rely on this one yet */
int get_max_analog_iso();

/* auto expo interface (Canon metering) */
int get_max_ae_ev();
int get_ae_value();
int get_bv();   // APEX units
int get_ae_state();

#define AF_ENABLE 1
#define AF_DISABLE 0
#define AF_DONT_CHANGE -1

/* change AF settings (CFN AF button) so a software-triggered picture will autofocus or not */
/* returns 1 if it changed anything */
int lens_setup_af(int should_af);

/* undo changes made by lens_setup_af (setup the user preference back) */
/* note: if you take the battery out between these two calls, you may end up with misconfigured AF button */
/* => my camera doesn't autofocus!!! */
void lens_cleanup_af();

/* todo: move to cfn.h? */
extern int cfn_get_af_button_assignment();
void cfn_set_af_button(int value);
void restore_af_button_assignment_at_shutdown();

int get_alo();
int get_htp();
void set_htp(int value);

// misc macros for avoiding numeric ev values
#define EXPO_1_3_STOP 3
#define EXPO_HALF_STOP 4
#define EXPO_2_3_STOP 5
#define EXPO_FULL_STOP 8

// please extend as required, not every value is here yet
#define ISO_100 72
#define ISO_125 75
#define ISO_160 77
#define ISO_200 80
#define ISO_250 83
#define ISO_320 85
#define ISO_400 88
#define ISO_500 91
#define ISO_640 93
#define ISO_800 96
#define ISO_1000 99
#define ISO_1250 101
#define ISO_1600 104
#define ISO_2000 107
#define ISO_2500 109
#define ISO_3200 112
#define ISO_4000 115
#define ISO_5000 117
#define ISO_6400 120
#define ISO_12800 128
#define ISO_25600 136

#define SHUTTER_BULB 12 /* special value for BULB mode */
#define SHUTTER_MIN 16
#define SHUTTER_30s 16
#define SHUTTER_25s 19
#define SHUTTER_20s 20
#define SHUTTER_15s 24
#define SHUTTER_13s 27
#define SHUTTER_10s 28
#define SHUTTER_8s 32
#define SHUTTER_6s 35
#define SHUTTER_5s 37
#define SHUTTER_4s 40
#define SHUTTER_3s2 43
#define SHUTTER_3s 44
#define SHUTTER_2s5 45
#define SHUTTER_2s 48
#define SHUTTER_1s6 51
#define SHUTTER_1s5 52
#define SHUTTER_1s3 53
#define SHUTTER_1s 56
#define SHUTTER_0s8 59
#define SHUTTER_0s7 60
#define SHUTTER_0s6 61
#define SHUTTER_0s5 64
#define SHUTTER_0s4 67
#define SHUTTER_0s3 68
#define SHUTTER_1_4 72
#define SHUTTER_1_5 75
#define SHUTTER_1_6 76
#define SHUTTER_1_8 80
#define SHUTTER_1_10 83
#define SHUTTER_1_13 85
#define SHUTTER_1_15 88
#define SHUTTER_1_20 92
#define SHUTTER_1_25 93
#define SHUTTER_1_30 96
#define SHUTTER_1_40 99
#define SHUTTER_1_45 100
#define SHUTTER_1_50 101
#define SHUTTER_1_60 104
#define SHUTTER_1_80 107
#define SHUTTER_1_90 108
#define SHUTTER_1_100 109
#define SHUTTER_1_125 112
#define SHUTTER_1_160 115
#define SHUTTER_1_180 116
#define SHUTTER_1_200 117
#define SHUTTER_1_250 120
#define SHUTTER_1_320 123
#define SHUTTER_1_350 124
#define SHUTTER_1_400 125
#define SHUTTER_1_500 128
#define SHUTTER_1_640 131
#define SHUTTER_1_750 132
#define SHUTTER_1_800 133
#define SHUTTER_1_1000 136
#define SHUTTER_1_1250 139
#define SHUTTER_1_1500 140
#define SHUTTER_1_1600 141
#define SHUTTER_1_2000 144
#define SHUTTER_1_2500 147
#define SHUTTER_1_3000 148
#define SHUTTER_1_3200 149
#define SHUTTER_1_4000 152
#define SHUTTER_1_5000 155
#define SHUTTER_1_6000 156
#define SHUTTER_1_6400 157
#define SHUTTER_1_8000 160
#define SHUTTER_MAX 160

#define APERTURE_1_4 16
#define APERTURE_2 24
#define APERTURE_2_8 32
#define APERTURE_3_5 36
#define APERTURE_4 40
#define APERTURE_4_5 44
#define APERTURE_5_6 48
#define APERTURE_6_7 52
#define APERTURE_8 56
#define APERTURE_9_5 60
#define APERTURE_11 64
#define APERTURE_13 68
#define APERTURE_16 72
#define APERTURE_19 76
#define APERTURE_22 80
#define APERTURE_27 84
#define APERTURE_32 88

/* camera ready to take a picture or change shooting settings? */
int job_state_ready_to_take_pic();
void lens_wait_readytotakepic();

/* force an update of PROP_LV_LENS outside LiveView */
void _prop_lv_lens_request_update();

#endif /* _lens_h_ */
