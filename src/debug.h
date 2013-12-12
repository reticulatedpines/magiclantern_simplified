#ifndef _debug_h_
#define _debug_h_

#include <stdbool.h>

/** \file
 * Debug macros and functions.
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


/** Debug messages and debug manager */
extern void
DebugMsg(
        int                     subsys,
        int                     level,
        const char *            fmt,
        ...
);


extern const char * dm_names[];

// To find these, look at the dm_names table at 0x292C
// Subtract the pointer from 0x292c and divide by 4
#define DM_SETPROP      0
#define DM_PRP          1
#define DM_PROPAD       2
#define DM_INTCOM       3
#define DM_WINSYS       4
#define DM_CTRLSRV      5
#define DM_LVCAF        6
#define DM_LVAF         7
#define DM_LVCAE        8
#define DM_LVAE         9
#define DM_LVFD         10
#define DM_LVMD         11
#define DM_LVCLR        12
#define DM_LVWB         13
#define DM_DP           14
#define DM_MAC          15
#define DM_CRP          16
#define DM_UPDC         17
#define DM_SYS          18
#define DM_RD           19
#define DM_AUDIO        20
#define DM_ENGP         0x16
#define DM_MOVR         48
#define DM_MAGIC        50 // Replaces PTPCOM with MAGIC
#define DM_LVCDEV       61
#define DM_RSC          0x80
#define DM_DISP         64
#define DM_LV           86
#define DM_LVCFG        89
#define DM_GUI          131
#define DM_GUI_M        132
#define DM_GUI_E        133
#define DM_BIND         137

struct dm_state
{
        const char *            type; // off_0x00
        uint32_t                off_0x04;
        uint32_t                off_0x08;
        uint32_t                off_0x0c;
        void *                  signature; // off_0x10
        uint32_t                unknown[ (788 - 0x14)/4 ];
};

extern struct dm_state * dm_state_ptr;
extern struct state_object * dm_state_object;
extern void dmstart( void ); // post the start event
extern void dmStart( void ); // initiate the start
extern void dmstop( void );
extern void dumpentire( void );
extern void dumpf( void );
extern void dm_set_store_level( uint32_t class, uint32_t level );

extern void card_led_on();
extern void card_led_off();
extern bool get_halfshutter_pressed();

extern void
dm_event_dispatch(
        int                     input,
        int                     dwParam,
        int                     dwEventId
);

extern void
debug_init( void );

void debug_init_stuff( void );

void request_crash_log(int type);

char* get_config_preset_name(void);

#define DEBUG_LOG_THIS(x) {     \
dm_set_print_level(255,0); dm_set_store_level(255,0); \
dm_set_print_level(21,30); dm_set_store_level(21,30); \
dmstart(); msleep(100); DryosDebugMsg(DM_MAGIC, 25, ">>>>>>>>>>>>>>"); \
x; \
DryosDebugMsg(DM_MAGIC, 26, "<<<<<<<<<<<<<<<"); \
msleep(100); dmstop(); call("dumpf"); }

#endif
