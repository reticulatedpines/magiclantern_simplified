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
#ifdef CONFIG_DIGIC_678
// Verified on 750D (D6), 200D (D7), M50, R, RP, 250D (D8)
struct dialog {
    char * type;                                        // Signature, pointer to "DIALOG" string
    struct window *          window;
    uint32_t                 id;                        // 4th arg to CreateDialogBox, referenced as ID in some prints
    struct subscriberClass * hLanguageSubscriber;
#if defined(CONFIG_DIGIC_VI) || defined(CONFIG_DIGIC_VII)
    uint32_t                 unk_d67_01;                // Not initialized in CreateDialogBox; type may be wrong
#endif
    struct subscriberClass * hTerminateSubscriber;      // disp_sw_controller?
    uint32_t                 refresh_x;                 // Region to redraw? probably a "damaged" region.
    uint32_t                 refresh_y;
    uint32_t                 refresh_w;
    uint32_t                 refresh_h;
    uint32_t                 origin_maybe;              // not sure, referenced as origin in messages, may be resolution related ID
    uint32_t                 pos_x;                     // Region of window onscreen position?
    uint32_t                 pos_y;
    uint32_t                 pos_w;
    uint32_t                 pos_h;
#ifdef CONFIG_DIGIC_VIII
    uint32_t                 flag_1;                    // Set either 0 or 1. Defaults to 0.
    uint32_t                 flag_2;                    // Set either 0 or 1. Defaults to 0.
#endif // CONFIG_DIGIC_VIII
    struct gui_task *        controller;                // CtrlServ object. We call it gui_task
    void *                   dlgItem_related_1;
    uint32_t                 unk_01;
#if defined(CONFIG_DIGIC_VI) || defined(CONFIG_DIGIC_VII)
    uint8_t                  unk_d67_02[20];            // Not initialized in CreateDialogBox; Compacted to array, type surely wrong.
#endif // defined(CONFIG_DIGIC_VI) || defined(CONFIG_DIGIC_VII)
    void *                   dlgItem_related_2;         // dlgItem_related_2 and _3 are set to dlgItem_related_1 in CreateDialogBox
    void *                   dlgItem_related_3;         // Referenced in TerminateDialogBox, item[1] passed to DestroyDialogItem.
    void *                   handler;                   // Pointer to CBR
    void *                   handler_arg;               // Arg for CBR
    uint32_t                 unk_02;                    // Set in WINSYS_GetFocusedItemIDOfDialogItem.
    uint16_t                 short_x;                   // Another region. This one expressed in shorts insted of uint32_t
    uint16_t                 short_y;                   // region may be connected with resources loading, called from ResLoad function
    uint16_t                 short_w;                   // where it returns XYWH to resource structure.
    uint16_t                 short_h;
    uint32_t                 unk_03;
    uint32_t                 unk_04;
    uint32_t                 unk_05;
    uint8_t                  unk_06;                    // Not initialized in CreateDialogBox; type may be wrong
    uint8_t                  unk_07;                    // ^ like above
    uint8_t                  unk_08;                    // ^ like above
    uint8_t                  unk_09;                    // ^ like above
    uint32_t                 unk_10;
    uint32_t                 color_related_1;           // Set to 0x40000000 in CreateDialogBox. Seems to be some kind of bitmask, color related.
    uint32_t                 some_x;                    // Another region, unknown meaning
    uint32_t                 some_y;                    // Set by default to 0,0,arg1,arg2 in CreateDialogBox
    uint32_t                 some_w;
    uint32_t                 some_h;
    uint32_t                 color_related_2;           // See color_related_1
    uint16_t                 unk_11;
    uint8_t                  unk_12;
    uint8_t                  unk_13;
    void *                   child_list;                // Pointer to buffer of (sizeof(ptr) * child_list_count_maybe)
    uint                     child_list_count;          // "search table length %d"; +2 from "real" size - see CreateDialogBox
    uint32_t                 reaction_x;                // Region. See SetReactionAreaToDialog
    uint32_t                 reaction_y;
    uint32_t                 reaction_w;
    uint32_t                 reaction_h;
    uint32_t                 unk_14;
    uint32_t                 unk_15;
    uint32_t                 unk_16;
    uint32_t                 unk_17;
    uint32_t                 unk_18;
    uint16_t                 unk_19;
    uint8_t                  unk_20;                    // Not initialized in CreateDialogBox; type may be wrong
    uint8_t                  unk_21;                    // ^ like above
    uint8_t                  unk_22;                    // ^ like above
    uint8_t                  unk_23;                    // ^ like above
    uint8_t                  unk_24;                    // ^ like above
    uint8_t                  unk_25;                    // ^ like above
    uint32_t                 unk_26;
    uint32_t                 unk_27;
    uint16_t                 unk_28;
    uint8_t                  unk_29;                    // Not initialized in CreateDialogBox; type may be wrong
    uint8_t                  unk_30;                    // ^ like above
    uint32_t                 unk_31;
    uint32_t                 unk_32;
    uint32_t                 unk_33;
    uint32_t                 unk_34;
    uint32_t                 unk_35;
    uint32_t                 unk_36;
    uint32_t                 unk_37;
    uint32_t                 unk_38;
    uint32_t                 unk_39;
    uint32_t                 unk_40;
#ifdef CONFIG_DIGIC_VIII                                // DIGIC8
    uint32_t                 _refresh_x;                // Region used as arguments passed to WINSYS_RegisterRefreshRectangle_maybe
    uint32_t                 _refresh_y;                // which updates refresh_[xywh] conditionally from those values.
    uint32_t                 _refresh_w;
    uint32_t                 _refresh_h;
    uint32_t                 unk_d8_01;
    uint32_t                 unk_d8_02;
    uint32_t                 unk_d8_03;
    uint32_t                 rotationAngle;             // Guess: GUI can render rotated 90 degrees (EVF)
    //uint32_t               field_0x122;               // Those two exists on RP and later. We don't use them, and
    //uint32_t               field_0x124;               // do not create struct so I left them commented out.
#endif // CONFIG_DIGIC_VIII
};

#ifdef CONFIG_DIGIC_VIII
// RP, 250D is 0x128, but we left two commented out as they are not needed
SIZE_CHECK_STRUCT( dialog, 0x120 );
#else
// verified on 750D, 200D
SIZE_CHECK_STRUCT( dialog, 0x110 );
#endif

#else // =< DIGIC5
// kitor: looks a bit similar to D67. By comparing with 5D3 I fixed a few mistakes.
struct dialog
{
        const char *            type;                   // "DIALOG" at 0x147F8
        struct window *         window;                 // off 0x04
        void *                  id;                     // off 0x08
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
        uint32_t                color_related_1;        // Set to 0x40000000 in CreateDialogBox. Seems to be some kind of bitmask, color related.
        uint32_t                some_x;                 // Another region, unknown meaning
        uint32_t                some_y;                 // Set by default to 0,0,arg1,arg2 in CreateDialogBox
        uint32_t                some_w;
        uint32_t                some_h;
        uint32_t                color_related_2;        // See color_related_1
        uint16_t                off_0xa0;               // initial=0
        uint16_t                off_0xa2;
        uint32_t                off_0xa4;
        uint32_t                off_0xa8;               // initial=0
        uint32_t                off_0xac;               // initial=0
};

SIZE_CHECK_STRUCT( dialog, 0xB0 );
#endif

/*
 * DIGIC8+ naming is WINSYS_CreateDialogBox
 */
extern struct dialog *
dialog_create(
        int                     width,         // region set as 10,10,width,height
        int                     height,        // on dialog init
        dialog_handler_t        handler,
        int                     id,
        void *                  handler_arg
);

/*
 * Models that use multiple GUI layers have additional parameter with layer ID.
 * Those also have a wrapper with hardcoded layer 0 (equiv to dialog_create),
 * I guess for compatibility reasons (so all not-layer-aware code can run)
 *
 * On R/RP layer 1 is used just to draw focus overlays while in LV mode, only
 * one app makes use of it.
 */
extern struct dialog *
dialog_create_ex(
        int                     width,         // region set as 10,10,width,height
        int                     height,        // on dialog init
        dialog_handler_t        handler,
        int                     id,
        void *                  handler_arg,
        int                     layer_id       // GUI layer to draw window on
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
