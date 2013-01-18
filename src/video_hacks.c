
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

static void
bitrate_cache_hacks_display( void * priv, int x, int y, int selected )
{
    bmp_printf(selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Video hacks   : %s", 
               bitrate_cache_hacks ? "ON" : "OFF"
               );

#if defined(CONFIG_7D)
    if(!ml_rpc_available())
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Master DIGiC hacks not available in this release.");
    }
    else
#endif
    {
        menu_draw_icon(x, y, MNI_ON, 0);
    }
}

static void
bitrate_flushing_rate_display( void * priv, int x, int y, int selected )
{
    bmp_printf(selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "Flush every   : %d frames", 
               bitrate_flushing_rate
               );

#if defined(CONFIG_7D)
    if(!ml_rpc_available())
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Master DIGiC hacks not available in this release.");
    }
    else 
#endif
    if(!bitrate_cache_hacks)
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Video hacks disabled.");
    }
    else
    {
        menu_draw_icon(x, y, MNI_ON, 0);
    }
}

static void
bitrate_gop_size_display( void * priv, int x, int y, int selected )
{
    bmp_printf(selected ? MENU_FONT_SEL : MENU_FONT,
               x, y,
               "GOP size      : %d frames", 
               bitrate_gop_size
               );

#if defined(CONFIG_7D)
    if(!ml_rpc_available())
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Master DIGiC hacks not available in this release.");
    }
    else
#endif
    if(!bitrate_cache_hacks)
    {
        menu_draw_icon(x, y, MNI_WARNING, (intptr_t) "Video hacks disabled.");
    }
    else
    {
        menu_draw_icon(x, y, MNI_ON, 0);
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
        .help = "Change video recording characteristics. Be careful!",
        .select = menu_open_submenu,
        .children =  (struct menu_entry[]) {
            {
                .name = "Enable",
                .priv = &bitrate_cache_hacks,
                .display = bitrate_cache_hacks_display,
                .max  = 1,
                .help = "Enable experimental hacks through cache hacks."
            },
            {
                .name = "Flush rate",
                .priv = &bitrate_flushing_rate,
                .display    = bitrate_flushing_rate_display,
                .min  = 2,
                .max  = 50,
                .help = "Flush movie buffer every n frames."
            },
            {
                .name = "GOP size",
                .priv = &bitrate_gop_size,
                .display    = bitrate_gop_size_display,
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