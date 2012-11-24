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

#include "arm-mcr.h"

typedef uint32_t gui_event_t; // not used

/** Windowing system elements */
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
struct window
{
        const char *            type; // "Window Instance" at 0x14920
        uint32_t                off_0x04;       // initial=0
        uint32_t                off_0x08;       // initial=0
        uint32_t                off_0x0c;       // initial=0
        uint32_t                x;              // off_0x10;
        uint32_t                y;              // off_0x14;
        uint32_t                width;          // off_0x18; r5
        uint32_t                height;         // off_0x1c; r6
        window_callback         callback;       // off 0x20
        void *                  arg;            // off_0x24;
        uint32_t                wx_maybe;       // off_0x28;
        uint32_t                wy_maybe;       // off_0x2c;
};

SIZE_CHECK_STRUCT( window, 0x30 );

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


#endif
