#include <dryos.h>
#include <property.h>
#include <menu.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>
#include <version.h>
#include <flexinfo.h>

#ifdef FEATURE_FLEXINFO

#if !defined(BGMT_Q) //use picstyle button if Q is not present
#define BGMT_Q BGMT_PICSTYLE
#endif

/* the menu is not so useful for end users, but makes it easy to tweak item positions for developers.
   actually only developer build ML from source, so keep it enabled until its in a more mature state and the next release is coming.
*/

#ifdef FEATURE_FLEXINFO_FULL
#define FLEXINFO_DEVELOPER_MENU
#define FLEXINFO_XML_CONFIG
#endif

#define BUF_SIZE 128

// those are not camera-specific LP-E6
#define DISPLAY_BATTERY_LEVEL_1 60 //%
#define DISPLAY_BATTERY_LEVEL_2 20 //%

/* e.g. 5D3 has different free space calculation, decide it by property availability, maybe there are others too */
#define free_space_32k (free_space_raw * (cluster_size>>10) / (32768>>10))
#ifdef PROP_CLUSTER_SIZE
static PROP_INT(PROP_CLUSTER_SIZE, cluster_size);
static PROP_INT(PROP_FREE_SPACE, free_space_raw);
#else
extern int cluster_size;
extern int free_space_raw;
#endif

/* updated every redraw */
int32_t info_bg_color = 0;
int32_t info_field_color = 0;

#ifdef FLEXINFO_DEVELOPER_MENU

uint32_t info_screen_required = 0;

uint32_t info_movestate = 0;
uint32_t info_edit_mode = 0;

#define FLEXINFO_MOVE_UP    1
#define FLEXINFO_MOVE_DOWN  2
#define FLEXINFO_MOVE_LEFT  4
#define FLEXINFO_MOVE_RIGHT 8

#define FLEXINFO_EDIT_KEYPRESS 0
#define FLEXINFO_EDIT_CYCLIC   1


void info_edit_none(uint32_t action, uint32_t parameter);
void info_edit_move(uint32_t action, uint32_t parameter);
void info_edit_hide(uint32_t action, uint32_t parameter);
void info_edit_anchoring(uint32_t action, uint32_t parameter);
void info_edit_parameters(uint32_t action, uint32_t parameter);

void (*info_edit_handlers[])(uint32_t, uint32_t) = 
{
    &info_edit_none,
    &info_edit_move,
    &info_edit_hide,
    &info_edit_anchoring,
    &info_edit_parameters,
};

extern int menu_redraw_blocked;

#endif

/*
    this is the definition of the info screen elements.
    it can either be made switchable for photo and LV setting or put in an array.
    the config can get loaded from an user-save and -editable xml file.

    Q: do we put raw X/Y positions here or keep them im consts.h?
 */
info_elem_t info_config[] =
{
    { .config = { { INFO_TYPE_CONFIG } } },

#if defined(CONFIG_7D)
    /* print ISO range */
    { .string = { { INFO_TYPE_STRING, { ISO_RANGE_POS_X, ISO_RANGE_POS_Y, 2, .name = "ISO Range" }}, INFO_STRING_ISO_MINMAX, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 2 and 3, WB strings */
    { .string = { { INFO_TYPE_STRING, { WBS_POS_X - 10, WBS_POS_Y, 2, .name = "WB GM" }}, INFO_STRING_WBS_GM, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { WBS_POS_X + 60, WBS_POS_Y, 2, .name = "WB BA" }}, INFO_STRING_WBS_BA, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 4, battery_icon referenced as anchor */
    { .battery_icon = { { INFO_TYPE_BATTERY_ICON, { DISPLAY_BATTERY_POS_X, DISPLAY_BATTERY_POS_Y, 2, .name = "Battery Icon" }}, DISPLAY_BATTERY_LEVEL_2, DISPLAY_BATTERY_LEVEL_1 } },
    { .battery_perf = { { INFO_TYPE_BATTERY_PERF, { 100, 2, 3, INFO_ANCHOR_LEFT | INFO_ANCHOR_TOP, 4 }}, /* 0=vert,1=horizontal */ 0, /* x size */ 8, /* y size */ 8 } },
    { .string = { { INFO_TYPE_STRING, { 8, 3, 2, INFO_ANCHOR_HCENTER | INFO_ANCHOR_BOTTOM, 4, INFO_ANCHOR_HCENTER | INFO_ANCHOR_TOP }}, INFO_STRING_BATTERY_PCT, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { -20, 2, 2, INFO_ANCHOR_LEFT | INFO_ANCHOR_TOP, 4 }}, INFO_STRING_BATTERY_ID, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_LARGE } },

    /* entry 8, MLU string */
    { .string = { { INFO_TYPE_STRING, { MLU_STATUS_POS_X, MLU_STATUS_POS_Y, 2, .name = "MLU" }}, INFO_STRING_MLU, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 9, kelvin */
    { .string = { { INFO_TYPE_STRING, { WB_KELVIN_POS_X, WB_KELVIN_POS_Y, 2, .name = "Kelvin" }}, INFO_STRING_KELVIN, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM_SHADOW } },

    /* entry 10, pictures */
    { .fill = { { INFO_TYPE_FILL, { 540, 390, 1, 0, 0, 0, 150, 60, .name = "Pics (clear)" }}, .color = INFO_COL_FIELD } },
    { .string = { { INFO_TYPE_STRING, { 0, 0, 2, .name = "Pics", .anchor = 10, .anchor_flags = INFO_ANCHOR_VCENTER|INFO_ANCHOR_HCENTER, .anchor_flags_self = INFO_ANCHOR_VCENTER|INFO_ANCHOR_HCENTER }}, INFO_STRING_PICTURES_AVAIL, COLOR_FG_NONLV, INFO_COL_FIELD, INFO_FONT_CANON } },

    /* entry 12, header (optional) */
    { .string = { { INFO_TYPE_STRING, { 28, 3, 2, .name = "Date", .user_disable = 1 }}, INFO_STRING_CAM_DATE, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 710, 3, 2, .name = "Build", .user_disable = 1, .anchor_flags_self = INFO_ANCHOR_RIGHT }}, INFO_STRING_BUILD, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },
    /* entry 14, footer (optional) */
    { .string = { { INFO_TYPE_STRING, { 28, 459, 2, .name = "Card label", .user_disable = 1 }}, INFO_STRING_CARD_LABEL_A, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 693, 459, 2, .name = "Copyright", .user_disable = 1, .anchor_flags_self = INFO_ANCHOR_RIGHT }}, INFO_STRING_COPYRIGHT, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },

    /* entry 16, free space */
    { .text = { { INFO_TYPE_TEXT, { 144, 162, 2, .anchor_flags_self = (INFO_ANCHOR_RIGHT | INFO_ANCHOR_BOTTOM), .name = "GB" }}, "GB", COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 0, 2, 2, INFO_ANCHOR_LEFT | INFO_ANCHOR_BOTTOM, 16, INFO_ANCHOR_RIGHT | INFO_ANCHOR_BOTTOM, .name = "Space" }}, INFO_STRING_FREE_GB_FLOAT, COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_CANON } },

    /* entry 18, clock */
    { .string = { { INFO_TYPE_STRING, { 35, 250, 2, .name = "Hrs" }}, INFO_STRING_TIME_HH24, COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_CANON } },
    { .text = { { INFO_TYPE_TEXT, { 0, 0, 2, INFO_ANCHOR_RIGHT | INFO_ANCHOR_TOP, 18, INFO_ANCHOR_LEFT | INFO_ANCHOR_TOP, .name = ":" }}, ":", COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_CANON } },
    { .string = { { INFO_TYPE_STRING, { 0, 0, 2, INFO_ANCHOR_RIGHT | INFO_ANCHOR_TOP, 19, INFO_ANCHOR_LEFT | INFO_ANCHOR_TOP, .name = "Min" }}, INFO_STRING_TIME_MM, COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_CANON } },
    { .string = { { INFO_TYPE_STRING, { 0, -2, 2, INFO_ANCHOR_RIGHT | INFO_ANCHOR_BOTTOM, 20, INFO_ANCHOR_LEFT | INFO_ANCHOR_BOTTOM, .name = "Sec" }}, INFO_STRING_TIME_SS, COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_MEDIUM } },
#endif

#if defined(CONFIG_5D2)
    /* entry 1, print ISO range, not printed correctly, 5D2 has larger ISO range => disable */
    { .string = { { INFO_TYPE_STRING, { ISO_RANGE_POS_X, ISO_RANGE_POS_Y, 2, .name = "ISO Range", .user_disable = 1 }}, INFO_STRING_ISO_MINMAX, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 2 and 3, WB strings */
    { .string = { { INFO_TYPE_STRING, { 320, 384, 2, .name = "WB GM" }}, INFO_STRING_WBS_GM, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 398, 384, 2, .name = "WB BA" }}, INFO_STRING_WBS_BA, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 4, battery_icon referenced as anchor */
    { .battery_icon = { { INFO_TYPE_BATTERY_ICON, { DISPLAY_BATTERY_POS_X, DISPLAY_BATTERY_POS_Y, 2, .name = "Battery Icon" }}, DISPLAY_BATTERY_LEVEL_2, DISPLAY_BATTERY_LEVEL_1 } },
    { .battery_perf = { { INFO_TYPE_BATTERY_PERF, { 4, 0, 3, INFO_ANCHOR_VCENTER | INFO_ANCHOR_RIGHT, 4, INFO_ANCHOR_VCENTER | INFO_ANCHOR_LEFT }}, /* 0=vert,1=horizontal */ 0, /* x size */ 8, /* y size */ 8 } },
    { .string = { { INFO_TYPE_STRING, { 8, 2, 2, INFO_ANCHOR_HCENTER | INFO_ANCHOR_BOTTOM, 4, INFO_ANCHOR_HCENTER | INFO_ANCHOR_TOP }}, INFO_STRING_BATTERY_PCT, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 0, 1, 2, INFO_ANCHOR_VCENTER | INFO_ANCHOR_LEFT, 4, INFO_ANCHOR_VCENTER | INFO_ANCHOR_RIGHT }}, INFO_STRING_BATTERY_ID, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_LARGE } },

    /* entry 8, MLU string */
    { .string = { { INFO_TYPE_STRING, { 260, 310, 2, .name = "MLU" }}, INFO_STRING_MLU, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 9, kelvin */
    { .string = { { INFO_TYPE_STRING, { 505, 275, 2, .name = "Kelvin" }}, INFO_STRING_KELVIN, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM_SHADOW } },

    /* entry 10, pictures */
    { .fill = { { INFO_TYPE_FILL, { 540, 390, 1, 0, 0, 0, 150, 60, .name = "Pics (clear)" }}, .color = INFO_COL_FIELD } },
    { .string = { { INFO_TYPE_STRING, { 0, 0, 2, .name = "Pics", .anchor = 10, .anchor_flags = INFO_ANCHOR_VCENTER|INFO_ANCHOR_HCENTER, .anchor_flags_self = INFO_ANCHOR_VCENTER|INFO_ANCHOR_HCENTER }}, INFO_STRING_PICTURES_AVAIL, COLOR_FG_NONLV, INFO_COL_FIELD, INFO_FONT_CANON } },

    /* entry 12, header (optional) */
    { .string = { { INFO_TYPE_STRING, { 693, 3, 2, .name = "Date", .user_disable = 1, .anchor_flags_self = INFO_ANCHOR_RIGHT }}, INFO_STRING_CAM_DATE, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 28, 459, 2, .name = "Build", .user_disable = 1 }}, INFO_STRING_BUILD, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },
    /* entry 14, footer (optional) */
    { .string = { { INFO_TYPE_STRING, { 28, 3, 2, .name = "Lens", .user_disable = 1 }}, INFO_STRING_LENS, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 693, 459, 2, .name = "Copyright", .user_disable = 1, .anchor_flags_self = INFO_ANCHOR_RIGHT }}, INFO_STRING_COPYRIGHT, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },

    /* entry 16, free space */
    { .text = { { INFO_TYPE_TEXT, { 144, 162, 2, .anchor_flags_self = (INFO_ANCHOR_RIGHT | INFO_ANCHOR_BOTTOM), .name = "GB" }}, "GB", COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 0, 2, 2, INFO_ANCHOR_LEFT | INFO_ANCHOR_BOTTOM, 16, INFO_ANCHOR_RIGHT | INFO_ANCHOR_BOTTOM, .name = "Space" }}, INFO_STRING_FREE_GB_FLOAT, COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_CANON } },

    /* entry 18, clock */
    { .string = { { INFO_TYPE_STRING, { 38, 250, 2, .name = "Hrs" }}, INFO_STRING_TIME_HH24, COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_LARGE } },
    { .text = { { INFO_TYPE_TEXT, { -4, 0, 2, INFO_ANCHOR_RIGHT | INFO_ANCHOR_TOP, 18, INFO_ANCHOR_LEFT | INFO_ANCHOR_TOP, .name = ":" }}, ":", COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_LARGE } },
    { .string = { { INFO_TYPE_STRING, { -4, 0, 2, INFO_ANCHOR_RIGHT | INFO_ANCHOR_TOP, 19, INFO_ANCHOR_LEFT | INFO_ANCHOR_TOP, .name = "Min" }}, INFO_STRING_TIME_MM, COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_LARGE } },
    { .string = { { INFO_TYPE_STRING, { 0, -2, 2, INFO_ANCHOR_RIGHT | INFO_ANCHOR_BOTTOM, 20, INFO_ANCHOR_LEFT | INFO_ANCHOR_BOTTOM, .name = "Sec" }}, INFO_STRING_TIME_SS, COLOR_CYAN, INFO_COL_PEEK, INFO_FONT_MEDIUM } },
#endif

#if defined(CONFIG_5D3) || defined(CONFIG_6D)
    /* print ISO range */
    { .string = { { INFO_TYPE_STRING, { ISO_RANGE_POS_X, ISO_RANGE_POS_Y, 2, .name = "ISO Range" }}, INFO_STRING_ISO_MINMAX, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 2 and 3, WB strings */
    { .string = { { INFO_TYPE_STRING, { WBS_POS_X, WBS_POS_Y, 2, .name = "WB GM" }}, INFO_STRING_WBS_GM, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { WBS_POS_X + 24, WBS_POS_Y, 2, .name = "WB BA" }}, INFO_STRING_WBS_BA, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 4, battery_icon referenced as anchor */
    { .battery_icon = { { INFO_TYPE_BATTERY_ICON, { DISPLAY_BATTERY_POS_X, DISPLAY_BATTERY_POS_Y, 2, .name = "Battery Icon" }}, DISPLAY_BATTERY_LEVEL_2, DISPLAY_BATTERY_LEVEL_1 } },
    { .battery_perf = { { INFO_TYPE_BATTERY_PERF, { 86, 2, 3, INFO_ANCHOR_LEFT | INFO_ANCHOR_TOP, 4 }}, /* 0=vert,1=horizontal */ 0, /* x size */ 8, /* y size */ 8 } },
    { .string = { { INFO_TYPE_STRING, { 8, 0, 2, INFO_ANCHOR_HCENTER | INFO_ANCHOR_BOTTOM, 4, INFO_ANCHOR_HCENTER | INFO_ANCHOR_TOP }}, INFO_STRING_BATTERY_PCT, COLOR_YELLOW, INFO_COL_BG, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 4, 2, 2, INFO_ANCHOR_RIGHT | INFO_ANCHOR_TOP, 4 }}, INFO_STRING_BATTERY_ID, COLOR_YELLOW, INFO_COL_BG, INFO_FONT_LARGE } },

    /* entry 8, MLU string */
    { .string = { { INFO_TYPE_STRING, { MLU_STATUS_POS_X, MLU_STATUS_POS_Y, 2, .name = "MLU" }}, INFO_STRING_MLU, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 9, kelvin */
    { .string = { { INFO_TYPE_STRING, { WB_KELVIN_POS_X, WB_KELVIN_POS_Y, 2, .name = "Kelvin" }}, INFO_STRING_KELVIN, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM_SHADOW } },

    /* entry 10, HDR bracketing status */
    { .string = { { INFO_TYPE_STRING, { HDR_STATUS_POS_X, HDR_STATUS_POS_Y, 2, .name = "HDR" }}, INFO_STRING_HDR, COLOR_YELLOW, INFO_COL_BG, INFO_FONT_MEDIUM } },
#endif

#if defined(CONFIG_60D)
    /* print ISO range */
    { .string = { { INFO_TYPE_STRING, { ISO_RANGE_POS_X, ISO_RANGE_POS_Y, 2, .name = "ISO Range" }}, INFO_STRING_ISO_MINMAX, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 2 and 3, WB strings */
    { .string = { { INFO_TYPE_STRING, { WBS_POS_X, WBS_POS_Y, 2, .name = "WB GM" }}, INFO_STRING_WBS_GM, COLOR_YELLOW, INFO_COL_BG, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { WBS_POS_X + 24, WBS_POS_Y, 2, .name = "WB BA" }}, INFO_STRING_WBS_BA, COLOR_YELLOW, INFO_COL_BG, INFO_FONT_MEDIUM } },

    /* entry 4, battery_icon referenced as anchor */
    { .battery_icon = { { INFO_TYPE_BATTERY_ICON, { DISPLAY_BATTERY_POS_X, DISPLAY_BATTERY_POS_Y, 2, .name = "Battery Icon" }}, DISPLAY_BATTERY_LEVEL_2, DISPLAY_BATTERY_LEVEL_1 } },
    { .battery_perf = { { INFO_TYPE_BATTERY_PERF, { 86, 2, 3, INFO_ANCHOR_LEFT | INFO_ANCHOR_TOP, 4 }}, /* 0=vert,1=horizontal */ 0, /* x size */ 8, /* y size */ 8 } },
    { .string = { { INFO_TYPE_STRING, { 8, 0, 2, INFO_ANCHOR_HCENTER | INFO_ANCHOR_BOTTOM, 4, INFO_ANCHOR_HCENTER | INFO_ANCHOR_TOP }}, INFO_STRING_BATTERY_PCT, COLOR_YELLOW, INFO_COL_BG, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 4, 2, 2, INFO_ANCHOR_RIGHT | INFO_ANCHOR_TOP, 4 }}, INFO_STRING_BATTERY_ID, COLOR_YELLOW, INFO_COL_BG, INFO_FONT_LARGE } },

    /* entry 8, MLU string */
    { .string = { { INFO_TYPE_STRING, { MLU_STATUS_POS_X, MLU_STATUS_POS_Y, 2, .name = "MLU" }}, INFO_STRING_MLU, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM } },

    /* entry 9, kelvin */
    { .string = { { INFO_TYPE_STRING, { WB_KELVIN_POS_X, WB_KELVIN_POS_Y, 2, .name = "Kelvin" }}, INFO_STRING_KELVIN, COLOR_YELLOW, INFO_COL_FIELD, INFO_FONT_MEDIUM_SHADOW } },

    /* entry 10, HDR bracketing status */
    { .string = { { INFO_TYPE_STRING, { HDR_STATUS_POS_X, HDR_STATUS_POS_Y, 2, .name = "HDR" }}, INFO_STRING_HDR, COLOR_YELLOW, INFO_COL_BG, INFO_FONT_MEDIUM } },
#endif

#if defined(CONFIG_500D)
    /* entry 1 and 2, WB strings */
    { .string = { { INFO_TYPE_STRING, { 420, 245, 2, .name = "WB BA" }}, INFO_STRING_WBS_BA, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_MEDIUM_SHADOW } },
    { .string = { { INFO_TYPE_STRING, { 420, 270, 2, .name = "WB GM" }}, INFO_STRING_WBS_GM, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_MEDIUM_SHADOW } },

    /* entry 3, MLU string */
    { .string = { { INFO_TYPE_STRING, { 78, 260, 2, .name = "MLU" }}, INFO_STRING_MLU, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_MEDIUM } },
    
    /* entry 4, Kelvin */
    { .string = { { INFO_TYPE_STRING, { 307+(94/2), 237+(62/2), 2, .anchor_flags_self = INFO_ANCHOR_VCENTER|INFO_ANCHOR_HCENTER, .name = "Kelvin" }}, INFO_STRING_KELVIN, COLOR_FG_NONLV, INFO_COL_FIELD, INFO_FONT_CANON } },
    { .fill =   { { INFO_TYPE_FILL,   { 307, 237, 1, INFO_ANCHOR_ABSOLUTE, 4, INFO_ANCHOR_ABSOLUTE, 94, 62, .name = "Kelvin (clear)" }}, .color = INFO_COL_PEEK } },

    /* entry 6, clock */
    { .string = { { INFO_TYPE_STRING, { 200, 410, 2, .name = "Time" }}, INFO_STRING_TIME, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_LARGE } },
    
    /* entry 7, HDR bracketing status */
    { .string = { { INFO_TYPE_STRING, { 316, 200, 2, .anchor_flags_self = INFO_ANCHOR_HCENTER, .name = "HDR" }}, INFO_STRING_HDR, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_MEDIUM } },

    /* entry 8-11, free space & photos left*/
    { .fill =   { { INFO_TYPE_FILL,   { 519, 396, 1, INFO_ANCHOR_ABSOLUTE, 0, INFO_ANCHOR_ABSOLUTE, 138, 55, .name = "Space (clear)" }}, .color = INFO_COL_PEEK } },
    { .string = { { INFO_TYPE_STRING, { 519+(138/2), 412, 2, .anchor_flags_self = (INFO_ANCHOR_VCENTER|INFO_ANCHOR_HCENTER), .name = "Space Pics" }}, INFO_STRING_PICTURES_AVAIL, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_CANON } },
    { .string = { { INFO_TYPE_STRING, { 573, 442, 3, .anchor_flags_self = (INFO_ANCHOR_VCENTER|INFO_ANCHOR_HCENTER), .name = "Space GB" }}, INFO_STRING_FREE_GB_FLOAT, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_LARGE } },
    { .text =   { { INFO_TYPE_TEXT,   { 2, 0, 3, (INFO_ANCHOR_VCENTER|INFO_ANCHOR_RIGHT), 10, (INFO_ANCHOR_VCENTER|INFO_ANCHOR_LEFT), .name = "GB" }}, "GB", COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_LARGE } },

    /* entry 12, header (optional) */
    { .string = { { INFO_TYPE_STRING, { 28, 2, 2, .name = "Lens", .user_disable = 0 }}, INFO_STRING_LENS, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 710, 2, 2, .name = "Date", .user_disable = 0, .anchor_flags_self = INFO_ANCHOR_RIGHT }}, INFO_STRING_CAM_DATE, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },

    /* entry 13, footer (optional) */
    { .string = { { INFO_TYPE_STRING, { 28, 459, 2, .name = "Build", .user_disable = 0 }}, INFO_STRING_BUILD, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_MEDIUM_SHADOW } },
    { .string = { { INFO_TYPE_STRING, { 693, 459, 2, .name = "Copyright", .user_disable = 0, .anchor_flags_self = INFO_ANCHOR_RIGHT }}, INFO_STRING_COPYRIGHT, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_MEDIUM_SHADOW } },
#endif

#if defined(CONFIG_550D)
    /* entry 1 and 2, WB strings */
    { .string = { { INFO_TYPE_STRING, { 490, 245, 2, .name = "WB BA" }}, INFO_STRING_WBS_BA, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM_SHADOW } },
    { .string = { { INFO_TYPE_STRING, { 490, 270, 2, .name = "WB GM" }}, INFO_STRING_WBS_GM, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM_SHADOW } },

    /* entry 3, MLU string */
    { .string = { { INFO_TYPE_STRING, { 78, 260, 2, .name = "MLU" }}, INFO_STRING_MLU, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_MEDIUM } },
    
    /* entry 4, Kelvin */
    { .string = { { INFO_TYPE_STRING, { 307+(94/2), 237+(62/2), 2, .anchor_flags_self = INFO_ANCHOR_VCENTER|INFO_ANCHOR_HCENTER, .name = "Kelvin" }}, INFO_STRING_KELVIN, COLOR_FG_NONLV, INFO_COL_FIELD, INFO_FONT_CANON } },
    { .fill =   { { INFO_TYPE_FILL,   { 307, 237, 1, INFO_ANCHOR_ABSOLUTE, 4, INFO_ANCHOR_ABSOLUTE, 94, 62, .name = "Kelvin (clear)" }}, .color = INFO_COL_FIELD } },

    /* entry 6, clock */
    { .string = { { INFO_TYPE_STRING, { 200, 410, 2, .name = "Time" }}, INFO_STRING_TIME, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_LARGE } },
    
    /* entry 7, HDR bracketing status */
    { .string = { { INFO_TYPE_STRING, { 316, 200, 2, .anchor_flags_self = INFO_ANCHOR_HCENTER, .name = "HDR" }}, INFO_STRING_HDR, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_MEDIUM } },

    /* entry 8-11, free space & photos left*/
    { .fill =   { { INFO_TYPE_FILL,   { 519, 396, 1, INFO_ANCHOR_ABSOLUTE, 0, INFO_ANCHOR_ABSOLUTE, 138, 55, .name = "Space (clear)" }}, .color = INFO_COL_PEEK } },
    { .string = { { INFO_TYPE_STRING, { 519+(138/2), 412, 2, .anchor_flags_self = (INFO_ANCHOR_VCENTER|INFO_ANCHOR_HCENTER), .name = "Space Pics" }}, INFO_STRING_PICTURES_AVAIL, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_CANON } },
    { .string = { { INFO_TYPE_STRING, { 573, 442, 3, .anchor_flags_self = (INFO_ANCHOR_VCENTER|INFO_ANCHOR_HCENTER), .name = "Space GB" }}, INFO_STRING_FREE_GB_FLOAT, COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_LARGE } },
    { .text =   { { INFO_TYPE_TEXT,   { 2, 0, 3, (INFO_ANCHOR_VCENTER|INFO_ANCHOR_RIGHT), 10, (INFO_ANCHOR_VCENTER|INFO_ANCHOR_LEFT), .name = "GB" }}, "GB", COLOR_FG_NONLV, INFO_COL_PEEK, INFO_FONT_LARGE } },
    
#endif

#if defined(CONFIG_600D) || defined(CONFIG_650D) || defined(CONFIG_700D) || defined(CONFIG_100D) || defined(CONFIG_1100D)
    /* entry 1, max AUTO ISO */
    { .string = { { INFO_TYPE_STRING, { MAX_ISO_POS_X, MAX_ISO_POS_Y, 2, .name = "Max ISO Range"  }}, INFO_STRING_ISO_MAX, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM_SHADOW } },

    /* entry 2 and 3, WB strings */
    { .string = { { INFO_TYPE_STRING, { WBS_GM_POS_X, WBS_GM_POS_Y, 2, .name = "WB GM" }}, INFO_STRING_WBS_GM, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM_SHADOW } },
    { .string = { { INFO_TYPE_STRING, { WBS_POS_X, WBS_POS_Y, 2, .name = "WB BA" }}, INFO_STRING_WBS_BA, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM_SHADOW } },

    /* entry 4, MLU string */
    { .string = { { INFO_TYPE_STRING, { MLU_STATUS_POS_X, MLU_STATUS_POS_Y, 2, .name = "MLU" }}, INFO_STRING_MLU, COLOR_FG_NONLV, INFO_COL_FIELD, INFO_FONT_SMALL } },

    /* entry 5, HDR bracketing status */
    { .string = { { INFO_TYPE_STRING, { HDR_STATUS_POS_X, HDR_STATUS_POS_Y, 2, .name = "HDR" }}, INFO_STRING_HDR, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM_SHADOW } },

    /* entry 6,7,8 and 9, Kelvin */
    { .string = { { INFO_TYPE_STRING, { WB_K_ICON_POS_X, WB_K_ICON_POS_Y, 2, .name = "K ICON" }}, INFO_STRING_KELVIN_ICO, COLOR_FG_NONLV, INFO_COL_FIELD, INFO_FONT_CANON } },
    { .string = { { INFO_TYPE_STRING, { WB_K_ICON_POS_X+1, WB_K_ICON_POS_Y, 2, .name = "K ICON1" }}, INFO_STRING_KELVIN_ICO, COLOR_FG_NONLV, INFO_COL_FIELD, INFO_FONT_CANON } },
    { .string = { { INFO_TYPE_STRING, { WB_K_ICON_POS_X-1, WB_K_ICON_POS_Y, 2, .name = "K ICON2" }}, INFO_STRING_KELVIN_ICO, COLOR_FG_NONLV, INFO_COL_FIELD, INFO_FONT_CANON } },
    { .string = { { INFO_TYPE_STRING, { WB_KELVIN_POS_X, WB_KELVIN_POS_Y, 2, .name = "Kelvin" }}, INFO_STRING_KELVIN, COLOR_FG_NONLV, INFO_COL_FIELD, INFO_FONT_CANON } },

    /* entry 10, header (optional) */
    { .string = { { INFO_TYPE_STRING, { 28, 2, 2, .name = "Lens", .user_disable = 0 }}, INFO_STRING_LENS, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },
    { .string = { { INFO_TYPE_STRING, { 710, 2, 2, .name = "Date", .user_disable = 0, .anchor_flags_self = INFO_ANCHOR_RIGHT }}, INFO_STRING_CAM_DATE, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM } },
    /* entry 11, footer (optional) */
    { .string = { { INFO_TYPE_STRING, { 28, 459, 2, .name = "Build", .user_disable = 0 }}, INFO_STRING_BUILD, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM_SHADOW } },
    { .string = { { INFO_TYPE_STRING, { 693, 459, 2, .name = "Copyright", .user_disable = 0, .anchor_flags_self = INFO_ANCHOR_RIGHT }}, INFO_STRING_COPYRIGHT, COLOR_FG_NONLV, INFO_COL_BG, INFO_FONT_MEDIUM_SHADOW } },
#endif

    { .type = INFO_TYPE_END },
};

#ifdef FLEXINFO_XML_CONFIG

char *info_strncpy(char *dst, char *src, uint32_t length)
{
    uint32_t pos = 0;

    while(pos < length)
    {
        dst[pos] = src[pos];
        if(!src[pos])
        {
            return dst;
        }
        pos++;
    }
    dst[pos] = 0;

    return dst;
}

uint32_t info_xml_get_element(char *config, uint32_t *start_pos, char *buf, uint32_t buf_length)
{
    uint32_t start = 0;
    uint32_t end = 0;
    uint32_t pos = *start_pos;
    char escape_quot = 0;

    /* skip any whitespace */
    while(config[pos] && (config[pos] == ' ' || config[pos] == '\t' || config[pos] == '\r' || config[pos] == '\n'))
    {
        pos++;
    }

    /* reached the end or no starting tag found? */
    if(!config[pos] || config[pos] != '<')
    {
        strcpy(buf, "");
        return 1;
    }

    /* well, then this is our next tag */
    pos++;
    start = pos;

    while(config[pos] && (config[pos] != '>' || escape_quot))
    {
        /* ignore any tags within quotation marks, waiting for an closed quot mark of the same type */
        if(config[pos] == '"' || config[pos] == '\'')
        {
            /* nothing escaped yet? */
            if(!escape_quot)
            {
                /* set our current quotation mark type */
                escape_quot = config[pos];
            }
            else if(escape_quot == config[pos])
            {
                /* same quotation mark hit again, unset it */
                escape_quot = 0;
            }
        }

        /* blank out any whitespace with a real space - as long it is not in a string */
        if(!escape_quot && (config[pos] == '\t' || config[pos] == '\r' || config[pos] == '\n'))
        {
            config[pos] = ' ';
        }
        pos++;
    }

    /* reached the end or no end tag found? */
    if(!config[pos] || config[pos] != '>')
    {
        strcpy(buf, "");
        return 1;
    }

    /* well, then this is our end */
    end = pos - 1;
    *start_pos = pos + 1;

    /* empty tags are quite useless and not well-formed */
    if(end < start || (end - start + 1) >= buf_length )
    {
        strcpy(buf, "");
        return 1;
    }

    /* copy text */
    info_strncpy(buf, &(config[start]), end - start + 1);
    buf[end - start + 1] = '\0';

    return 0;
}

uint32_t info_xml_get_attribute_token(char *attribute_str, char *buf, uint32_t buf_length)
{
    uint32_t start = 0;
    uint32_t end = 0;
    uint32_t pos = 0;
    char escape_quot = 0;

    /* skip any character until next whitespace */
    while(attribute_str[pos] && attribute_str[pos] == ' ')
    {
        pos++;
    }

    /* reached the end or tag end found? */
    if(!attribute_str[pos] || attribute_str[pos] == '/')
    {
        strcpy(buf, "");
        return 1;
    }

    start = pos;

    while(attribute_str[pos] && ((attribute_str[pos] != ' ' && attribute_str[pos] != '/' && attribute_str[pos] != '=') || escape_quot ))
    {
        /* ignore any tags within quotation marks, waiting for an closed quot mark of the same type */
        if(attribute_str[pos] == '"' || attribute_str[pos] == '\'')
        {
            /* nothing escaped yet? */
            if(!escape_quot)
            {
                /* set our current quotation mark type */
                escape_quot = attribute_str[pos];
            }
            else if(escape_quot == attribute_str[pos])
            {
                /* same quotation mark hit again, unset it */
                escape_quot = 0;
            }
        }

        pos++;
    }

    pos--;
    end = pos;

    if(end < pos || (end - start + 1) >= buf_length )
    {
        strcpy(buf, "");
        return 1;
    }

    /* copy text */
    info_strncpy(buf, &(attribute_str[start]), end - start + 1);
    buf[end - start + 1] = '\0';

    return 0;
}

/*
 * gets element_str = "xml_token name1=value1 name2 = value2/"
 * and returns value for given attribute name
 */
uint32_t info_xml_get_attribute(char *element_str, char *attribute, char *buf, uint32_t buf_length)
{
    uint32_t pos = 0;

    /* skip any character until next whitespace to skip tag name */
    while(element_str[pos] && element_str[pos] != ' ')
    {
        pos++;
    }

    pos++;

    /* reached the end or tag end found? */
    if(!element_str[pos] || element_str[pos] == '/')
    {
        strcpy(buf, "");
        return 1;
    }

    /* do this until the end was reached */
    while(1)
    {
        char attribute_token[32];
        char value_token[32];

        /* skip until next non-whitespace */
        while(element_str[pos] && element_str[pos] == ' ')
        {
            pos++;
        }

        /* reached the end or tag end found? */
        if(!element_str[pos] || element_str[pos] == '/')
        {
            strcpy(buf, "");
            return 1;
        }

        if(info_xml_get_attribute_token(&(element_str[pos]), attribute_token, sizeof(attribute_token)))
        {
            strcpy(buf, "");
            return 1;
        }

        pos += strlen(attribute_token);

        /* skip " = " between attribute and value */
        while(element_str[pos] && (element_str[pos] == ' ' || element_str[pos] == '='))
        {
            pos++;
        }

        /* reached the end? */
        if(!element_str[pos])
        {
            strcpy(buf, "");
            return 1;
        }

        /* now get the value of this attribute */
        if(info_xml_get_attribute_token(&(element_str[pos]), value_token, sizeof(value_token)))
        {
            strcpy(buf, "");
            return 1;
        }

        /* if this was the token we looked for, return content */
        if(!strcmp(attribute, attribute_token))
        {
            /* trim quotes */
            if(value_token[0] == '"')
            {
                uint32_t len = strlen(value_token);
                info_strncpy(value_token, &(value_token[1]), len - 2);
            }

            info_strncpy(buf, value_token, buf_length);
            return 0;
        }

        pos += strlen(value_token);
    }

    return 1;
}

uint32_t info_xml_parse_pos(info_elem_t *config, char *config_str)
{
    char buf[32];

    /* all element have x/y etc */
    if(!info_xml_get_attribute(config_str, "x", buf, sizeof(buf)))
    {
        config->hdr.pos.x = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "y", buf, sizeof(buf)))
    {
        config->hdr.pos.y = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "z", buf, sizeof(buf)))
    {
        config->hdr.pos.z = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "w", buf, sizeof(buf)))
    {
        config->hdr.pos.w = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "h", buf, sizeof(buf)))
    {
        config->hdr.pos.h = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "anchor_flags", buf, sizeof(buf)))
    {
        config->hdr.pos.anchor_flags = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "anchor", buf, sizeof(buf)))
    {
        config->hdr.pos.anchor = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "anchor_flags_self", buf, sizeof(buf)))
    {
        config->hdr.pos.anchor_flags_self = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "user_disable", buf, sizeof(buf)))
    {
        config->hdr.pos.user_disable = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "name", buf, sizeof(buf)))
    {
        info_strncpy(config->hdr.pos.name, buf, sizeof(config->hdr.pos.name));
    }

    return 0;
}


uint32_t info_xml_parse_string(info_elem_t *config, char *config_str)
{
    char buf[32];

    uint32_t ret = info_xml_parse_pos(config, config_str);

    if(ret)
    {
        return ret;
    }

    config->type = INFO_TYPE_STRING;

    if(!info_xml_get_attribute(config_str, "string_type", buf, sizeof(buf)))
    {
        config->string.string_type = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "fgcolor", buf, sizeof(buf)))
    {
        config->string.fgcolor = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "bgcolor", buf, sizeof(buf)))
    {
        config->string.bgcolor = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "font_type", buf, sizeof(buf)))
    {
        config->string.font_type = atoi(buf);
    }

    return 0;
}

uint32_t info_xml_parse_text(info_elem_t *config, char *config_str)
{
    char buf[32];

    uint32_t ret = info_xml_parse_pos(config, config_str);

    if(ret)
    {
        return ret;
    }

    config->type = INFO_TYPE_TEXT;

    if(!info_xml_get_attribute(config_str, "text", buf, sizeof(buf)))
    {
        info_strncpy(config->text.text, buf, sizeof(config->text.text));
    }
    if(!info_xml_get_attribute(config_str, "fgcolor", buf, sizeof(buf)))
    {
        config->text.fgcolor = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "bgcolor", buf, sizeof(buf)))
    {
        config->text.bgcolor = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "font_type", buf, sizeof(buf)))
    {
        config->text.font_type = atoi(buf);
    }

    return 0;
}

uint32_t info_xml_parse_fill(info_elem_t *config, char *config_str)
{
    char buf[32];

    uint32_t ret = info_xml_parse_pos(config, config_str);

    if(ret)
    {
        return ret;
    }

    config->type = INFO_TYPE_FILL;

    if(!info_xml_get_attribute(config_str, "color", buf, sizeof(buf)))
    {
        config->fill.color = atoi(buf);
    }

    return 0;
}

uint32_t info_xml_parse_battery_icon(info_elem_t *config, char *config_str)
{
    char buf[32];

    uint32_t ret = info_xml_parse_pos(config, config_str);

    if(ret)
    {
        return ret;
    }

    config->type = INFO_TYPE_BATTERY_ICON;

    if(!info_xml_get_attribute(config_str, "pct_red", buf, sizeof(buf)))
    {
        config->battery_icon.pct_red = atoi(buf);
    }

    if(!info_xml_get_attribute(config_str, "pct_yellow", buf, sizeof(buf)))
    {
        config->battery_icon.pct_yellow = atoi(buf);
    }

    return 0;
}

uint32_t info_xml_parse_battery_perf(info_elem_t *config, char *config_str)
{
    char buf[32];

    uint32_t ret = info_xml_parse_pos(config, config_str);

    if(ret)
    {
        return ret;
    }

    config->type = INFO_TYPE_BATTERY_PERF;

    if(!info_xml_get_attribute(config_str, "horizontal", buf, sizeof(buf)))
    {
        config->battery_perf.horizontal = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "width", buf, sizeof(buf)))
    {
        config->battery_perf.width = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "height", buf, sizeof(buf)))
    {
        config->battery_perf.height = atoi(buf);
    }

    return 0;
}

uint32_t info_xml_parse_icon(info_elem_t *config, char *config_str)
{
    char buf[32];

    uint32_t ret = info_xml_parse_pos(config, config_str);

    if(ret)
    {
        return ret;
    }

    config->type = INFO_TYPE_ICON;

    if(!info_xml_get_attribute(config_str, "fgcolor", buf, sizeof(buf)))
    {
        config->icon.fgcolor = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "bgcolor", buf, sizeof(buf)))
    {
        config->icon.bgcolor = atoi(buf);
    }
    if(!info_xml_get_attribute(config_str, "filename", buf, sizeof(buf)))
    {
        info_strncpy(config->icon.filename, buf, sizeof(config->icon.filename));
    }

    return 0;
}

uint32_t info_load_config(char *filename)
{
	uint32_t allocated_elements = 32;
    uint32_t size = 0;
    uint32_t done = 0;
    uint32_t config_string_pos = 0;
    uint32_t config_element_pos = 0;
    char xml_element[256];
    char attr_buf[64];

    if( FIO_GetFileSize( filename, &size ) != 0 )
    {
        return 1;
	}

	char *xml_config = alloc_dma_memory(size + 1);
    xml_config[size] = '\0';
	if (!xml_config)
	{
        return 1;
	}

	if ((unsigned)read_file(filename, xml_config, size)!=size)
	{
        free_dma_memory(xml_config);
        return 1;
	}

    /* read first xml token */
    if(info_xml_get_element(xml_config, &config_string_pos, xml_element, sizeof(xml_element)))
    {
        free_dma_memory(xml_config);
        return 1;
    }

    /* should be a flexinfo */
    if(strncmp(xml_element, "flexinfo", 8))
    {
        free_dma_memory(xml_config);
        return 1;
    }

    /* attribute tells how many elements are allocated */
    if(!info_xml_get_attribute(xml_element, "elements", attr_buf, sizeof(attr_buf)))
    {
        allocated_elements = atoi(attr_buf) + 3;
    }

    /* allocate the new config */
    info_elem_t *new_config = (info_elem_t *)alloc_dma_memory(allocated_elements*sizeof(info_elem_t));
    memset(new_config, 0, allocated_elements*sizeof(info_elem_t));

    /* first is config header */
    new_config[config_element_pos].type = INFO_TYPE_CONFIG;

    /* config/root element has one configurable attribute. but may be omitted */
    if(!info_xml_get_attribute(xml_element, "name", attr_buf, sizeof(attr_buf)))
    {
        info_strncpy(new_config[config_element_pos].config.name, attr_buf, sizeof(new_config[config_element_pos].config.name));
    }

    config_element_pos++;

    do
    {
        uint32_t ret = 1;
        info_elem_t *element = &(new_config[config_element_pos]);

        /* read next element */
        if(info_xml_get_element(xml_config, &config_string_pos, xml_element, sizeof(xml_element)))
        {
            free_dma_memory(new_config);
            free_dma_memory(xml_config);
            return 1;
        }

        if(!strncmp(xml_element, "string", 6))
        {
            ret = info_xml_parse_string(element, xml_element);
        }
        if(!strncmp(xml_element, "text", 6))
        {
            ret = info_xml_parse_text(element, xml_element);
        }
        if(!strncmp(xml_element, "fill", 4))
        {
            ret = info_xml_parse_fill(element, xml_element);
        }
        if(!strncmp(xml_element, "battery_icon", 12))
        {
            ret = info_xml_parse_battery_icon(element, xml_element);
        }
        if(!strncmp(xml_element, "battery_perf", 12))
        {
            ret = info_xml_parse_battery_perf(element, xml_element);
        }
        if(!strncmp(xml_element, "icon", 4))
        {
            ret = info_xml_parse_icon(element, xml_element);
        }
        if(!strncmp(xml_element, "/flexinfo", 9))
        {
            element->type = INFO_TYPE_END;
            done = 1;
            ret = 0;
        }

        config_element_pos++;
        if(ret || allocated_elements < config_element_pos)
        {
            free_dma_memory(new_config);
            free_dma_memory(xml_config);
            return ret;
        }

    } while (!done);

    free_dma_memory(xml_config);
    free_dma_memory(new_config);
    memcpy(info_config, new_config, config_element_pos * sizeof(info_elem_t));
    return 0;
}

uint32_t info_save_config(info_elem_t *config, char *file)
{
    uint32_t pos = 1;
    uint32_t elements = 0;

    while(config[elements].type != INFO_TYPE_END)
    {
        elements++;
    }

    FILE* f = FIO_CreateFileEx(file);
    if(!f)
    {
        return 1;
    }

    my_fprintf(f, "<flexinfo elements=%d>\n", elements - 1);

    while(config[pos].type != INFO_TYPE_END)
    {
        my_fprintf(f, "    ");
        switch(config[pos].type)
        {
            case INFO_TYPE_STRING:
                my_fprintf(f, "<string ");
                break;
            case INFO_TYPE_TEXT:
                my_fprintf(f, "<text ");
                break;
            case INFO_TYPE_BATTERY_ICON:
                my_fprintf(f, "<battery_icon ");
                break;
            case INFO_TYPE_BATTERY_PERF:
                my_fprintf(f, "<battery_perf ");
                break;
            case INFO_TYPE_FILL:
                my_fprintf(f, "<fill ");
                break;
            case INFO_TYPE_ICON:
                my_fprintf(f, "<icon ");
                break;
        }

        /* dump position field data */
        my_fprintf(f, "name=\"%s\" ", config[pos].hdr.pos.name);
        if(config[pos].hdr.pos.x)
        {
            my_fprintf(f, "x=%d ", config[pos].hdr.pos.x);
        }
        if(config[pos].hdr.pos.y)
        {
            my_fprintf(f, "y=%d ", config[pos].hdr.pos.y);
        }
        if(config[pos].hdr.pos.z)
        {
            my_fprintf(f, "z=%d ", config[pos].hdr.pos.z);
        }
        if(config[pos].hdr.pos.w)
        {
            my_fprintf(f, "w=%d ", config[pos].hdr.pos.w);
        }
        if(config[pos].hdr.pos.h)
        {
            my_fprintf(f, "h=%d ", config[pos].hdr.pos.h);
        }
        if(config[pos].hdr.pos.anchor_flags)
        {
            my_fprintf(f, "anchor_flags=%d ", config[pos].hdr.pos.anchor_flags);
        }
        if(config[pos].hdr.pos.anchor)
        {
            my_fprintf(f, "anchor=%d ", config[pos].hdr.pos.anchor);
        }
        if(config[pos].hdr.pos.anchor_flags_self)
        {
            my_fprintf(f, "anchor_flags_self=%d ", config[pos].hdr.pos.anchor_flags_self);
        }
        if(config[pos].hdr.pos.user_disable)
        {
            my_fprintf(f, "user_disable=%d ", config[pos].hdr.pos.user_disable);
        }

        switch(config[pos].type)
        {
            case INFO_TYPE_STRING:
                if(config[pos].string.string_type)
                {
                    my_fprintf(f, "string_type=%d ", config[pos].string.string_type);
                }
                if(config[pos].string.fgcolor)
                {
                    my_fprintf(f, "fgcolor=%d ", config[pos].string.fgcolor);
                }
                if(config[pos].string.bgcolor)
                {
                    my_fprintf(f, "bgcolor=%d ", config[pos].string.bgcolor);
                }
                if(config[pos].string.font_type)
                {
                    my_fprintf(f, "font_type=%d ", config[pos].string.font_type);
                }
                break;

            case INFO_TYPE_TEXT:
                my_fprintf(f, "text=\"%s\" ", config[pos].text.text);
                if(config[pos].text.fgcolor)
                {
                    my_fprintf(f, "fgcolor=%d ", config[pos].text.fgcolor);
                }
                if(config[pos].text.bgcolor)
                {
                    my_fprintf(f, "bgcolor=%d ", config[pos].text.bgcolor);
                }
                if(config[pos].text.font_type)
                {
                    my_fprintf(f, "font_type=%d ", config[pos].text.font_type);
                }
                break;

            case INFO_TYPE_BATTERY_ICON:
                if(config[pos].battery_icon.pct_red)
                {
                    my_fprintf(f, "pct_red=%d ", config[pos].battery_icon.pct_red);
                }
                if(config[pos].battery_icon.pct_yellow)
                {
                    my_fprintf(f, "pct_yellow=%d ", config[pos].battery_icon.pct_yellow);
                }
                break;

            case INFO_TYPE_BATTERY_PERF:
                if(config[pos].battery_perf.horizontal)
                {
                    my_fprintf(f, "horizontal=%d ", config[pos].battery_perf.horizontal);
                }
                if(config[pos].battery_perf.width)
                {
                    my_fprintf(f, "width=%d ", config[pos].battery_perf.width);
                }
                if(config[pos].battery_perf.height)
                {
                    my_fprintf(f, "height=%d ", config[pos].battery_perf.height);
                }
                break;

            case INFO_TYPE_FILL:
                if(config[pos].fill.color)
                {
                    my_fprintf(f, "color=%d ", config[pos].fill.color);
                }
                break;

            case INFO_TYPE_ICON:
                my_fprintf(f, "filename=\"%s\"", config[pos].icon.filename);
                if(config[pos].icon.fgcolor)
                {
                    my_fprintf(f, "fgcolor=%d ", config[pos].icon.fgcolor);
                }
                if(config[pos].icon.bgcolor)
                {
                    my_fprintf(f, "bgcolor=%d ", config[pos].icon.bgcolor);
                }
                break;
            break;
        }
        my_fprintf(f, "/>\n");
        pos++;
    }

    my_fprintf(f, "</flexinfo>\n");
    FIO_CloseFile(f);

    return 0;
}

/* ********************************************************************************** */
#endif // FLEXINFO_XML_CONFIG

void info_trim_string(char* string)
{
    int dest = 0;
    int src = 0;
    int len = strlen(string);

    /* nothing to do */
    if (len == 0)
    {
        return;
    }

    /* Advance src to the first non-whitespace character */
    while(isspace(string[src]))
    {
        src++;
    }

    /* Copy the string to the "front" of the buffer including NUL */
    for(dest = 0; src < len + 1; dest++, src++)
    {
        string[dest] = string[src];
    }

    /* omit NUL byte */
    dest = strlen(string) - 1;

    /* Working backwards, set all trailing spaces to NUL */
    while(dest >= 0 && isspace(string[dest]))
    {
        string[dest] = '\0';
        dest--;
    }
}

char *info_get_cardmaker(char drv)
{
    if (drv == 'A')
#ifdef CARD_A_MAKER
        return (char*)CARD_A_MAKER;
#else
        return NULL;
#endif
   else
#ifdef CARD_B_MAKER
        return (char*)CARD_B_MAKER;
#else
        return NULL;
#endif
}

char *info_get_cardmodel(char drv)
{
    if (drv == 'A')
#ifdef CARD_A_MODEL
        return (char*)CARD_A_MODEL;
#else
        return NULL;
#endif
   else
#ifdef CARD_B_MODEL
        return (char*)CARD_B_MODEL;
#else
        return NULL;
#endif
}

char *info_get_cardlabel(char drv)
{
    if (drv == 'A')
#ifdef CARD_A_LABEL
        return (char*)CARD_A_LABEL;
#else
        return NULL;
#endif
   else
#ifdef CARD_B_LABEL
        return (char*)CARD_B_LABEL;
#else
        return NULL;
#endif
}


/* ********************************************************************************** */


uint32_t info_get_string(char *buffer, uint32_t maxsize, uint32_t string_type)
{
    strcpy(buffer, "");

    switch(string_type)
    {
        case INFO_STRING_ISO:
        {
            snprintf(buffer, maxsize, "(ISO)");
            break;
        }
        case INFO_STRING_ISO_MIN:
        {
            if (lens_info.raw_iso != 0)
            {
                return 1;
            }
            snprintf(buffer, maxsize, "MIN:%d",raw2iso(auto_iso_range >> 8));
            break;
        }
        case INFO_STRING_ISO_MAX:
        {
            if (lens_info.raw_iso != 0)
            {
                return 1;
            }
            snprintf(buffer, maxsize, "MAX:%d",raw2iso(auto_iso_range & 0xFF));
            break;
        }
        case INFO_STRING_ISO_MINMAX:
        {
            if (lens_info.raw_iso != 0)
            {
                return 1;
            }
            snprintf(buffer, maxsize, "[%d-%d]", MAX((get_htp() ? 200 : 100), raw2iso(auto_iso_range >> 8)), raw2iso(auto_iso_range & 0xFF));
            break;
        }
        case INFO_STRING_KELVIN:
        {
            if (lens_info.wb_mode != WB_KELVIN)
            {
                return 1;
            }
            snprintf(buffer, maxsize, "%d", lens_info.kelvin);
            break;
        }
        case INFO_STRING_KELVIN_ICO:
        {
            if (lens_info.wb_mode != WB_KELVIN)
            {
                return 1;
            }
            snprintf(buffer, maxsize, "K");
            break;
        }
        case INFO_STRING_WBS_BA:
        {
            int ba = lens_info.wbs_ba;
            if (ba == 0)
            {
                return 1;
            }
            snprintf(buffer, maxsize, "%s%d", ba > 0 ? "A" : "B", ABS(ba));
            break;
        }
        case INFO_STRING_WBS_GM:
        {
            int gm = lens_info.wbs_gm;
            if (gm == 0)
            {
                return 1;
            }
            snprintf(buffer, maxsize, "%s%d", gm > 0 ? "G" : "M", ABS(gm));
            break;
        }
        case INFO_STRING_DATE_DDMMYYYY:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%2d.%2d.%4d", now.tm_mday,(now.tm_mon+1),(now.tm_year+1900));
            break;
        }
        case INFO_STRING_DATE_YYYYMMDD:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%4d.%2d.%2d", (now.tm_year+1900),(now.tm_mon+1),now.tm_mday);
            break;
        }
        case INFO_STRING_CAM_DATE:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            if (date_format == DATE_FORMAT_YYYY_MM_DD)
              snprintf(buffer, maxsize, "%4d.%02d.%02d", (now.tm_year+1900),(now.tm_mon+1),now.tm_mday);
            else if (date_format == DATE_FORMAT_MM_DD_YYYY)
              snprintf(buffer, maxsize, "%02d.%02d.%4d", (now.tm_mon+1),now.tm_mday,(now.tm_year+1900));
            else
              snprintf(buffer, maxsize, "%02d.%02d.%4d", now.tm_mday,(now.tm_mon+1),(now.tm_year+1900));
            break;
        }
        case INFO_STRING_DATE_MM:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%2d", (now.tm_mon+1));
            break;
        }
        case INFO_STRING_DATE_DD:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%2d", now.tm_mday);
            break;
        }
        case INFO_STRING_DATE_YY:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%2d", now.tm_year % 100);
            break;
        }
        case INFO_STRING_DATE_YYYY:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%4d", (now.tm_year+1900));
            break;
        }
        case INFO_STRING_TIME:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%02d:%02d", now.tm_hour, now.tm_min);
            break;
        }
        case INFO_STRING_TIME_HH12:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%02d", now.tm_hour % 13);
            break;
        }
        case INFO_STRING_TIME_HH24:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%02d", now.tm_hour);
            break;
        }
        case INFO_STRING_TIME_MM:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%02d", now.tm_min);
            break;
        }
        case INFO_STRING_TIME_SS:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%02d", now.tm_sec);
            break;
        }
        case INFO_STRING_TIME_AMPM:
        {
            struct tm now;
            LoadCalendarFromRTC(&now);
            snprintf(buffer, maxsize, "%s", (now.tm_hour > 12) ? "PM" : "AM");
            break;
        }
        case INFO_STRING_ARTIST:
        {
            snprintf(buffer, maxsize, "%s", artist_name);
            info_trim_string(buffer);
            break;
        }
        case INFO_STRING_COPYRIGHT:
        {
            snprintf(buffer, maxsize, "%s", copyright_info);
            info_trim_string(buffer);
            break;
        }
        case INFO_STRING_LENS:
        {
            snprintf(buffer, maxsize, "%s", lens_info.name);
            break;
        }
        case INFO_STRING_BUILD:
        {
            snprintf(buffer, maxsize, "%s", build_version);
            break;
        }
#ifdef CONFIG_BATTERY_INFO
        case INFO_STRING_BATTERY_PCT:
        {
            snprintf(buffer, maxsize, "%d%%%%", GetBatteryLevel());
            break;
        }
        case INFO_STRING_BATTERY_ID:
        {
            if (GetBatteryHist() == 0)
            {
                return 1;
            }
            snprintf(buffer, maxsize, "%d", GetBatteryHist());
            break;
        }
#else
        case INFO_STRING_BATTERY_PCT:
        case INFO_STRING_BATTERY_ID:
        {
            /* feature not enabled/available */
            return 1;
            break;
        }
#endif
        case INFO_STRING_CARD_LABEL_A:
            // card label is not a null terminated string and the length of it is fix 11 chars
            memcpy(buffer, info_get_cardlabel('A'), 11);
            buffer[11]='\0';
            info_trim_string(buffer);
            break;

        case INFO_STRING_CARD_LABEL_B:
            memcpy(buffer, info_get_cardlabel('B'), 11);
            buffer[11]='\0';
            info_trim_string(buffer);
            break;

        case INFO_STRING_CARD_MAKER_A:
            snprintf(buffer, maxsize, "%s", info_get_cardmaker('A'));
            info_trim_string(buffer);
            break;

        case INFO_STRING_CARD_MAKER_B:
            snprintf(buffer, maxsize, "%s", info_get_cardmaker('B'));
            info_trim_string(buffer);
            break;

        case INFO_STRING_CARD_MODEL_A:
            snprintf(buffer, maxsize, "%s", info_get_cardmodel('A'));
            info_trim_string(buffer);
            break;

        case INFO_STRING_CARD_MODEL_B:
            snprintf(buffer, maxsize, "%s", info_get_cardmodel('B'));
            info_trim_string(buffer);
            break;

        case INFO_STRING_CARD_SPACE_A:
        case INFO_STRING_CARD_SPACE_B:
        case INFO_STRING_CARD_FILES_A:
        case INFO_STRING_CARD_FILES_B:
            snprintf(buffer, maxsize, "(n/a)");
            break;

        case INFO_STRING_PICTURES_AVAIL_AUTO:
        {
            if (avail_shot>99999)
                snprintf(buffer, maxsize, "[99999]");
            else if (avail_shot>9999)
                snprintf(buffer, maxsize, "[%5d]", avail_shot);
            else if (avail_shot>999)
                snprintf(buffer, maxsize, "[%4d]", avail_shot);
            else
                return 1;
            break;
        }

        case INFO_STRING_PICTURES_AVAIL:
        {
            if (avail_shot>99999)
                snprintf(buffer, maxsize, "[99999]");
            else
                snprintf(buffer, maxsize, "%d", avail_shot);
            break;
        }

        case INFO_STRING_MLU:
        {
            if (get_mlu() == 0)
            {
                return 1;
            }
            snprintf(buffer, maxsize, "MLU");
            break;
        }
        case INFO_STRING_HDR:
#ifdef FEATURE_HDR_BRACKETING
        {
            extern int hdr_enabled, hdr_steps, hdr_stepsize;
            if (!hdr_enabled)
            {
                return 1;
            }
            snprintf(buffer, maxsize,
                "HDR %Xx%d%sEV",
                hdr_steps == 1 ? 10 : hdr_steps, // trick: when steps=1 (auto) it will display A :)
                hdr_stepsize / 8,
                ((hdr_stepsize/4) % 2) ? ".5" : "");
            break;
        }
#else
        {
            /* feature not enabled/available */
            return 1;
            break;
        }
#endif
        case INFO_STRING_FREE_GB_INT:
        {
            int fsg = free_space_32k >> 15;
            snprintf(buffer, maxsize, "%d", fsg);
            break;
        }
        case INFO_STRING_FREE_GB_FLOAT:
        {
            int fsg = free_space_32k >> 15;
            int fsgr = free_space_32k - (fsg << 15);
            int fsgf = (fsgr * 10) >> 15;

            snprintf(buffer, maxsize, "%d.%d", fsg, fsgf);
            break;
        }

        /* empty string */
        case INFO_STRING_NONE:
        {
            break;
        }
        /* error */
        default:
            return 1;
    }

    return 0;
}

uint32_t info_measure_string(char *string, uint32_t font_type, int32_t *width, int32_t *height)
{
    int font = -1;
    switch(font_type)
    {
        case INFO_FONT_SMALL:
        case INFO_FONT_SMALL_SHADOW:
            font = FONT_SMALL;
            break;
        case INFO_FONT_MEDIUM:
        case INFO_FONT_MEDIUM_SHADOW:
            font = FONT_MED;
            break;
        case INFO_FONT_LARGE:
        case INFO_FONT_LARGE_SHADOW:
            font = FONT_LARGE;
            break;
        case INFO_FONT_CANON:
        {
            font = 0;
            *width = 0;
            for (char* c = string; *c; c++)
                *width += bfnt_char_get_width(*c);
            *height = 40;
            break;
        }
        /* error */
        default:
            return 1;
    }

    if(font >= 0)
    {
        *width = bmp_string_width(FONT_MED, string);
        *height = fontspec_font(font)->height;
    }

    return 0;
}

uint32_t info_get_anchor_offset(info_elem_t *element, uint32_t flags, int32_t *offset_x, int32_t *offset_y)
{
    switch(flags & INFO_ANCHOR_H_MASK)
    {
        case INFO_ANCHOR_LEFT:
            *offset_x = 0;
            break;
        case INFO_ANCHOR_HCENTER:
            *offset_x = element->hdr.pos.w / 2;
            break;
        case INFO_ANCHOR_RIGHT:
            *offset_x = element->hdr.pos.w;
            break;
        default:
            *offset_x = 0;
            break;
    }

    switch(flags & INFO_ANCHOR_V_MASK)
    {
        case INFO_ANCHOR_TOP:
            *offset_y = 0;
            break;
        case INFO_ANCHOR_VCENTER:
            *offset_y = element->hdr.pos.h / 2;
            break;
        case INFO_ANCHOR_BOTTOM:
            *offset_y = element->hdr.pos.h;
            break;
        default:
            *offset_y = 0;
            break;
    }

    return 0;
}

uint32_t info_get_absolute(info_elem_t *config, info_elem_t *element)
{
    int32_t offset_x = 0;
    int32_t offset_y = 0;

    /* in case of absolute positioning, this is the absolute pos else it is the offset from the anchor */
    element->hdr.pos.abs_x = element->hdr.pos.x;
    element->hdr.pos.abs_y = element->hdr.pos.y;

    /* if the element is relatively positioned to some other element, we have to look it up */
    if(element->hdr.pos.anchor != 0)
    {
        /* determine position from referenced element identified by 'anchor' and update pos_x, pos_y (they contain the offset) */
        info_elem_t *anchor = &(config[element->hdr.pos.anchor]);

        if(!anchor->hdr.pos.shown)
        {
            element->hdr.pos.shown = 0;
        }

        /* calculate anchor offset from top left of anchor item */
        info_get_anchor_offset(anchor, element->hdr.pos.anchor_flags, &offset_x, &offset_y);

        /* if any coordinate was specified to be relative (anchored), update it */
        if(element->hdr.pos.anchor_flags & INFO_ANCHOR_H_MASK)
        {
            element->hdr.pos.abs_x += anchor->hdr.pos.abs_x + offset_x;
        }
        if(element->hdr.pos.anchor_flags & INFO_ANCHOR_V_MASK)
        {
            element->hdr.pos.abs_y += anchor->hdr.pos.abs_y + offset_y;
        }
    }

    /* translate position by own anchor offset */
    info_get_anchor_offset(element, element->hdr.pos.anchor_flags_self, &offset_x, &offset_y);

    /* if any coordinate was specified to be relative (anchored), update it */
    if(element->hdr.pos.anchor_flags_self & INFO_ANCHOR_H_MASK)
    {
        element->hdr.pos.abs_x -= offset_x;
    }
    if(element->hdr.pos.anchor_flags_self & INFO_ANCHOR_V_MASK)
    {
        element->hdr.pos.abs_y -= offset_y;
    }

    return 0;
}

int32_t info_resolve_color(int32_t color, int32_t x, int32_t y)
{
    switch((uint32_t)color >> 24)
    {
        case 0xFF:
            color = info_bg_color;
            break;
        case 0xFE:
            color = info_field_color;
            break;
        case 0xFD:
            color = bmp_getpixel((color>>12) & 0xFFF,color & 0xFFF);
            break;
        case 0xFC:
            color = bmp_getpixel(x + ((color>>12) & 0xFFF), y + (color & 0xFFF));
            break;
        default:
            break;
    }

    return color;
}

uint32_t info_print_string(info_elem_t *config, info_elem_string_t *element, uint32_t run_type)
{
    char str[BUF_SIZE];

    /* get absolute position of this element */
    info_get_absolute(config, (info_elem_t *)element);

    int pos_x = element->hdr.pos.abs_x;
    int pos_y = element->hdr.pos.abs_y;

    /* nothing to show? mark as not shown */
    if(info_get_string(str, BUF_SIZE, element->string_type))
    {
        element->hdr.pos.shown = 0;
    }

    /* anchor not shown or nothing to print */
    if(!element->hdr.pos.shown)
    {
        return 1;
    }

    /* update the width/height */
    info_measure_string(str, element->font_type, &element->hdr.pos.w, &element->hdr.pos.h);

    /* ToDo: make defineable */
    uint32_t fgcolor = element->fgcolor;
    uint32_t bgcolor = element->bgcolor;
    uint32_t fnt;

    /* look up special colors */
    bgcolor = info_resolve_color(bgcolor, pos_x, pos_y);
    fgcolor = info_resolve_color(fgcolor, pos_x, pos_y);

    /* print string if this was not just a pre-pass run */
    if(run_type == INFO_PRINT)
    {
        switch(element->font_type)
        {
            case INFO_FONT_SMALL:
                fnt = FONT(FONT_SMALL, fgcolor, bgcolor);
                bmp_printf(fnt, pos_x, pos_y, str);
                break;
            case INFO_FONT_MEDIUM:
                fnt = FONT(FONT_MED, fgcolor, bgcolor);
                bmp_printf(fnt, pos_x, pos_y, str);
                break;
            case INFO_FONT_LARGE:
                fnt = FONT(FONT_LARGE, fgcolor, bgcolor);
                bmp_printf(fnt, pos_x, pos_y, str);
                break;
            case INFO_FONT_SMALL_SHADOW:
                fnt = SHADOW_FONT(FONT(FONT_SMALL, fgcolor, bgcolor));
                bmp_printf(fnt, pos_x, pos_y, str);
                break;
            case INFO_FONT_MEDIUM_SHADOW:
                fnt = SHADOW_FONT(FONT(FONT_MED, fgcolor, bgcolor));
                bmp_printf(fnt, pos_x, pos_y, str);
                break;
            case INFO_FONT_LARGE_SHADOW:
                fnt = SHADOW_FONT(FONT(FONT_LARGE, fgcolor, bgcolor));
                bmp_printf(fnt, pos_x, pos_y, str);
                break;
            case INFO_FONT_CANON:
                bfnt_puts(str, pos_x, pos_y, fgcolor, bgcolor);
                break;
            /* error */
            default:
                return 1;
        }
    }

    return 0;
}

uint32_t info_print_text(info_elem_t *config, info_elem_text_t *element, uint32_t run_type)
{
    /* get absolute position of this element */
    info_get_absolute(config, (info_elem_t *)element);

    int pos_x = element->hdr.pos.abs_x;
    int pos_y = element->hdr.pos.abs_y;

    /* anchor not shown or nothing to print */
    if(!element->hdr.pos.shown)
    {
        return 1;
    }

    /* update the width/height */
    info_measure_string(element->text, element->font_type, &element->hdr.pos.w, &element->hdr.pos.h);

    /* ToDo: make defineable */
    uint32_t fgcolor = element->fgcolor;
    uint32_t bgcolor = element->bgcolor;
    uint32_t fnt;

    /* look up special colors */
    bgcolor = info_resolve_color(bgcolor, pos_x, pos_y);
    fgcolor = info_resolve_color(fgcolor, pos_x, pos_y);

    /* print string if this was not just a pre-pass run */
    if(run_type == INFO_PRINT)
    {
        switch(element->font_type)
        {
            case INFO_FONT_SMALL:
                fnt = FONT(FONT_SMALL, fgcolor, bgcolor);
                bmp_printf(fnt, pos_x, pos_y, element->text);
                break;
            case INFO_FONT_MEDIUM:
                fnt = FONT(FONT_MED, fgcolor, bgcolor);
                bmp_printf(fnt, pos_x, pos_y, element->text);
                break;
            case INFO_FONT_LARGE:
                fnt = FONT(FONT_LARGE, fgcolor, bgcolor);
                bmp_printf(fnt, pos_x, pos_y, element->text);
                break;
            case INFO_FONT_SMALL_SHADOW:
                fnt = SHADOW_FONT(FONT(FONT_SMALL, fgcolor, bgcolor));
                bmp_printf(fnt, pos_x, pos_y, element->text);
                break;
            case INFO_FONT_MEDIUM_SHADOW:
                fnt = SHADOW_FONT(FONT(FONT_MED, fgcolor, bgcolor));
                bmp_printf(fnt, pos_x, pos_y, element->text);
                break;
            case INFO_FONT_LARGE_SHADOW:
                fnt = SHADOW_FONT(FONT(FONT_LARGE, fgcolor, bgcolor));
                bmp_printf(fnt, pos_x, pos_y, element->text);
                break;
            case INFO_FONT_CANON:
                bfnt_puts(element->text, pos_x, pos_y, fgcolor, bgcolor);
                break;
            /* error */
            default:
                return 1;
        }
    }

    return 0;
}

uint32_t info_print_fill(info_elem_t *config, info_elem_fill_t *element, uint32_t run_type)
{
    /* get absolute position of this element */
    info_get_absolute(config, (info_elem_t *)element);

    /* anchor not shown or nothing to print */
    if(!element->hdr.pos.shown)
    {
        return 1;
    }

    /* look up special colors */
    int32_t color = info_resolve_color(element->color, element->hdr.pos.abs_x, element->hdr.pos.abs_y);

    bmp_fill(color, element->hdr.pos.abs_x, element->hdr.pos.abs_y, element->hdr.pos.w, element->hdr.pos.h);
    return 0;
}

uint32_t info_print_icon(info_elem_t *config, info_elem_icon_t *element, uint32_t run_type)
{
    /* get absolute position of this element */
    info_get_absolute(config, (info_elem_t *)element);

    /* anchor not shown or nothing to print */
    if(!element->hdr.pos.shown)
    {
        return 1;
    }

    return 0;
}

uint32_t info_print_battery_perf(info_elem_t *config, info_elem_battery_perf_t *element, uint32_t run_type)
{
    /* get absolute position of this element */
    info_get_absolute(config, (info_elem_t *)element);

    /* anchor not shown or nothing to print */
    if(!element->hdr.pos.shown)
    {
        return 1;
    }

    int width = element->width;
    int height = element->height;

    if(element->horizontal)
    {
        element->hdr.pos.w = 3 * width + 8;
        element->hdr.pos.h = height;
    }
    else
    {
        element->hdr.pos.w = width;
        element->hdr.pos.h = 3 * height + 4;
    }

#ifdef CONFIG_BATTERY_INFO
    int pos_x = element->hdr.pos.abs_x;
    int pos_y = element->hdr.pos.abs_y;

    if(run_type == INFO_PRINT)
    {
        int perf = GetBatteryPerformance();
        if(element->horizontal)
        {
            bmp_fill((perf<1 ? 50 : COLOR_GREEN2),pos_x,pos_y,width,height);
            bmp_fill((perf<2 ? 50 : COLOR_GREEN2),pos_x+4+width,pos_y,width,height);
            bmp_fill((perf<3 ? 50 : COLOR_GREEN2),pos_x+8+2*width,pos_y,width,height);
        }
        else
        {
            bmp_fill((perf<3 ? 50 : COLOR_GREEN2),pos_x,pos_y,width,height);
            bmp_fill((perf<2 ? 50 : COLOR_GREEN2),pos_x,pos_y+2+height,width,height);
            bmp_fill((perf<1 ? 50 : COLOR_GREEN2),pos_x,pos_y+4+2*height,width,height);
        }
    }
#else
    /* feature n/a, paint it red */
    bmp_fill(COLOR_RED, element->hdr.pos.abs_x, element->hdr.pos.abs_y, element->hdr.pos.w, element->hdr.pos.h);
#endif
    return 0;
}

uint32_t info_print_battery_icon(info_elem_t *config, info_elem_battery_icon_t *element, uint32_t run_type)
{
    element->hdr.pos.w = 96;
    element->hdr.pos.h = 32;

    /* get absolute position of this element */
    info_get_absolute(config, (info_elem_t *)element);

    /* anchor not shown or nothing to print */
    if(!element->hdr.pos.shown)
    {
        return 1;
    }

#if 0 // fights with Canon icon; do not draw, but keep it for positioning the other elements

#ifdef CONFIG_BATTERY_INFO
    int batlev = GetBatteryLevel();
    int info_field_color = bmp_getpixel(615,455);

    int pos_x = element->hdr.pos.abs_x;
    int pos_y = element->hdr.pos.abs_y;

    if(run_type == INFO_PRINT)
    {
        uint batcol = 0;
        uint batfil = 0;
        bmp_fill(info_field_color,pos_x-4,pos_y+14,96,32); // clear the Canon battery icon

        if (batlev <= (int)element->pct_red)
        {
            batcol = COLOR_RED;
        }
        else
        {
            batcol = COLOR_WHITE;
        }

        bmp_fill(batcol,pos_x+10,pos_y,72,32); // draw the new battery icon
        bmp_fill(batcol,pos_x,pos_y+8,12,16);
        bmp_fill(info_field_color,pos_x+14,pos_y+4,64,24);

        if (batlev <= (int)element->pct_red)
        {
            batcol = COLOR_RED;
        }
        else if (batlev <= (int)element->pct_yellow)
        {
            batcol = COLOR_YELLOW;
        }
        else
        {
            batcol = COLOR_GREEN2;
        }

        batfil = batlev*56/100;
        bmp_fill(batcol,pos_x+18+56-batfil,pos_y+8,batfil,16);
    }
#else
    /* feature n/a, paint it red */
    bmp_fill(COLOR_RED, element->hdr.pos.abs_x, element->hdr.pos.abs_y, element->hdr.pos.w, element->hdr.pos.h);
#endif

#endif
    return 0;
}

uint32_t info_get_next_z(info_elem_t *config, uint32_t current)
{
    uint32_t pos = 0;
    uint32_t next = INFO_Z_END;

    while(config[pos].type != INFO_TYPE_END)
    {
        uint32_t z = config[pos].hdr.pos.z;

        if(z >= current && z < next )
        {
            next = z;
        }
        pos++;
    }

    return next;
}

uint32_t info_print_element(info_elem_t *config, info_elem_t *element, uint32_t run_type)
{
    switch(element->type)
    {
        case INFO_TYPE_STRING:
            return info_print_string(config, (info_elem_string_t *)element, run_type);
        case INFO_TYPE_TEXT:
            return info_print_text(config, (info_elem_text_t *)element, run_type);
        case INFO_TYPE_BATTERY_ICON:
            return info_print_battery_icon(config, (info_elem_battery_icon_t *)element, run_type);
        case INFO_TYPE_BATTERY_PERF:
            return info_print_battery_perf(config, (info_elem_battery_perf_t *)element, run_type);
        case INFO_TYPE_FILL:
            return info_print_fill(config, (info_elem_fill_t *)element, run_type);
        case INFO_TYPE_ICON:
            return info_print_icon(config, (info_elem_icon_t *)element, run_type);
    }

    return 1;
}

#ifdef FLEXINFO_DEVELOPER_MENU
uint32_t info_checksum_element(info_elem_t *config)
{
    uint32_t checksum = 0x234AE10A;

    for(int y_pos = 0; y_pos < config->hdr.pos.h; y_pos++)
    {
        for(int x_pos = 0; x_pos < config->hdr.pos.w; x_pos++)
        {
            uint32_t value = bmp_getpixel(config->hdr.pos.abs_x + x_pos, config->hdr.pos.abs_x + y_pos);
            checksum ^= value;
            checksum += value;
            checksum = (checksum >> 31) | (checksum << 1);
        }
    }

    return checksum;
}

uint32_t info_update_element_checksum(info_elem_t *config)
{
    config->hdr.pos.checksum = info_checksum_element(config);
    return 0;
}

uint32_t info_update_checkums(info_elem_t *config)
{
    uint32_t pos = 1;

    while(config[pos].type != INFO_TYPE_END)
    {
        info_checksum_element(&(config[pos]));
        pos++;
    }

    return 0;
}

uint32_t info_checkums_valid(info_elem_t *config)
{
    uint32_t ret = 1;
    uint32_t pos = 1;

    while(config[pos].type != INFO_TYPE_END)
    {
        if(info_checksum_element(&(config[pos])) != config[pos].hdr.pos.checksum)
        {
            ret = 0;
        }
        pos++;
    }

    return ret;
}
#endif

uint32_t info_print_config(info_elem_t *config)
{
    uint32_t pos = 1;
    int32_t z = 0;
    
    #ifdef FLEXINFO_DEVELOPER_MENU
    if(info_screen_required && !info_edit_mode)
    {
        memcpy((void*)get_bvram_mirror(), bmp_vram_idle(), 960*480);
    }
    #endif

    /* read colors again if we redraw over canon gui */
    if(!gui_menu_shown())
    {
        info_bg_color = bmp_getpixel(10,1);
        info_field_color = bmp_getpixel(615,375);
    }

    while(config[pos].type != INFO_TYPE_END)
    {
        /* by default all are set as shown */
        config[pos].hdr.pos.shown = !config[pos].hdr.pos.user_disable;
        pos++;
    }

    pos = 1;
    while(config[pos].type != INFO_TYPE_END)
    {
        /* but check if the elements are invisible. this updates above flag and ensures that elements are only drawn if the anchor (that must come first) is shown */
        info_print_element(config, &(config[pos]), INFO_PRERUN);
        pos++;
    }

    z = info_get_next_z(config, 0);
    while(z != INFO_Z_END)
    {
        pos = 1;
        while(config[pos].type != INFO_TYPE_END)
        {
            if(z == config[pos].hdr.pos.z)
            {
                info_print_element(config, &(config[pos]), INFO_PRINT);

                #ifdef FLEXINFO_DEVELOPER_MENU
                /* if it was shown, update redraw counter */
                if(config[pos].hdr.pos.shown)
                {
                    config[pos].hdr.pos.redraws++;
                }

                /* paint border around item and some label when the item was selected */
                uint32_t selected_item = config[0].config.selected_item;

                if(config[0].config.show_boundaries || (info_edit_mode && (selected_item == pos || config[selected_item].hdr.pos.anchor == pos)))
                {
                    int color = COLOR_RED;

                    /* the currently selected item is drawn green and the anchor target is drawn blue */
                    if(selected_item == pos)
                    {
                        color = COLOR_GREEN1;
                    }
                    else if(config[selected_item].hdr.pos.anchor == pos)
                    {
                        color = COLOR_BLUE;
                    }

                    /* very small sized elements will get drawn as blocks */
                    if(config[pos].hdr.pos.w > 4 && config[pos].hdr.pos.h > 4)
                    {
                        draw_line(config[pos].hdr.pos.abs_x, config[pos].hdr.pos.abs_y, config[pos].hdr.pos.abs_x + config[pos].hdr.pos.w, config[pos].hdr.pos.abs_y, color);
                        draw_line(config[pos].hdr.pos.abs_x, config[pos].hdr.pos.abs_y, config[pos].hdr.pos.abs_x, config[pos].hdr.pos.abs_y + config[pos].hdr.pos.h, color);
                        draw_line(config[pos].hdr.pos.abs_x + config[pos].hdr.pos.w, config[pos].hdr.pos.abs_y + config[pos].hdr.pos.h, config[pos].hdr.pos.abs_x, config[pos].hdr.pos.abs_y + config[pos].hdr.pos.h, color);
                        draw_line(config[pos].hdr.pos.abs_x + config[pos].hdr.pos.w, config[pos].hdr.pos.abs_y + config[pos].hdr.pos.h, config[pos].hdr.pos.abs_x + config[pos].hdr.pos.w, config[pos].hdr.pos.abs_y, color);
                        draw_line(config[pos].hdr.pos.abs_x, config[pos].hdr.pos.abs_y, config[pos].hdr.pos.abs_x + config[pos].hdr.pos.w, config[pos].hdr.pos.abs_y + config[pos].hdr.pos.h, color);
                        draw_line(config[pos].hdr.pos.abs_x + config[pos].hdr.pos.w, config[pos].hdr.pos.abs_y, config[pos].hdr.pos.abs_x, config[pos].hdr.pos.abs_y + config[pos].hdr.pos.h, color);
                    }
                    else
                    {
                        bmp_fill(color,config[pos].hdr.pos.abs_x,config[pos].hdr.pos.abs_y,8,8);
                    }

                    if(selected_item == pos)
                    {
                        /* draw anchor line */
                        info_elem_t *anchor = &(config[config[pos].hdr.pos.anchor]);
                        int32_t anchor_offset_x = 0;
                        int32_t anchor_offset_y = 0;
                        int32_t element_offset_x = 0;
                        int32_t element_offset_y = 0;

                        info_get_anchor_offset(anchor, config[pos].hdr.pos.anchor_flags, &anchor_offset_x, &anchor_offset_y);
                        info_get_anchor_offset(&(config[pos]), config[pos].hdr.pos.anchor_flags_self, &element_offset_x, &element_offset_y);

                        draw_line(anchor->hdr.pos.abs_x + anchor_offset_x, anchor->hdr.pos.abs_y + anchor_offset_y, config[pos].hdr.pos.abs_x + element_offset_x, config[pos].hdr.pos.abs_y + element_offset_y, COLOR_WHITE);
                    }

                    /* now put the title bar */
                    char label[64];
                    int offset = 0;
                    int font_height = fontspec_font(FONT_SMALL)->height;

                    strcpy(label, "");

                    /* position properly when the item is at some border */
                    if(font_height > config[pos].hdr.pos.abs_y)
                    {
                        offset = config[pos].hdr.pos.h;
                    }
                    else
                    {
                        offset = -font_height;
                    }

                    /* any name to print? */
                    if(strlen(config[pos].hdr.pos.name) > 0)
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%s ", config[pos].hdr.pos.name);
                        strcpy(&label[strlen(label)], buf);
                    }

                    if(config[0].config.show_boundaries)
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%d draws ", config[pos].hdr.pos.redraws);
                        strcpy(&label[strlen(label)], buf);
                    }

                    int fnt = FONT(FONT_SMALL, COLOR_WHITE, color);
                    bmp_printf(fnt, COERCE(config[pos].hdr.pos.abs_x, 0, 720), COERCE(config[pos].hdr.pos.abs_y + offset, 0, 480), label);
                }
                #endif // FLEXINFO_DEVELOPER_MENU
            }
            pos++;
        }
        /* find the next highest layers */
        z = info_get_next_z(config, z + 1);
    }
    return 0;
}

uint32_t info_print_screen()
{
    return info_print_config(info_config);
}

#ifdef FLEXINFO_DEVELOPER_MENU

char info_current_menu[64];
char info_current_desc[64];

void info_menu_draw_editscreen(info_elem_t *config)
{
    uint32_t font_type = FONT_MED;
    uint32_t menu_x = 30;
    uint32_t menu_y = 30;
    uint32_t width = MAX(fontspec_font(font_type)->width * (strlen(info_current_menu) + 5), fontspec_font(FONT_SMALL)->width * strlen(info_current_desc));
    uint32_t height = fontspec_font(font_type)->height + fontspec_font(FONT_SMALL)->height;

    /* ensure that the menu isnt displayed directly over the item currently being selected */
    info_elem_t *selected_item = &(config[config[0].config.selected_item]);
    static int32_t ani_offset = 0;

    /* some neat animation to move menu text without confusing user */
    if(selected_item->hdr.pos.abs_y < 100 || selected_item->hdr.pos.abs_x < 100)
    {
        if(ani_offset < 160)
        {
            ani_offset += ((160 - ani_offset) ) / 3;
            config[0].config.fast_redraw = 1;
        }
        else
        {
            config[0].config.fast_redraw = 0;
        }
    }
    else
    {
        if(ani_offset > 0)
        {
            ani_offset -= (ani_offset) / 3;
            config[0].config.fast_redraw = 1;
        }
        else
        {
            config[0].config.fast_redraw = 0;
        }
    }

    menu_y += ani_offset;
    menu_x += ani_offset;
    
    BMP_LOCK
    (
        bmp_draw_to_idle(1);
        
        if(info_screen_required)
        {
            memcpy(bmp_vram_idle(), (void*) get_bvram_mirror(), 960*480);
        }
        else
        {
            /* clear screen */
            bmp_fill(40, 0, 0, 720, 480);
        }

        /* then print the elements */
        info_print_config(config);

        /* and now overwrite with border and menu item */
        bmp_fill(40, menu_x-8, menu_y - fontspec_font(FONT_SMALL)->height - 10, width+16, height+16);
        bmp_fill(COLOR_BLACK, menu_x-4, menu_y-2, width+8, height+4);
        bmp_draw_rect(COLOR_WHITE, menu_x-4, menu_y-2, width+8, height+4);

        /* title */
        int fnt = FONT(FONT_SMALL, COLOR_BLACK, COLOR_WHITE);
        if(config[0].config.selected_item)
        {
            bmp_printf(fnt, menu_x, menu_y - fontspec_font(FONT_SMALL)->height - 2, "Editing: '%s'", selected_item->hdr.pos.name);
        }
        else
        {
            bmp_printf(fnt, menu_x, menu_y - fontspec_font(FONT_SMALL)->height - 2, "Editing: use WHEEL to select item");
        }

        fnt = FONT(font_type, COLOR_WHITE, COLOR_GREEN1);
        bmp_printf(fnt, menu_x, menu_y, "[Q] %s", info_current_menu);
        fnt = FONT(FONT_SMALL, 40, COLOR_GREEN1);
        bmp_printf(fnt, menu_x, menu_y + fontspec_font(font_type)->height, info_current_desc);
        
        bmp_draw_to_idle(0);
        bmp_idle_copy(1,0);
    )
}

void info_edit_none(uint32_t action, uint32_t parameter)
{
    snprintf(info_current_menu, sizeof(info_current_menu), "");
    snprintf(info_current_desc, sizeof(info_current_desc), "");
}

void info_edit_move(uint32_t action, uint32_t parameter)
{
    info_elem_t *item = &info_config[info_config[0].config.selected_item];
    uint32_t keydelay = 0;

    snprintf(info_current_menu, sizeof(info_current_menu), "Move object");
    snprintf(info_current_desc, sizeof(info_current_desc), "CURSOR: move, WHEEL: select item");
    
    switch(action)
    {
        case FLEXINFO_EDIT_KEYPRESS:
        {
            break;
        }
        case FLEXINFO_EDIT_CYCLIC:
        {
            if(keydelay == 0 || keydelay > 15)
            {
                if(info_movestate & FLEXINFO_MOVE_UP)
                {
                    if(item->hdr.pos.y > -480)
                    {
                        item->hdr.pos.y--;
                    }
                }
                if(info_movestate & FLEXINFO_MOVE_DOWN)
                {
                    if(item->hdr.pos.y < 480)
                    {
                        item->hdr.pos.y++;
                    }
                }
                if(info_movestate & FLEXINFO_MOVE_LEFT)
                {
                    if(item->hdr.pos.x > -720)
                    {
                        item->hdr.pos.x--;
                    }
                }
                if(info_movestate & FLEXINFO_MOVE_RIGHT)
                {
                    if(item->hdr.pos.x < 720)
                    {
                        item->hdr.pos.x++;
                    }
                }
            }
            break;
        }
    }
    
    if(!info_movestate)
    {
        keydelay = 0;
    }
    else
    {
        keydelay++;
    }
}

void info_edit_hide(uint32_t action, uint32_t parameter)
{
    info_elem_t *item = &info_config[info_config[0].config.selected_item];
    
    snprintf(info_current_menu, sizeof(info_current_menu), "Visibility: %s", (item->hdr.pos.user_disable)?"Hidden":"Shown");
    snprintf(info_current_desc, sizeof(info_current_desc), "SET: toggle, WHEEL: select item");
    
    switch(action)
    {
        case FLEXINFO_EDIT_KEYPRESS:
        {
            /* handle keys for this mode */
            switch(parameter)
            {
                case BGMT_PRESS_SET:
                #ifdef BGMT_JOY_CENTER
                case BGMT_JOY_CENTER:
                #endif
                    item->hdr.pos.user_disable = !item->hdr.pos.user_disable;
                    break;
                case BGMT_PRESS_LEFT:
                    item->hdr.pos.user_disable = 0;
                    break;
                case BGMT_PRESS_RIGHT:
                    item->hdr.pos.user_disable = 1;
                    break;
            }
            break;
        }
        
        case FLEXINFO_EDIT_CYCLIC:
        {
            break;
        }
    }
}

void info_edit_anchoring(uint32_t action, uint32_t parameter)
{
    char anchor_name[32];
    static uint32_t anchor_mode = 0;
    info_elem_t *item = &info_config[info_config[0].config.selected_item];
    uint32_t *flags = NULL;
    const char *anchor_text[] = {
        "Absolute",
        "Left", "H-Center", "Right",
        "Top",
        "Top-Left", "Top-Center", "Top-Right",
        "V-Center",
        "Left-Center", "Center", "Right-Center",
        "Bottom",
        "Bottom-Left", "Bottom-Center", "Bottom-Right"};
    
    if(item->hdr.pos.anchor)
    {
        snprintf(anchor_name, sizeof(anchor_name), "#%d's", item->hdr.pos.anchor);
    }
    else
    {
        snprintf(anchor_name, sizeof(anchor_name), "(none)");
    }
    
    switch(anchor_mode)
    {
        case 0:
            flags = &item->hdr.pos.anchor_flags_self;
            snprintf(info_current_menu, sizeof(info_current_menu), "own <%s> on item  %s   %s  ", anchor_text[item->hdr.pos.anchor_flags_self & 0x0f], anchor_name, anchor_text[item->hdr.pos.anchor_flags & 0x0f]);
            snprintf(info_current_desc, sizeof(info_current_desc), "WHEEL: select, CURSOR: item anchor point, SET: own anchor");
            break;
            
        case 1:
            snprintf(info_current_menu, sizeof(info_current_menu), "own  %s  on item <%s>  %s  ", anchor_text[item->hdr.pos.anchor_flags_self & 0x0f], anchor_name, anchor_text[item->hdr.pos.anchor_flags & 0x0f]);
            snprintf(info_current_desc, sizeof(info_current_desc), "WHEEL: select, CURSOR: select anchor item, SET: item anchor");
            break;
            
        case 2:
            flags = &item->hdr.pos.anchor_flags;
            snprintf(info_current_menu, sizeof(info_current_menu), "own  %s  on item  %s  <%s> ", anchor_text[item->hdr.pos.anchor_flags_self & 0x0f], anchor_name, anchor_text[item->hdr.pos.anchor_flags & 0x0f]);
            snprintf(info_current_desc, sizeof(info_current_desc), "WHEEL: select, CURSOR: own anchor point, SET: item selection");
            break;
    }
    
    switch(action)
    {
        case FLEXINFO_EDIT_KEYPRESS:
        {
            /* handle keys for this mode */
            switch(parameter)
            {
                case BGMT_PRESS_SET:
                #ifdef BGMT_JOY_CENTER
                case BGMT_JOY_CENTER:
                #endif
                    anchor_mode = (anchor_mode + 1) % 3;
                    break;
                    
                case BGMT_PRESS_LEFT:
                    if(anchor_mode == 1)
                    {
                        if(item->hdr.pos.anchor > 0)
                        {
                            item->hdr.pos.anchor--;
                        }
                    }
                    else
                    {
                        switch(*flags & INFO_ANCHOR_H_MASK)
                        {
                            case INFO_ANCHOR_LEFT:
                                if((*flags & INFO_ANCHOR_V_MASK) == INFO_ANCHOR_TOP)
                                {
                                    *flags = 0;
                                }
                                break;
                            case INFO_ANCHOR_HCENTER:
                                *flags &= ~INFO_ANCHOR_H_MASK;
                                *flags |= INFO_ANCHOR_LEFT;
                                break;
                            case INFO_ANCHOR_RIGHT:
                                *flags &= ~INFO_ANCHOR_H_MASK;
                                *flags |= INFO_ANCHOR_HCENTER;
                                break;
                            default:
                                *flags &= ~INFO_ANCHOR_H_MASK;
                                *flags |= INFO_ANCHOR_LEFT;
                                break;
                        }
                    }
                    break;
                    
                case BGMT_PRESS_RIGHT:
                    if(anchor_mode == 1)
                    {
                        uint32_t count = 0;

                        while(info_config[count].type != INFO_TYPE_END)
                        {
                            count++;
                        }
                        if(item->hdr.pos.anchor < count)
                        {
                            item->hdr.pos.anchor++;
                        }
                    }
                    else
                    {
                        switch(*flags & INFO_ANCHOR_H_MASK)
                        {
                            case INFO_ANCHOR_LEFT:
                                *flags &= ~INFO_ANCHOR_H_MASK;
                                *flags |= INFO_ANCHOR_HCENTER;
                                break;
                            case INFO_ANCHOR_HCENTER:
                                *flags &= ~INFO_ANCHOR_H_MASK;
                                *flags |= INFO_ANCHOR_RIGHT;
                                break;
                            case INFO_ANCHOR_RIGHT:
                                *flags &= ~INFO_ANCHOR_H_MASK;
                                *flags |= INFO_ANCHOR_RIGHT;
                                break;
                            default:
                                *flags &= ~INFO_ANCHOR_H_MASK;
                                *flags |= INFO_ANCHOR_RIGHT;
                                break;
                        }
                    }
                    break;
                    
                case BGMT_PRESS_UP:
                    if(anchor_mode == 1)
                    {
                        item->hdr.pos.anchor = 0;
                    }
                    else
                    {
                        switch(*flags & INFO_ANCHOR_V_MASK)
                        {
                            case INFO_ANCHOR_TOP:
                                *flags &= ~INFO_ANCHOR_V_MASK;
                                *flags |= INFO_ANCHOR_TOP;
                                break;
                            case INFO_ANCHOR_VCENTER:
                                *flags &= ~INFO_ANCHOR_V_MASK;
                                *flags |= INFO_ANCHOR_TOP;
                                break;
                            case INFO_ANCHOR_BOTTOM:
                                *flags &= ~INFO_ANCHOR_V_MASK;
                                *flags |= INFO_ANCHOR_VCENTER;
                                break;
                            default:
                                *flags &= ~INFO_ANCHOR_V_MASK;
                                *flags |= INFO_ANCHOR_TOP;
                                break;
                        }
                    }
                    break;
                    
                case BGMT_PRESS_DOWN:
                    if(anchor_mode == 1)
                    {
                        uint32_t count = 0;

                        while(info_config[count].type != INFO_TYPE_END)
                        {
                            count++;
                        }
                        item->hdr.pos.anchor = count;
                    }
                    else
                    {
                        switch(*flags & INFO_ANCHOR_V_MASK)
                        {
                            case INFO_ANCHOR_TOP:
                                *flags &= ~INFO_ANCHOR_V_MASK;
                                *flags |= INFO_ANCHOR_VCENTER;
                                break;
                            case INFO_ANCHOR_VCENTER:
                                *flags &= ~INFO_ANCHOR_V_MASK;
                                *flags |= INFO_ANCHOR_BOTTOM;
                                break;
                            case INFO_ANCHOR_BOTTOM:
                                *flags &= ~INFO_ANCHOR_V_MASK;
                                *flags |= INFO_ANCHOR_BOTTOM;
                                break;
                            default:
                                *flags &= ~INFO_ANCHOR_V_MASK;
                                *flags |= INFO_ANCHOR_BOTTOM;
                                break;
                        }
                    }
                    break;
            }
            break;
        }
        
        case FLEXINFO_EDIT_CYCLIC:
        {
            break;
        }
    }
}

info_string_map_t info_string_map[] = 
{
    { INFO_STRING_NONE               , "NONE"                },
    { INFO_STRING_ISO                , "ISO"                 },
    { INFO_STRING_ISO_MIN            , "ISO_MIN"             },
    { INFO_STRING_ISO_MAX            , "ISO_MAX"             },
    { INFO_STRING_ISO_MINMAX         , "ISO_MINMAX"          },
    { INFO_STRING_KELVIN             , "KELVIN"              },
    { INFO_STRING_WBS_BA             , "WBS_BA"              },
    { INFO_STRING_WBS_GM             , "WBS_GM"              },
    { INFO_STRING_DATE_DDMMYYYY      , "DATE_DDMMYYYY"       },
    { INFO_STRING_DATE_YYYYMMDD      , "DATE_YYYYMMDD"       },
    { INFO_STRING_DATE_MM            , "DATE_MM"             },
    { INFO_STRING_DATE_DD            , "DATE_DD"             },
    { INFO_STRING_DATE_YY            , "DATE_YY"             },
    { INFO_STRING_DATE_YYYY          , "DATE_YYYY"           },
    { INFO_STRING_TIME               , "TIME"                },
    { INFO_STRING_TIME_HH12          , "TIME_HH12"           },
    { INFO_STRING_TIME_HH24          , "TIME_HH24"           },
    { INFO_STRING_TIME_MM            , "TIME_MM"             },
    { INFO_STRING_TIME_SS            , "TIME_SS"             },
    { INFO_STRING_TIME_AMPM          , "TIME_AMPM"           },
    { INFO_STRING_ARTIST             , "ARTIST"              },
    { INFO_STRING_COPYRIGHT          , "COPYRIGHT"           },
    { INFO_STRING_LENS               , "LENS"                },
    { INFO_STRING_BUILD              , "BUILD"               },
    { INFO_STRING_CARD_LABEL_A       , "CARD_LABEL_A"        },
    { INFO_STRING_CARD_LABEL_B       , "CARD_LABEL_B"        },
    { INFO_STRING_CARD_SPACE_A       , "CARD_SPACE_A"        },
    { INFO_STRING_CARD_SPACE_B       , "CARD_SPACE_B"        },
    { INFO_STRING_CARD_FILES_A       , "CARD_FILES_A"        },
    { INFO_STRING_CARD_FILES_B       , "CARD_FILES_B"        },
    { INFO_STRING_CARD_MAKER_A       , "CARD_MAKER_A"        },
    { INFO_STRING_CARD_MAKER_B       , "CARD_MAKER_B"        },
    { INFO_STRING_CARD_MODEL_A       , "CARD_MODEL_A"        },
    { INFO_STRING_CARD_MODEL_B       , "CARD_MODEL_B"        },
    { INFO_STRING_BATTERY_PCT        , "BATTERY_PCT"         },
    { INFO_STRING_BATTERY_ID         , "BATTERY_ID"          },
    { INFO_STRING_PICTURES_AVAIL_AUTO, "PICTURES_AVAIL_AUTO" },
    { INFO_STRING_PICTURES_AVAIL     , "PICTURES_AVAIL"      },
    { INFO_STRING_MLU                , "MLU"                 },
    { INFO_STRING_HDR                , "HDR"                 },
    { INFO_STRING_CAM_DATE           , "CAM_DATE"            },
    { INFO_STRING_FREE_GB_INT        , "FREE_GB_INT"         },
    { INFO_STRING_FREE_GB_FLOAT      , "FREE_GB_FLOAT"       }
};

void info_edit_parameters(uint32_t action, uint32_t parameter)
{
    info_elem_t *item = &info_config[info_config[0].config.selected_item];
    
    switch(item->type)
    {
        case INFO_TYPE_STRING:
        {
            uint32_t string_index = 0;
            
            while(info_string_map[string_index].type != item->string.string_type && string_index < (sizeof(info_string_map) / sizeof(info_string_map[0])))
            {
                string_index++;
            }
            
            snprintf(info_current_menu, sizeof(info_current_menu), "String: %d (%s)", item->string.string_type, info_string_map[string_index].name);
            snprintf(info_current_desc, sizeof(info_current_desc), "WHEEL: select item, CURSOR: select string");
            switch(action)
            {
                case FLEXINFO_EDIT_KEYPRESS:
                {
                    switch(parameter)
                    {
                        case BGMT_PRESS_LEFT:
                            if(item->string.string_type > 0)
                            {
                                item->string.string_type--;
                            }
                            break;
                            
                        case BGMT_PRESS_RIGHT:
                            if(item->string.string_type < 43)
                            {
                                item->string.string_type++;
                            }
                            break;
                    }
                    break;
                }
            }
            break;
        }
            
        case INFO_TYPE_TEXT:
            snprintf(info_current_menu, sizeof(info_current_menu), "Text: <%s>", item->text.text);
            snprintf(info_current_desc, sizeof(info_current_desc), "WHEEL: select item, (not editable)");
            break;
            
        case INFO_TYPE_BATTERY_ICON:
            snprintf(info_current_menu, sizeof(info_current_menu), "Battery Icon");
            snprintf(info_current_desc, sizeof(info_current_desc), "WHEEL: select item, (not editable)");
            break;
        case INFO_TYPE_BATTERY_PERF:
            snprintf(info_current_menu, sizeof(info_current_menu), "Battery Perf");
            snprintf(info_current_desc, sizeof(info_current_desc), "WHEEL: select item, (not editable)");
            break;
        case INFO_TYPE_FILL:
            snprintf(info_current_menu, sizeof(info_current_menu), "Fill");
            snprintf(info_current_desc, sizeof(info_current_desc), "WHEEL: select item, (not editable)");
            break;
        case INFO_TYPE_ICON:
            snprintf(info_current_menu, sizeof(info_current_menu), "Icon");
            snprintf(info_current_desc, sizeof(info_current_desc), "WHEEL: select item, (not editable)");
            break;
    }
}

int handle_flexinfo_keys(struct event * event)
{
    if (IS_FAKE(event)) return 1; // only process real buttons, not emulated presses
    if (!info_edit_mode) return 1; // only process real buttons, not emulated presses

    switch(event->param)
    {
        /* these are the press-and-hold buttons which are evaluated in cyclic task */
        case BGMT_PRESS_UP:
            info_movestate |= FLEXINFO_MOVE_UP;
            break;
        case BGMT_PRESS_DOWN:
            info_movestate |= FLEXINFO_MOVE_DOWN;
            break;
        case BGMT_PRESS_LEFT:
            info_movestate |= FLEXINFO_MOVE_LEFT;
            break;
        case BGMT_PRESS_RIGHT:
            info_movestate |= FLEXINFO_MOVE_RIGHT;
            break;
        #ifdef BGMT_PRESS_UP_LEFT
        case BGMT_PRESS_UP_LEFT:
            info_movestate |= FLEXINFO_MOVE_UP | FLEXINFO_MOVE_LEFT;
            break;
        case BGMT_PRESS_UP_RIGHT:
            info_movestate |= FLEXINFO_MOVE_UP | FLEXINFO_MOVE_RIGHT;
            break;
        case BGMT_PRESS_DOWN_LEFT:
            info_movestate |= FLEXINFO_MOVE_DOWN | FLEXINFO_MOVE_LEFT;
            break;
        case BGMT_PRESS_DOWN_RIGHT:
            info_movestate |= FLEXINFO_MOVE_DOWN | FLEXINFO_MOVE_RIGHT;
            break;
        #endif

        #ifdef BGMT_UNPRESS_UDLR
        case BGMT_UNPRESS_UDLR:
        #else
        case BGMT_UNPRESS_LEFT:
        case BGMT_UNPRESS_RIGHT:
        case BGMT_UNPRESS_UP:
        case BGMT_UNPRESS_DOWN:
        #endif
            info_movestate = 0;
            break;
            
        /* these are always evaluated */
        case BGMT_PRESS_HALFSHUTTER:
        case BGMT_MENU:
            info_edit_mode = 0;
            break;
            
        #if defined(CONFIG_500D)
        case BGMT_LV:
        #else
        case BGMT_Q:
        #endif
            info_edit_mode = (info_edit_mode + 1) % (sizeof(info_edit_handlers) / sizeof(info_edit_handlers[0]));
            if(info_edit_mode == 0)
            {
                info_edit_mode = 1;
            }
            break;
            
        case BGMT_WHEEL_UP:
        case BGMT_WHEEL_LEFT:
        {
            if(info_config[0].config.selected_item > 1)
            {
                info_config[0].config.selected_item--;
            }
            break;
        }
            
        case BGMT_WHEEL_DOWN:
        case BGMT_WHEEL_RIGHT:
        {
            uint32_t count = 0;

            while(info_config[count].type != INFO_TYPE_END)
            {
                count++;
            }

            if(info_config[0].config.selected_item < (count - 1))
            {
                info_config[0].config.selected_item++;
            }
            break;
        }
    }
    
    info_edit_handlers[info_edit_mode](FLEXINFO_EDIT_KEYPRESS, event->param);
    
    /* if we are not active anymore, pass this event through */
    if(!info_edit_mode)
    {
        return 1;
    }
    return 0;
}

#ifdef FLEXINFO_XML_CONFIG
MENU_SELECT_FUNC(info_menu_save_select)
{
    info_save_config(info_config, FLEXINFO_DEFAULT_FILENAME);
}

MENU_SELECT_FUNC(info_menu_delete_select)
{
    FIO_RemoveFile(FLEXINFO_DEFAULT_FILENAME);
}
#endif

MENU_SELECT_FUNC(info_menu_reset_select)
{
#ifdef FLEXINFO_XML_CONFIG
    info_load_config(FLEXINFO_DEFAULT_FILENAME);
#endif
    info_print_config(info_config);
}

static struct menu_entry info_menus[] = {
    {
        .name = "FlexInfo Settings",
        .select = menu_open_submenu,
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {
            {
                .name = "Show boundaries",
                .priv = &(info_config[0].config.show_boundaries),
                .max = 1,
                .help = "Enable boundary display for all elements.",
            },
            {
                .name = "Use canon screen for edit",
                .priv = &info_screen_required,
                .min = 0,
                .max = 1,
                .help = "Screen will be captured when you leave this menu.",
            },
            {
                .name = "Edit mode",
                .priv = &info_edit_mode,
                .min = 0,
                .max = 1,
                .help = "Enter editing screen.",
            },
#ifdef FLEXINFO_XML_CONFIG
            {
                .name = "Save config",
                .select = info_menu_save_select,
                .help = "Save menu settings",
            },
#endif
            {
                .name = "Delete config",
                .select = info_menu_delete_select,
                .help = "Delete menu settings. Reboot to take effect.",
            },
            /* doesn't work
            {
                .name = "Reset setup",
                .select = info_menu_reset_select,
                .help = "Reset menu settings",
            },*/
            MENU_EOL,
        }
    }
};

static void info_init()
{
    menu_add( "Prefs", info_menus, COUNT(info_menus) );
#ifdef FLEXINFO_XML_CONFIG
    info_load_config(FLEXINFO_DEFAULT_FILENAME);
#endif
}

static void info_edit_task()
{
    TASK_LOOP
    {
        if ((gui_menu_shown() && info_edit_mode) || info_config[0].config.show_boundaries)
        {
            /* if we are editing, move object and block ML menu redraws */
            if(info_edit_mode)
            {
                menu_redraw_blocked = 1;
                info_edit_handlers[info_edit_mode](FLEXINFO_EDIT_CYCLIC, 0);
            }
            
            /* in any case redraw */
            info_menu_draw_editscreen(info_config);
            msleep(20);
        }
        else
        {
            /* no edit mode etc active, reset all variables */
            info_edit_mode = 0;
            menu_redraw_blocked = 0;
            info_movestate = 0;
            
            msleep(500);
        }
    }
}

TASK_CREATE( "info_edit_task", info_edit_task, 0, 0x16, 0x1000 );
INIT_FUNC("info.init", info_init);

#endif // FLEXINFO_DEVELOPER_MENU
#endif // FEATURE_FLEXINFO
