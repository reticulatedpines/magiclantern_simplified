

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
    char name[16];
    int32_t abs_x;
    int32_t abs_y;
    uint32_t shown;
    uint32_t checksum;
    uint32_t redraws;
} info_elem_pos_t;


#define INFO_TYPE_CONFIG 0
#define INFO_TYPE_END    1
#define INFO_TYPE_STRING 2
#define INFO_TYPE_FILL   3
#define INFO_TYPE_ICON   4

#define INFO_TYPE_BATTERY_ICON   5
#define INFO_TYPE_BATTERY_PERF   6

typedef struct
{
    uint32_t type;
    info_elem_pos_t pos;
} info_elem_header_t;

/* known strings to display */
#define INFO_STRING_ISO             1
#define INFO_STRING_ISO_MIN         2
#define INFO_STRING_ISO_MAX         3
#define INFO_STRING_ISO_MINMAX      4
#define INFO_STRING_KELVIN          5
#define INFO_STRING_WBS_BA          6
#define INFO_STRING_WBS_GM          7
#define INFO_STRING_DATE_DDMMYYYY   8
#define INFO_STRING_DATE_YYYYMMDD   9
#define INFO_STRING_DATE_MM         10
#define INFO_STRING_DATE_DD         11
#define INFO_STRING_DATE_YY         12
#define INFO_STRING_DATE_YYYY       13
#define INFO_STRING_TIME            14
#define INFO_STRING_TIME_HH12       15
#define INFO_STRING_TIME_HH24       16
#define INFO_STRING_TIME_MM         17
#define INFO_STRING_TIME_SS         18
#define INFO_STRING_TIME_AMPM       19
#define INFO_STRING_ARTIST          20
#define INFO_STRING_COPYRIGHT       21
#define INFO_STRING_LENS            22
#define INFO_STRING_BUILD           23
#define INFO_STRING_CARD_LABEL_A    24
#define INFO_STRING_CARD_LABEL_B    25
#define INFO_STRING_CARD_SPACE_A    26
#define INFO_STRING_CARD_SPACE_B    27
#define INFO_STRING_CARD_FILES_A    28
#define INFO_STRING_CARD_FILES_B    29
#define INFO_STRING_CARD_MAKER_A    30
#define INFO_STRING_CARD_MAKER_B    31
#define INFO_STRING_CARD_MODEL_A    32
#define INFO_STRING_CARD_MODEL_B    33
#define INFO_STRING_BATTERY_PCT     34
#define INFO_STRING_BATTERY_ID      35
#define INFO_STRING_PICTURES        36
#define INFO_STRING_MLU             37
#define INFO_STRING_HDR             38
#define INFO_STRING_NONE            -1

#define INFO_FONT_SMALL         0
#define INFO_FONT_MEDIUM        1
#define INFO_FONT_LARGE         2
#define INFO_FONT_SMALL_SHADOW  3
#define INFO_FONT_MEDIUM_SHADOW 4
#define INFO_FONT_LARGE_SHADOW  5
#define INFO_FONT_CANON         6

#define INFO_COL_BG    0xFFFFFFFE
#define INFO_COL_FIELD 0xFFFFFFFD


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
    uint32_t color;
} info_elem_fill_t;

typedef struct
{
    info_elem_header_t hdr;
    uint32_t show_boundaries;
    uint32_t selected_item;
    char name[16];
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
    char filename[32];
    uint8_t *icon_data;
} info_elem_icon_t;

typedef union
{
    uint32_t type;
    info_elem_header_t hdr;
    info_elem_config_t config;
    info_elem_string_t string;
    info_elem_battery_icon_t battery_icon;
    info_elem_battery_perf_t battery_perf;
    info_elem_fill_t fill;
    info_elem_icon_t icon;
} info_elem_t;



