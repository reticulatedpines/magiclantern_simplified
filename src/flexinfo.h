#ifndef _flexinfo_h_
#define _flexinfo_h_

#ifdef FEATURE_FLEXINFO

#define FLEXINFO_DEFAULT_FILENAME "ML/SETTINGS/FLEXINFO.XML"

#define FLEXINFO_DYNAMIC_ENTRIES 64

/* these are binary coded (combineable) flags to either set ABSOLUTE or like (LEFT|CENTER)
   examples:

    INFO_ANCHOR_ABSOLUTE: the element has absolute coordinates but only is printed when the anchor element is shown
    (INFO_ANCHOR_LEFT | INFO_ANCHOR_BOTTOM): element is printed x/y pixels from the anchor elements lower left corner
 */
#define INFO_ANCHOR_ABSOLUTE 0
#define INFO_ANCHOR_LEFT     (1<<0)
#define INFO_ANCHOR_HCENTER  (2<<0)
#define INFO_ANCHOR_RIGHT    (3<<0)
#define INFO_ANCHOR_TOP      (1<<2)
#define INFO_ANCHOR_VCENTER  (2<<2)
#define INFO_ANCHOR_BOTTOM   (3<<2)

#define INFO_ANCHOR_H_MASK   (3<<0)
#define INFO_ANCHOR_V_MASK   (3<<2)

#define INFO_Z_END 0x7FFFFFFF
#define INFO_ANCHOR_NONE 0

#define INFO_PRINT  0
#define INFO_PRERUN 1
 
#define INFO_NAME_LENGTH      16
#define INFO_TEXT_LENGTH      32
#define INFO_FILENAME_LENGTH  32

#define INFO_STATUS_USED      0
#define INFO_STATUS_FREE      1

/* pre-declare here as it will be used in some other structs */
typedef union info_elem_t info_elem_t;

typedef struct
{
    uint32_t type;
    char name[32];
} info_string_map_t;

typedef struct
{
    int32_t x;
    int32_t y;
    int32_t z;
    uint32_t anchor_flags;
    uint32_t anchor;
    uint32_t anchor_flags_self;
    int32_t w;
    int32_t h;
    uint32_t user_disable;
    char name[INFO_NAME_LENGTH];
    int32_t abs_x;
    int32_t abs_y;
    uint32_t shown;
    uint32_t checksum;
    uint32_t redraws;
    char anchor_name[INFO_NAME_LENGTH];
} info_elem_pos_t;


#define INFO_TYPE_CONFIG         0
#define INFO_TYPE_END            1
#define INFO_TYPE_STRING         2
#define INFO_TYPE_FILL           3
#define INFO_TYPE_ICON           4
#define INFO_TYPE_BATTERY_ICON   5
#define INFO_TYPE_BATTERY_PERF   6
#define INFO_TYPE_TEXT           7
#define INFO_TYPE_DYNAMIC       64

typedef struct
{
    uint32_t type;
    info_elem_pos_t pos;
    uint32_t status;
    uint32_t config_pos;
    info_elem_t *config;
} info_elem_header_t;


/* known strings to display - must not be changed as soon XML saving is really used */
#define INFO_STRING_NONE                 0
#define INFO_STRING_ISO                  1
#define INFO_STRING_ISO_MIN              2
#define INFO_STRING_ISO_MAX              3
#define INFO_STRING_ISO_MINMAX           4
#define INFO_STRING_KELVIN               5
#define INFO_STRING_WBS_BA               6
#define INFO_STRING_WBS_GM               7
#define INFO_STRING_DATE_DDMMYYYY        8
#define INFO_STRING_DATE_YYYYMMDD        9
#define INFO_STRING_DATE_MM              10
#define INFO_STRING_DATE_DD              11
#define INFO_STRING_DATE_YY              12
#define INFO_STRING_DATE_YYYY            13
#define INFO_STRING_TIME                 14
#define INFO_STRING_TIME_HH12            15
#define INFO_STRING_TIME_HH24            16
#define INFO_STRING_TIME_MM              17
#define INFO_STRING_TIME_SS              18
#define INFO_STRING_TIME_AMPM            19
#define INFO_STRING_ARTIST               20
#define INFO_STRING_COPYRIGHT            21
#define INFO_STRING_LENS                 22
#define INFO_STRING_BUILD                23
#define INFO_STRING_CARD_LABEL_A         24
#define INFO_STRING_CARD_LABEL_B         25
#define INFO_STRING_CARD_SPACE_A         26
#define INFO_STRING_CARD_SPACE_B         27
#define INFO_STRING_CARD_FILES_A         28
#define INFO_STRING_CARD_FILES_B         29
#define INFO_STRING_CARD_MAKER_A         30
#define INFO_STRING_CARD_MAKER_B         31
#define INFO_STRING_CARD_MODEL_A         32
#define INFO_STRING_CARD_MODEL_B         33
#define INFO_STRING_BATTERY_PCT          34
#define INFO_STRING_BATTERY_ID           35
#define INFO_STRING_PICTURES_AVAIL_AUTO  36
#define INFO_STRING_PICTURES_AVAIL       37
#define INFO_STRING_MLU                  38
#define INFO_STRING_HDR                  39
#define INFO_STRING_CAM_DATE             40
#define INFO_STRING_FREE_GB_INT          41
#define INFO_STRING_FREE_GB_FLOAT        42
#define INFO_STRING_KELVIN_ICO           43
#define INFO_STRING_TEMPERATURE          44
#define INFO_STRING_WBMODE               45
#define INFO_STRING_FOCUSMODE            46
#define INFO_STRING_SHOOTMODE            47
#define INFO_STRING_SHOOTMODE_SHORT      48
#define INFO_STRING_APERTURE             49
#define INFO_STRING_FOCAL_LEN            50
#define INFO_STRING_FOCAL_LEN_EQ         51
#define INFO_STRING_FOCAL_DIST           52
#define INFO_STRING_SHUTTER              53
#define INFO_STRING_IS_MODE              54
#define INFO_STRING_DOF_NEAR             55
#define INFO_STRING_DOF_FAR              56
#define INFO_STRING_DOF_HF               57

#define INFO_FONT_SMALL         0
#define INFO_FONT_MEDIUM        1
#define INFO_FONT_LARGE         2
#define INFO_FONT_SMALL_SHADOW  3
#define INFO_FONT_MEDIUM_SHADOW 4
#define INFO_FONT_LARGE_SHADOW  5
#define INFO_FONT_CANON         6

#define INFO_COL_BG            (0xFF000000)
#define INFO_COL_FIELD         (0xFE000000)
#define INFO_COL_PEEK          (0xFC000000) /* peek at actual position */
#define INFO_COL_PEEK_ABS(x,y) (0xFD000000 | (((x) & 0xFFF) << 12) | ((y) & 0xFFF))
#define INFO_COL_PEEK_REL(x,y) (0xFC000000 | (((x) & 0xFFF) << 12) | ((y) & 0xFFF))

typedef struct
{
    info_elem_header_t hdr;
    uint32_t string_type;
    uint32_t fgcolor;
    uint32_t bgcolor;
    uint32_t font_type;
} info_elem_string_t;

typedef struct
{
    info_elem_header_t hdr;
    char text[INFO_TEXT_LENGTH];
    uint32_t fgcolor;
    uint32_t bgcolor;
    uint32_t font_type;
} info_elem_text_t;

typedef struct
{
    info_elem_header_t hdr;
    uint32_t color;
} info_elem_fill_t;

typedef struct
{
    info_elem_header_t hdr;
    uint32_t show_boundaries;
    uint32_t selected_item;
    uint32_t fast_redraw;
    char name[INFO_NAME_LENGTH];
} info_elem_config_t;

typedef struct
{
    info_elem_header_t hdr;
    uint32_t pct_red;
    uint32_t pct_yellow;
} info_elem_battery_icon_t;

typedef struct
{
    info_elem_header_t hdr;
    uint32_t horizontal;
    uint32_t width;
    uint32_t height;
} info_elem_battery_perf_t;

typedef struct
{
    info_elem_header_t hdr;
    uint32_t fgcolor;
    uint32_t bgcolor;
    char filename[INFO_FILENAME_LENGTH];
    uint8_t *icon_data;
} info_elem_icon_t;

typedef struct
{
    info_elem_header_t hdr;
    /* this function gets called by flexinfo when the owner of this element is asked to release its data. will when e.g. flexinfo is disabling */
    uint32_t (*deinit)(info_elem_t *element);
    /* ask handler to (pre-)render given element */
    uint32_t (*print)(info_elem_t *element, uint32_t run_type);
    /* this is unused by flexinfo - its a private variable for the handler/owner of this element */
    void *priv;
} info_elem_dynamic_t;

union info_elem_t
{
    uint32_t type;
    info_elem_header_t hdr;
    info_elem_config_t config;
    info_elem_string_t string;
    info_elem_text_t text;
    info_elem_battery_icon_t battery_icon;
    info_elem_battery_perf_t battery_perf;
    info_elem_fill_t fill;
    info_elem_icon_t icon;
    info_elem_dynamic_t dynamic;
};

/* register a new element that is unconfigured by default. set it to the type you need it to be and care for it yourself */
info_elem_t *info_add_item();
/* unregister a previously registered element */
void info_free_item(info_elem_t *item);

/* register a new element that is unconfigured by default. set it to the type you need it to be and care for it yourself */
info_elem_t *info_add_item();
/* unregister a previously registered element */
void info_free_item(info_elem_t *item);
/* look up an element by its name */
info_elem_t *info_get_by_name(info_elem_t *config, char *name);

#endif
#endif
