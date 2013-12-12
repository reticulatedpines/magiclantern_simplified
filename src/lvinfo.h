/* LiveView info bars */

#ifndef _lvinfo_h_
#define _lvinfo_h_

struct lvinfo_item;

/* Callback routine for info items (optional) */

/* Preferred usage: just set the item->value string and maybe change the colors */

/* For really complex items, you can paint them here 
 * (set item->custom_drawing = 1 and it will be called again when you can paint) */

/* Passive mode (without callback): simply set item->value to a string that gets updated every now and then */

typedef void (*lvinfo_update_func)(
                struct lvinfo_item * item,  /* info item to be displayed */
                int can_draw                /* is it allowed to paint itself? */
        );

/* Use this macro to declare callback routines */
#define LVINFO_UPDATE_FUNC(func) \
    void func ( \
                struct lvinfo_item *        item, \
                int can_draw \
    )

/* Use this macro to statically allocate a buffer for item->value from a callback routine */
#define LVINFO_BUFFER(n) \
    static char buffer[n]; \
    buffer[0] = '\0'; \
    item->value = buffer;

/* on which bar would you like to place your item? */
enum lvinfo_bar
{  
    LV_TOP_BAR_ONLY,
    LV_BOTTOM_BAR_ONLY,
    LV_PREFER_TOP_BAR,
    LV_PREFER_BOTTOM_BAR,
    LV_WHEREVER_IT_FITS
};

/* main info item structure */

/* INIT: you may specify it at initialization */
/* SET:  you may change it at runtime, from the update function */
/* GET:  you can read the value from the update function */
/* PRIV: don't touch it */
/* GUI:  you will be able to change this from menu */
struct lvinfo_item
{
    char* name;                 /* [INIT]     for menu */
    char* value;                /* [INIT/SET] note: you need to allocate RAM for it; use LVINFO_BUFFER for that */
    lvinfo_update_func update;  /* [INIT/CBR] called before displaying; can override strings, dimensions and so on */
    int8_t preferred_position;  /* [INIT/GUI] 0 = center, -1, -2 = to the left, all items get sorted according to this field */
    int8_t priority;            /* [INIT/SET] if there's not enough space, the items with low priority will disappear */
    enum lvinfo_bar which_bar:8;/* [INIT/GUI] where to render this item */

    int16_t width;              /* [GET/SET]  default: measured from value and fontspec; 0 = do not display this item at all */
    int16_t height;             /* [GET/SET]  default: fontspec height */
    int8_t color_fg;            /* [GET/SET]  text and background colors */
    int8_t color_bg;            /* [GET/SET]  default: gray in photo mode, black in movie mode */
    
    unsigned custom_drawing: 1; /* [SET]      "update" should set this to 1 if it needs custom drawing, then it will be called again with can_draw = 1 */
    unsigned disabled: 1;       /* [PRIV/GUI] user may want to disable certain items (by default, all stuff is enabled) */
    unsigned hidden: 1;         /* [PRIV]     some items may be hidden automatically if they don't fit */
    unsigned placed: 1;         /* [PRIV]     already placed on the destination bar */

    uint32_t fontspec;          /* [GET]      assigned by the layout engine (FONT_MED_LARGE or FONT_MED) */
    int16_t x;                  /* [GET]      computed by the layout engine (don't override) */
    int16_t y;                  /* [GET]      anchor: top center */
};

SIZE_CHECK_STRUCT(lvinfo_item, 32);

/* add a new info item to LiveView bars */
/* info_item MUST be allocated by the caller */

/* example: 

     static struct lvinfo_item item = {
        .name = "abc",
        .value = &my_string,                // you can have a statically allocated string that may get updated every now and then
        .update = my_update_func            // or a dynamic function that gets called when the info screen is about to redraw
        .which_bar = LV_PREFER_TOP_BAR,
        ...
    };
    lvinfo_add_item(&item);
    
or:
    static struct lvinfo_item items[] = {
        { 
            .name = "foo",
            ...
        },
        { 
            .name = "bar",
            ...
        },
    };
    lvinfo_add_items(items, COUNT(items));
*/

void lvinfo_add_items(struct lvinfo_item * items, int count);
void lvinfo_add_item(struct lvinfo_item * item);

#endif
