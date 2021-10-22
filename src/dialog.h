/** \file
 * DryOS dialog/window interface.
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

#ifndef _dryos_dialog_h_
#define _dryos_dialog_h_

#include "compiler.h"

typedef uint32_t gui_event_t;

/** Windowing system elements */
/*
 * kitor: completly wrong for R, however unused so I left it as is for now.
 */
struct winsys_struct
{
        void *                  vram_instance; // off 0x00
        struct vram_object *    vram_object; // off 0x04
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                off_0x1c;
        uint32_t                off_0x20;
        struct semaphore *      sem; // off 0x24
        uint32_t                off_0x28;
        uint32_t                off_0x2c;
        uint32_t                flag_0x30;
        uint32_t                flag_0x34;
        uint32_t                off_0x38;
        uint32_t                off_0x3c;
        uint32_t                off_0x40;
        uint32_t                off_0x44; // used for collapsed vram?
        uint32_t                off_0x48;
        uint32_t                off_0x4c;
};

SIZE_CHECK_STRUCT( winsys_struct, 0x50 );

extern struct winsys_struct winsys_struct;


typedef int (*window_callback)( void * );

/** Returned by window_create_maybe() at 0xFFA6BC00 */
/**
 * kitor: On R180 struct gained unk_04, related with new arg to window_create()
 *        unk_01 is related to redraw calls (guess: is redraw needed)
 *        wx_maybe and wy_maybe on R180 are struct window pointers
 */
struct window
{
        const char *            type;         // "Window Instance"
        uint32_t                unk_01;       // initial=0
        uint32_t                unk_02;       // initial=0
        uint32_t                unk_03;       // initial=0
        uint32_t                x;
        uint32_t                y;
        uint32_t                width;
        uint32_t                height;
        window_callback         callback;
        void *                  arg;
#ifdef CONFIG_R
        uint32_t                unk_04;
#endif
        uint32_t                wx_maybe;
        uint32_t                wy_maybe;
};

#ifdef CONFIG_R
SIZE_CHECK_STRUCT( window, 0x34 );
#else
SIZE_CHECK_STRUCT( window, 0x30 );
#endif

/**
 * kitor: more like WINSYS_CreateWindowInstance() on new generations
 * EOS R has one more param at the end. Unconfirmed guess: z-index
 */
extern struct window *
window_create(
        uint32_t                x,
        uint32_t                y,
        uint32_t                w,
        uint32_t                h,
        window_callback         callback,
        void *                  arg
);

struct dialog;
struct dialog_list;
struct dialog_item;

/** Returns 0 if it handled the message, 1 if it did not? */
typedef int (*dialog_handler_t)(
        struct dialog *         self,
        void *                  arg,
        gui_event_t             event
);


/** These are chock-full of callbacks.  I don't know what most of them do. */
struct dialog_item
{
        const char *            type;                   // "DIALOGITEM" at 0xFFCA7B1c
        struct dialog_list *    next;                   // maybe parent? 0x04
        struct dialog_list *    prev;                   // maybe 0x08
        struct dialog_callbacks * callbacks;            // maybe object? 0x0c
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        uint32_t                off_0x1c;
        uint32_t                off_0x20;
        uint32_t                off_0x24;
        uint32_t                off_0x28;
        void                    (*move_callback)( struct dialog *, int x, int y ); //           off_0x2c;
        uint32_t                off_0x30;
        uint32_t                off_0x34;
        uint32_t                off_0x38;
        uint32_t                off_0x3c;
};
SIZE_CHECK_STRUCT( dialog_item, 0x40 );


/** Dialog children?  Maybe? */
struct dialog_list
{
        uint16_t                index;                  // off 0x00
        uint16_t                off_0x02;
        struct dialog_item *    item;                   // off 0x04 maybe
        uint32_t                arg1;                   // off 0x08, passed to creat
        uint32_t                arg2;                   // off_0x0c, passed to creat
        uint32_t                off_0x10;
        uint32_t                off_0x14;
        uint32_t                off_0x18;
        struct dialog_list *    next;                   // off 0x1c
};


/** Dialog box gui elements */
#if defined(CONFIG_R) || defined(CONFIG_M50)
// tested on R and M50. RP has slightly different size.
struct dialog {
    char * type;
    struct window * window;
    uint32_t arg4; /* 4th arg to WINSYS_CreateDialogBox, referenced as ID in some prints */
    struct subscriberClass * hLanguageSubscriber;
    struct subscriberClass * hTerminateSubscriber; /* disp_sw_controller? */
    uint32_t refresh_x; /* rectangle to redraw? */
    uint32_t refresh_y;
    uint32_t refresh_w;
    uint32_t refresh_h;
    uint32_t origin_maybe; /* not sure, referenced as origin in messages */
    uint32_t pos_x;
    uint32_t pos_y;
    uint32_t pos_w;
    uint32_t pos_h;
    uint32_t field_0x38;
    uint32_t field_0x3c;
    struct gui_task * controller; /* CtrlServ aka gui_task object */
    uint8_t field_0x44;
    uint8_t field_0x45;
    uint8_t field_0x46;
    uint8_t field_0x47;
    uint32_t field_0x48;
    uint32_t * field_0x4c;
    uint32_t * field_0x50;
    void * handler;
    void * handler_arg;
    uint32_t field_0x5c; /* set in WINSYS_GetFocusedItemIDOfDialogItem_maybe */
    uint16_t field_0x60;
    uint16_t field_0x62;
    uint16_t field_0x64;
    uint16_t field_0x66;
    uint32_t field_0x68;
    uint32_t field_0x6c;
    uint32_t field_0x70;
    uint8_t field_0x74;
    uint8_t field_0x75;
    uint8_t field_0x76;
    uint8_t field_0x77;
    uint32_t field_0x78;
    uint32_t const_40000000_0; /* set in WINSYS_CreateDialogBox */
    uint32_t some_w; /* see WINSYS_ResizeDialogBox_maybe */
    uint32_t some_h;
    uint32_t id; /* is id ID really? See 0x8, this is referenced as ID */
    uint32_t level_maybe;
    uint32_t const_40000000_1; /* set in WINSYS_CreateDialogBox */
    uint16_t field_0x94;
    uint8_t field_0x96;
    uint8_t field_0x97;
    void * child_list_maybe; /* buffer of (pointer size * count below) */
    uint child_list_count_maybe; /* search table length %d; +2 from "real" size - see WINSYS_CreateDialogBox */
    uint32_t reaction_x; /* see WINSYS_SetReactionAreaToDialog */
    uint32_t reaction_y;
    uint32_t reaction_w;
    uint32_t reaction_h;
    uint32_t field_0xb0;
    uint32_t field_0xb4;
    uint32_t field_0xb8;
    uint32_t field_0xbc;
    uint32_t field_0xc0;
    uint16_t field_0xc4;
    uint8_t field_0xc6;
    uint8_t field_0xc7;
    uint8_t field_0xc8;
    uint8_t field_0xc9;
    uint8_t field_0xca;
    uint8_t field_0xcb;
    uint32_t field_0xcc;
    uint32_t field_0xd0;
    uint16_t field_0xd4;
    uint8_t field_0xd6;
    uint8_t field_0xd7;
    uint32_t field_0xd8;
    uint32_t field_0xdc;
    uint32_t field_0xe0;
    uint32_t field_0xe4;
    uint32_t field_0xe8;
    uint32_t field_0xec;
    uint32_t field_0xf0;
    uint32_t field_0xf4;
    uint32_t field_0xf8;
    uint32_t field_0xfc;
    uint32_t field_0x100;
    uint32_t field_0x104;
    uint32_t field_0x108;
    uint32_t field_0x10c;
    uint32_t field_0x110;
    uint32_t field_0x114;
    uint32_t field_0x118;
    uint32_t rotationAngle;
};
#elif defined(CONFIG_DIGIC_VI) || defined(CONFIG_DIGIC_VII)
// confirmed on 750D, 200D
struct dialog {
    const char * type; //class, pSignature
    void * hWindow;
    uint32_t id;
    void * hLanguageSubscriber;
    uint8_t field_0x10;
    uint8_t field_0x11;
    uint8_t field_0x12;
    uint8_t field_0x13;
    void * hTerminateSubscriber;
    uint32_t field_0x18;
    uint32_t field_0x1c;
    uint32_t field_0x20;
    uint32_t field_0x24;
    uint32_t field_0x28;
    uint32_t field_0x2c;
    uint32_t field_0x30;
    uint32_t field_0x34;
    uint32_t field_0x38;
    void * controller;
    uint8_t field_0x40;
    uint8_t field_0x41;
    uint8_t field_0x42;
    uint8_t field_0x43;
    uint32_t field_0x44;
    uint8_t field_0x48;
    uint8_t field_0x49;
    uint8_t field_0x4a;
    uint8_t field_0x4b;
    uint8_t field_0x4c;
    uint8_t field_0x4d;
    uint8_t field_0x4e;
    uint8_t field_0x4f;
    uint8_t field_0x50;
    uint8_t field_0x51;
    uint8_t field_0x52;
    uint8_t field_0x53;
    uint8_t field_0x54;
    uint8_t field_0x55;
    uint8_t field_0x56;
    uint8_t field_0x57;
    uint8_t field_0x58;
    uint8_t field_0x59;
    uint8_t field_0x5a;
    uint8_t field_0x5b;
    uint32_t * field_0x5c;
    uint32_t * field_0x60;
    void * handler;
    void * handler_arg;
    uint32_t field_0x6c;
    uint16_t field_0x70;
    uint16_t field_0x72;
    uint16_t field_0x74;
    uint16_t field_0x76;
    uint32_t field_0x78;
    uint32_t field_0x7c;
    uint32_t field_0x80;
    uint8_t field_0x84;
    uint8_t field_0x85;
    uint8_t field_0x86;
    uint8_t field_0x87;
    uint32_t field_0x88;
    uint32_t color_related_1;
    uint32_t unknown_region_x;
    uint32_t unknown_region_y;
    uint32_t unknown_region_w;
    uint32_t unknown_region_h;
    uint32_t color_related_2;
    uint16_t field_0xa4;
    uint8_t field_0xa6;
    uint8_t field_0xa7;
    void * child_list_maybe;
    int child_list_count_maybe;
    uint32_t field_0xb0;
    uint32_t field_0xb4;
    uint32_t field_0xb8;
    uint32_t field_0xbc;
    uint32_t field_0xc0;
    uint32_t field_0xc4;
    uint32_t field_0xc8;
    uint32_t field_0xcc;
    uint32_t field_0xd0;
    uint16_t field_0xd4;
    uint8_t field_0xd6;
    uint8_t field_0xd7;
    uint8_t field_0xd8;
    uint8_t field_0xd9;
    uint8_t field_0xda;
    uint8_t field_0xdb;
    uint32_t field_0xdc;
    uint32_t field_0xe0;
    uint16_t field_0xe4;
    uint8_t field_0xe6;
    uint8_t field_0xe7;
    uint32_t field_0xe8;
    uint32_t field_0xec;
    uint32_t field_0xf0;
    uint32_t field_0xf4;
    uint32_t field_0xf8;
    uint32_t field_0xfc;
    uint32_t field_0x100;
    uint32_t field_0x104;
    uint32_t field_0x108;
    uint32_t field_0x10c;
};
#else
struct dialog
{
        const char *            type;                   // "DIALOG" at 0x147F8
        struct window *         window;                 // off 0x04
        void *                  arg0;                   // off 0x08
        struct langcon *        langcon;                // off 0x0c
        struct dispsw_control * disp_sw_controller;     // off 0x10
        struct publisher *      publisher;              // off 0x14
        uint32_t                off_0x18;
        uint32_t                off_0x1c;
        uint32_t                off_0x20;               // initial=0
        uint32_t                off_0x24;               // initial=0
        uint32_t                off_0x28;               // initial=2
        uint32_t                off_0x2c;               // initial=0
        uint32_t                off_0x30;               // initial=0
        uint32_t                flag_0x34;              // initial=0, set to 1 by
        uint32_t                off_0x38;               // initial=0
        struct task *           gui_task;               // off 0x3c
        uint32_t                off_0x40;               // pointed to by 0x08
        uint32_t                off_0x44;               // initial=0
        uint32_t                off_0x48;
        uint32_t                off_0x4c;
        uint32_t                off_0x50;
        uint32_t                off_0x54;
        uint32_t                off_0x58;
        void *                  child_list_maybe;       // off_0x5c;
        dialog_handler_t        handler;                // off 0x60
        uint32_t                handler_arg;            // off_0x64;
        uint32_t                off_0x68;               // initial=0
        uint16_t                off_0x6c;               // initial=0
        uint16_t                off_0x6e;               // initial=0
        uint16_t                off_0x70;
        uint16_t                off_0x72;               // initial=0
        uint32_t                off_0x74;               // initial=1
        uint32_t                off_0x78;               // initial=0
        uint32_t                off_0x7c;               // initial=0
        struct publisher *      publisher_callback;     // off_0x80;
        uint32_t                off_0x84;
        uint32_t                const_40000000_0;       // off_0x88;
        uint32_t                off_0x8c;               // initial=0xA
        uint32_t                off_0x90;               // initial=0xA
        uint32_t                id;                     // off_0x94;
        uint32_t                level_maybe;            // off_0x98;
        uint32_t                const_40000000_1;       // off_0x9c;
        uint16_t                off_0xa0;               // initial=0
        uint16_t                off_0xa2;
        uint32_t                off_0xa4;
        uint32_t                off_0xa8;               // initial=0
        uint32_t                off_0xac;               // initial=0
};

SIZE_CHECK_STRUCT( dialog, 0xB0 );
#endif
/*
 * kitor: New generations call it WINSYS_CreateDialogBox()
 * EOS R has one more param at the end. Unconfirmed guess: z-index
 */
extern struct dialog *
dialog_create(
        int                     id,      // must be unique?
        int                     level_maybe,
        dialog_handler_t        handler,
        void *                  arg1,
        void *                  arg2
);

extern void
dialog_delete(
        struct dialog *         dialog
);

extern void
dialog_draw(
        struct dialog *         dialog
);

extern void
dialog_window_draw(
        struct dialog *         dialog
);

extern void
dialog_window_prepare(
        struct dialog *         dialog,
        void *                  unused
);

extern void
dialog_post_event(
        unsigned                event,
        unsigned                arg,
        struct dialog *         dialog
);

extern void
dialog_set_focus(
        struct dialog *         dialog
);


/** type 0 == 720, 1 == 960? */
extern void
dialog_set_origin_type(
        struct dialog *         dialog,
        int                     type
);

extern void
dialog_resize(
        struct dialog *         dialog,
        int                     w,
        int                     h,
        int                     unknown
);

extern void
dialog_window_resize(
        struct dialog *         dialog,
        int                     w,
        int                     h,
        int                     unknown
);

extern void
dialog_move(
        struct dialog *         dialog,
        int                     x,
        int                     y
);

extern void
dialog_move_item(
        struct dialog *         dialog,
        int                     x,
        int                     y,
        int                     index
);

extern void
dialog_label_item(
        struct dialog *         dialog,
        uint32_t                id,
        const char *            label,
        int                     len_maybe,
        int                     unknown
);


/** Top level dialog handler (gui_task) */
extern void
dialog_handler( void );

/* returns Canon's dialog handler used right now */
void* get_current_dialog_handler();

/* Canon stub */
void dialog_redraw(struct dialog * dialog);

void dialog_set_property_str(struct dialog * dialog, int string_id, char* msg);

#endif
