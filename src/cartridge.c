/** 
 * Cartridge hooks (used for HDR video).
 * 
 * If you use this file, don't use fps.c (use fps-engio.c instead).
 * 
 * Credits goto g3gg0
 **/

#include "math.h"
#include "dryos.h"
#include "bmp.h"
#include "property.h"
#include "menu.h"
#include "lens.h"
#include "config.h"

#ifdef CONFIG_600D
#define CARTIRIDGE_CALL_TABLE 0x8AAC
#endif
#ifdef CONFIG_60D
#define CARTIRIDGE_CALL_TABLE 0x26490
#endif
#ifdef CONFIG_1100D
#define CARTIRIDGE_CALL_TABLE 0x8B24
#endif

#define HOOK_TABLE_FUNCTION(table,position,new,old) \
    old = (void*)((unsigned int*)table)[position];\
    ((unsigned int*)table)[position] = (unsigned int)new;\

#define UNHOOK_TABLE_FUNCTION(table,position,old) \
    ((unsigned int*)table)[position] = (unsigned int)old;\

#define IS_TABLE_FUNCTION_HOOKED(table,position,new) \
    (((unsigned int*)table)[position] == (unsigned int)new) \

#define REDIRECT_BUFFER(address,buffer) \
    memcpy(buffer, (unsigned char*)(*((unsigned int *)(address))), sizeof(buffer));\
    *((unsigned int *)(address)) = (unsigned int)buffer;\

static char cartridge_table[0xF4];
static void (*cartridge_AfStopPathReal)(void *this) = NULL;

static void cartridge_AfStopPath(void *this)
{
    /* change ISO for HDR movie (will only work if HDR is enabled) */
    hdr_step();

    /* call hooked function */
    cartridge_AfStopPathReal(this);
}

// Q: what happes if this is called when Canon firmware flips the resolution?
static void update_cartridge_table()
{
    extern int hdrv_enabled;
    if (hdrv_enabled)
    {
        // cartridge call table is sometimes overriden by Canon firmware
        // so this function polls the status periodically (and updates it if needed)
        if (!is_cartridge_table_overridden())
        {
            /* first clone the cartridge call table */
            REDIRECT_BUFFER(CARTIRIDGE_CALL_TABLE, cartridge_table);

            /* now hook the function Af_StopPath in cloned cartridge_table */
            HOOK_TABLE_FUNCTION(cartridge_table, 0x39, cartridge_AfStopPath, cartridge_AfStopPathReal);
            //~ beep();
            
            lens_display_set_dirty();
       }
    }
    else
    {
        if (is_cartridge_table_overridden() && cartridge_AfStopPathReal)
        {
            UNHOOK_TABLE_FUNCTION(cartridge_table, 0x39, cartridge_AfStopPathReal);
            cartridge_AfStopPathReal = NULL;
            //~ beep();

            lens_display_set_dirty();
        }
    }
}

int is_cartridge_table_overridden()
{
    return MEM(CARTIRIDGE_CALL_TABLE) == (int)cartridge_table &&
        IS_TABLE_FUNCTION_HOOKED(cartridge_table, 0x39, cartridge_AfStopPath);
}


static void cartridge_task()
{
    TASK_LOOP
    {
        if (lv)
        {
            update_cartridge_table();
        }
        msleep(200);
    }
}

TASK_CREATE("cartridge_task", cartridge_task, 0, 0x1d, 0x1000 );
