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
#define SENSOR_TIMING_TABLE MEM(0xCB20)
#define VIDEO_PARAMETERS_SRC_3 0x70AE8 // notation from g3gg0
#define CARTIRIDGE_CALL_TABLE 0x8AAC
#define AEWB_struct_ptr 0x1dcc
#endif
#ifdef CONFIG_60D
#define SENSOR_TIMING_TABLE MEM(0x2a668)
#define VIDEO_PARAMETERS_SRC_3 0x4FDA8
#define AEWB_struct_ptr 0x1E80
#define CARTIRIDGE_CALL_TABLE 0x26490
#endif
#ifdef CONFIG_1100D
#define SENSOR_TIMING_TABLE MEM(0xce98)
#define VIDEO_PARAMETERS_SRC_3 0x70C0C
#define CARTIRIDGE_CALL_TABLE 0x8B24
#endif

static uint16_t * sensor_timing_table_original = 0;
static uint16_t sensor_timing_table_patched[175*2];

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
static void update_hard_expo_override()
{
    extern int hdrv_enabled;
    if (hdrv_enabled)
    {
        // cartridge call table is sometimes overriden by Canon firmware
        // so this function polls the status periodically (and updates it if needed)
        if (!is_hard_exposure_override_active())
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
        if (is_hard_exposure_override_active() && cartridge_AfStopPathReal)
        {
            UNHOOK_TABLE_FUNCTION(cartridge_table, 0x39, cartridge_AfStopPathReal);
            cartridge_AfStopPathReal = NULL;
            //~ beep();

            lens_display_set_dirty();
        }
    }
}

int is_hard_exposure_override_active()
{
    return MEM(CARTIRIDGE_CALL_TABLE) == (int)cartridge_table &&
        IS_TABLE_FUNCTION_HOOKED(cartridge_table, 0x39, cartridge_AfStopPath);
}

static void cartridge_init()
{
    // make a copy of the original sensor timing table (so we can patch it)
    sensor_timing_table_original = (void*)SENSOR_TIMING_TABLE;
    memcpy(sensor_timing_table_patched, sensor_timing_table_original,  sizeof(sensor_timing_table_patched));
}

INIT_FUNC("cartridge", cartridge_init);


static void cartridge_task()
{
    TASK_LOOP
    {
        if (lv)
        {
            update_hard_expo_override();
        }
        msleep(200);
    }
}

TASK_CREATE("cartridge_task", cartridge_task, 0, 0x1d, 0x1000 );
