/*
 * Copyright (C) 2013 Magic Lantern Team
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
 
#ifndef __MLV_REC_INTERFACE_H__
#define __MLV_REC_INTERFACE_H__


/* interface to register callbacks for various stuff */
typedef void (*event_cbr_t) (uint32_t event, void *ctx, mlv_hdr_t *hdr);

/* event types, can be combined */
#define MLV_REC_EVENT_STARTING    (1U<<0) /* gets called when recording is going to be started */
#define MLV_REC_EVENT_STARTED     (1U<<1) /* gets called when recording was started and blocks are being written */
#define MLV_REC_EVENT_STOPPING    (1U<<2) /* gets called when going to stop recoding for whatever reason. good point to finally push some data before files are closed */
#define MLV_REC_EVENT_STOPPED     (1U<<3) /* gets called when recording has stopped */
#define MLV_REC_EVENT_CYCLIC      (1U<<4) /* gets called whenever the cyclic RTCI block is written (usually 2 second interval) */
#define MLV_REC_EVENT_BLOCK       (1U<<5) /* gets called for every block before it is being written to the file. 'hdr' parameter will contain a pointer to the the block. might get called multiple times per block! */
#define MLV_REC_EVENT_VIDF        (1U<<6) /* gets called for every VIDF being queued for write. called from EDMAC CBR, so avoid using too much CPU time. */
#define MLV_REC_EVENT_PREPARING   (1U<<7) /* gets called before any buffer allocation or LV manipulation */


#if !defined(__MLV_REC_C__) && !defined(__MLV_LITE_C__)

/* register function as callback routine for all events ORed into 'event' and call that CBR with given 'ctx' */
extern WEAK_FUNC(ret_0) uint32_t mlv_rec_register_cbr(uint32_t event, event_cbr_t cbr, void *ctx);

/* unregister a previously registered CBR for any event */
extern WEAK_FUNC(ret_0) uint32_t mlv_rec_unregister_cbr(event_cbr_t cbr);

/* queue a MLV block for writing. timestamp will get set automatically as it requires knowledge of the absolute record starting time */
extern WEAK_FUNC(ret_0) uint32_t mlv_rec_queue_block(mlv_hdr_t *hdr);

#else
    
/* structure entry for registered CBR routines */
typedef struct
{
    uint32_t event;
    void *ctx;
    event_cbr_t cbr;
} cbr_entry_t;

#endif

#endif
