
/** \file
 * Bitrate
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"
#include "mvr.h"


#if defined(FEATURE_VIDEO_HACKS)

#include "cache_hacks.h"

uint32_t bitrate_cache_hacks = 0;
uint32_t bitrate_flushing_rate = 4;
uint32_t bitrate_gop_size = 12;

#if defined(CONFIG_7D)
#include "ml_rpc.h"
#endif

static MENU_UPDATE_FUNC(bitrate_flushing_rate_update)
{
    MENU_SET_VALUE("%d frames", bitrate_flushing_rate );

#if defined(CONFIG_7D)
    if(!ml_rpc_available())
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Master DIGiC hacks not available in this release.");
    }
    else 
#endif
    if(!bitrate_cache_hacks)
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Video hacks disabled.");
    }
}

static MENU_UPDATE_FUNC(bitrate_gop_size_update)
{
    MENU_SET_VALUE("%d frames", bitrate_gop_size);

#if defined(CONFIG_7D)
    if(!ml_rpc_available())
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Master DIGiC hacks not available in this release.");
    }
    else
#endif
    if(!bitrate_cache_hacks)
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Video hacks disabled.");
    }
}

static void
video_hack_task( void* unused )
{
    uint32_t old_bitrate_cache_hacks = 0;
    uint32_t old_bitrate_gop_size = 0;
    uint32_t old_bitrate_flushing_rate = 0;

    TASK_LOOP
    {
#if defined(CONFIG_7D)
        if(ml_rpc_available())
#endif
        {
            /* anything changed? */
            if(bitrate_cache_hacks != old_bitrate_cache_hacks || bitrate_flushing_rate != old_bitrate_flushing_rate || bitrate_gop_size != old_bitrate_gop_size)
            {
                if(bitrate_cache_hacks)
                {
                    /* patch flushing rate */
#if defined(CACHE_HACK_FLUSH_RATE_SLAVE)
                    cache_fake(CACHE_HACK_FLUSH_RATE_SLAVE, 0xE3A00000 | (bitrate_flushing_rate & 0xFF), TYPE_ICACHE);
#endif
#if defined(CACHE_HACK_FLUSH_RATE_MASTER)
                    ml_rpc_send(ML_RPC_CACHE_HACK, CACHE_HACK_FLUSH_RATE_MASTER, 0xE3A01000 | (bitrate_flushing_rate & 0xFF), TYPE_ICACHE, 2);
#endif
                    /* set GOP size */
#if defined(CACHE_HACK_GOP_SIZE_SLAVE)
                    cache_fake(CACHE_HACK_GOP_SIZE_SLAVE, 0xE3A00000 | (bitrate_gop_size & 0xFF), TYPE_ICACHE);
#endif
#if defined(CACHE_HACK_GOP_SIZE_MASTER)
                    ml_rpc_send(ML_RPC_CACHE_HACK, CACHE_HACK_GOP_SIZE_MASTER, 0xE3A01000 | (bitrate_gop_size & 0xFF), TYPE_ICACHE, 2);
#endif
                    /* make sure canon sound is disabled */
                    // todo: enable it back, as in fps-engio.c
                    // this code won't work on 5D2 and 50D
                    int mode  = 1;
                    prop_request_change(PROP_MOVIE_SOUND_RECORD, &mode, 4);
                    NotifyBox(2000,"Canon sound disabled");
                }
                else
                {
                    /* undo flushing rate */
#if defined(CACHE_HACK_FLUSH_RATE_SLAVE)
                    cache_fake(CACHE_HACK_FLUSH_RATE_SLAVE, MEM(CACHE_HACK_FLUSH_RATE_SLAVE), TYPE_ICACHE);
#endif
#if defined(CACHE_HACK_FLUSH_RATE_MASTER)
                    ml_rpc_send(ML_RPC_CACHE_HACK_DEL, CACHE_HACK_FLUSH_RATE_MASTER, TYPE_ICACHE, 0, 2);
#endif
                    /* undo GOP size */   
#if defined(CACHE_HACK_GOP_SIZE_SLAVE)
                    cache_fake(CACHE_HACK_GOP_SIZE_SLAVE, MEM(CACHE_HACK_GOP_SIZE_SLAVE), TYPE_ICACHE);
#endif                 
#if defined(CACHE_HACK_GOP_SIZE_MASTER)
                    ml_rpc_send(ML_RPC_CACHE_HACK_DEL, CACHE_HACK_GOP_SIZE_MASTER, TYPE_ICACHE, 0, 2);
#endif
                }
                
                old_bitrate_cache_hacks = bitrate_cache_hacks;
                old_bitrate_gop_size = bitrate_gop_size;
                old_bitrate_flushing_rate = bitrate_flushing_rate;
            }
        }

        msleep(250);
    }
}

static struct menu_entry video_hack_menus[] = {
    {
        .name = "Video Hacks",
        .priv = &bitrate_cache_hacks,
        .max  = 1,
        .help = "Experimental hacks: flush rate, GOP size. Be careful!",
        .depends_on = DEP_MOVIE_MODE,
        .children =  (struct menu_entry[]) {
            {
                .name = "Flush rate",
                .priv = &bitrate_flushing_rate,
                .update = bitrate_flushing_rate_update,
                .min  = 2,
                .max  = 50,
                .help = "Flush movie buffer every n frames."
            },
            {
                .name = "GOP size",
                .priv = &bitrate_gop_size,
                .update = bitrate_gop_size_update,
                .min  = 1,
                .max  = 100,
                .help = "Set GOP size to n frames."
            },
            MENU_EOL
        },
    },
};

void video_hack_init()
{
    cache_lock();
    menu_add( "Movie", video_hack_menus, COUNT(video_hack_menus) );
}

INIT_FUNC(__FILE__, video_hack_init);
TASK_CREATE("video_hack_task", video_hack_task, 0, 0x1d, 0x1000 );

#endif
