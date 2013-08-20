/*
based on original mem_spy in debug.c
*/

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <menu.h>

#define TYPE_BOOL 0
#define TYPE_INT8 1
#define TYPE_INT16 2
#define TYPE_INT32 3
#define TYPE_FLOAT 4

const int addresses[] = {}; //not tested

static CONFIG_INT("mem.spy.enabled", mem_spy, 0);
static CONFIG_INT("mem.spy.look_for", look_for, TYPE_INT32);
static CONFIG_INT("mem.spy.halfshutter_related", halfshutter_related, 0);
static CONFIG_INT("mem.spy.fixed_addresses", fixed_addresses, 0);
static CONFIG_INT("mem.spy.start_addr", start_addr, 0);
static CONFIG_INT("mem.spy.var_count", var_count, 50000);
static CONFIG_INT("mem.spy.value_lo", value_lo, 0);
static CONFIG_INT("mem.spy.value_hi", value_hi, 0);
static CONFIG_INT("mem.spy.count_lo", count_lo, 10);
static CONFIG_INT("mem.spy.count_hi", count_hi, 250);
static CONFIG_INT("mem.spy.freq_lo", freq_lo, 0);
static CONFIG_INT("mem.spy.freq_hi", freq_hi, 0);
static CONFIG_INT("mem.spy.start_delay", start_delay, 3);

static int* mem_mirror = 0;
static int* mem_changes = 0;
static int* mem_position = 0;

static int mem_spy_running = 0;
static int init_done = 0;
static int var_length = 4;
static int start_delay_counter;

#define COLUMN_COUNT 3
#define COLUMN_WIDTH 720 / COLUMN_COUNT
#define FONT_HEIGHT 12
#define POSITION_COUNT 480 / FONT_HEIGHT * COLUMN_COUNT

static int position[POSITION_COUNT][3];

static void init_position(){
    int x = 0;
    int y = 0;
    int i = 0;
    for(; i < POSITION_COUNT; i++) {
        position[i][0] = x;
        position[i][1] = y;
        position[i][2] = 1;
        
        y += FONT_HEIGHT;
        if(y > 480 - FONT_HEIGHT) {
            x += COLUMN_WIDTH;
            y = 0;
        }
    }
}

static int next_position() {
    int i;
    for(i = 0; i < POSITION_COUNT; i++) {
        if(position[i][2] == 1)return i;
    }
    return -1;
}

static int get_addr(int i)
{
    if (fixed_addresses) return addresses[i];
    else return start_addr + i * var_length;
}

int _t = 0;
static int _get_timestamp(struct tm * t)
{
    return t->tm_sec + t->tm_min * 60 + t->tm_hour * 3600 + t->tm_mday * 3600 * 24;
}
static void _tic()
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    _t = _get_timestamp(&now);
}
static int _toc()
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    return _get_timestamp(&now) - _t;
}

static int init_mem() // initial state of the analyzed memory
{
    // local copy of mem area analyzed
    if (!mem_mirror) mem_mirror = SmallAlloc(var_count * var_length + 100);
    if (!mem_mirror) return 0;
    
    // store changes
    if (!mem_changes) mem_changes = SmallAlloc(var_count * var_length + 100);
    if (!mem_changes) return 0;
    
    // store position
    if (!mem_position) mem_position = SmallAlloc(var_count * var_length + 100);
    if (!mem_position) return 0;
    
    int i;
    for (i = 0; i < var_count; i++) {
        uint32_t addr = get_addr(i);
        mem_mirror[i] = MEM(addr);
        mem_changes[i] = 0;
        mem_position[i] = -1;
    }
    
    _tic();
    
    return 1;
}

static int get_byte_length() {
    switch(look_for) {
        case TYPE_BOOL: return sizeof(bool);
        case TYPE_INT8: return sizeof(int8_t);
        case TYPE_INT16: return sizeof(int16_t);
        case TYPE_INT32: return sizeof(int32_t);
        case TYPE_FLOAT: return sizeof(float);
    }
    return 1;
}

static void mem_spy_task()
{
    mem_spy_running = 1;
    
    int i;
    TASK_LOOP
    {
        if(!mem_spy)break;
        
        if(start_delay_counter != 0){
            NotifyBox(1000, "spy in %d s", start_delay_counter);
            start_delay_counter--;
            msleep(1000);
            continue;
        }
        
        if (!init_done) {
            if(!init_mem()) {
                NotifyBox(1000, "SmallAlloc failed");
                break;
            }
            init_done = 1;
        }
    
        int elapsed_time = _toc();
        
        bmp_printf(FONT_SMALL, 600, 0, "%ds", elapsed_time);
        
        for (i = 0; i < var_count; i++)
        {
            if(mem_changes[i] == -1)continue;
            
            uint32_t addr = get_addr(i);
            
            int oldval = mem_mirror[i];
            int newval = MEM(addr);
            
            bool changed = oldval != newval;
            
            if (changed) {
                if(halfshutter_related && !get_halfshutter_pressed()) {
                    mem_changes[i]=-1;
                    goto ignored;
                }
                mem_changes[i]++;
                mem_mirror[i] = newval;
            }
            
            switch(look_for) {
                    case TYPE_INT8: newval = (int8_t) newval; break;
                    case TYPE_INT16: newval = (int16_t) newval; break;
                    case TYPE_INT32: newval = (int16_t) newval; break;
                    case TYPE_FLOAT: newval = (float) newval; break;
            }
            
            if (look_for == TYPE_BOOL && newval != 0 && newval != 1 && newval != -1 ){
                goto ignored;
            }
            
            if (value_lo && newval < value_lo) goto ignored;
            if (value_hi && newval > value_hi) goto ignored;

            if (count_lo && mem_changes[i] < count_lo) goto ignored;
            if (count_hi && mem_changes[i] > count_hi) goto ignored;

            int freq = mem_changes[i] / elapsed_time;
            if (freq_lo && freq < freq_lo) goto ignored;
            if (freq_hi && freq > freq_hi) goto ignored;
            
            if(mem_position[i] == -1){
                int pos = next_position();
                if(pos != -1) {
                    mem_position[i] = pos;
                    position[mem_position[i]][2] = 0;
                }
            }
            
            if(mem_position[i] != -1) {
                int x = position[mem_position[i]][0];
                int y = position[mem_position[i]][1];
                int font = FONT(FONT_SMALL, (changed) ? COLOR_YELLOW : COLOR_WHITE, COLOR_BLACK);
                switch(look_for) {
                    case TYPE_BOOL:
                        bmp_printf(
                            font,
                            x, y,
                            "%8x\t%8d\t%d\t",
                            addr, mem_changes[i], newval
                        );
                        break;
                    case TYPE_INT8:
                    case TYPE_INT16:
                    case TYPE_INT32:
                        bmp_printf(
                            font,
                            x, y,
                            "%8x\t%8d\t%8d\t",
                            addr, mem_changes[i], newval
                        );
                        break;
                    case TYPE_FLOAT:
                        bmp_printf(
                            font,
                            x, y,
                            "%8x\t%8d\t%d.%d\t",
                            addr, mem_changes[i], newval * 100,  newval % 100
                        );
                        break;
                }
            }
            
            continue;
            
            ignored:
            if(mem_position[i] != -1) {
                bmp_fill(
                    COLOR_BLACK,
                    position[mem_position[i]][0],
                    position[mem_position[i]][1],
                    COLUMN_WIDTH, 12
                );
                position[mem_position[i]][2] = 1;
                mem_position[i] = -1;
            }
        }
        
        msleep(5);
    };
    
    mem_spy = 0;
    mem_spy_running = 0;
}

static void start() {
    var_length = get_byte_length();
    init_position();
    init_done = 0;
    start_delay_counter = start_delay;
    if(fixed_addresses) var_count = COUNT(addresses);
    
    if(mem_spy && !mem_spy_running){
        task_create("mem_spy_task", 0x1c, 0x1000, mem_spy_task, (void*)0);
    }
}

static MENU_SELECT_FUNC(mem_spy_sel){
    mem_spy = !mem_spy;
    start();
}

static MENU_UPDATE_FUNC(look_for_upd){
    mem_spy = 0;
    var_length = get_byte_length();
    MENU_SET_RINFO("%dB", var_length);
}

static MENU_UPDATE_FUNC(fixed_addresses_upd){
    if(fixed_addresses)var_count = COUNT(addresses);
}

static MENU_UPDATE_FUNC(start_addr_upd){
    MENU_SET_VALUE("0x%x", start_addr);
    MENU_SET_ENABLED(!fixed_addresses);
    MENU_SET_ICON(!fixed_addresses ? IT_DICE : IT_DICE_OFF, 0);
}

static MENU_SELECT_FUNC(start_addr_sel){
    start_addr += delta * 10000;
}

static MENU_UPDATE_FUNC(var_count_upd){
    MENU_SET_ENABLED(!fixed_addresses);
    MENU_SET_ICON(!fixed_addresses ? IT_DICE : IT_DICE_OFF, 0);
    if(!fixed_addresses) {
        MENU_SET_RINFO("0x%x", start_addr + var_count * var_length);
    } else {
        MENU_SET_RINFO("");
    }
}

static MENU_SELECT_FUNC(next_range){
    start_addr += var_count * var_length;
}

static MENU_UPDATE_FUNC(zero_disable){
    MENU_SET_ICON(*(int*)entry->priv == 0 ? IT_DICE_OFF : IT_DICE, 0);
}

static MENU_UPDATE_FUNC(start_delay_upd){
    MENU_SET_VALUE("%ds", start_delay);
}
static struct menu_entry mem_spy_menu[] =
{
    {
        .name = "Memory spy",
        .priv = &mem_spy,
        .max = 1,
        .select = mem_spy_sel,
        .submenu_width = 700,
        .children = (struct menu_entry[]) {
            {
                .name = "Look for",
                .priv = &look_for,
                .choices = CHOICES(
                    "bools 0,1,-1",
                    "int8",
                    "int16",
                    "int32",
                    "floats",
                ),
                .max = 4,
                .update = look_for_upd,
                .help = "In fact this convert int32 to you choice.",
                .help2 = "Memory is always scanned by 4B.",
            },
            {
                .name = "Halfshutter related",
                .priv = &halfshutter_related,
                .max = 1,
                .help = "Hide vars that change if halfshutter is not pressed.",
            },
            {
                .name = "Fixed addresses",
                .priv = &fixed_addresses,
                .max = 1,
                .help = "Defined in source.",
                .update = fixed_addresses_upd,
            },
            {
                .name = "Start address",
                .priv = &start_addr,
                .icon_type = IT_DICE,
                .update = start_addr_upd,
                .select = start_addr_sel,
                .help = "Edit module config file for specific address.",
            },
            {
                .name = "Var count",
                .priv = &var_count,
                .icon_type = IT_DICE,
                .max = 200 * 1000,
                .update = var_count_upd,
            },
            {
                .name = "Next range",
                .icon_type = IT_ACTION,
                .select = next_range,
                .help = "Set end address as start address.",
            },
            {
                .name = "Value min",
                .priv = &value_lo,
                .icon_type = IT_DICE,
                .max = 1000 * 1000,
                .min = 1000 * 1000 *-1,
                .help = "Look for a specific range of values.",
                .update = zero_disable,
            },
            {
                .name = "Value max",
                .priv = &value_hi,
                .icon_type = IT_DICE,
                .max = 1000 * 1000,
                .min = 1000 * 1000 *-1,
                .help = "Look for a specific range of values.",
                .update = zero_disable,
            },
            {
                .name = "Changes min",
                .priv = &count_lo,
                .icon_type = IT_DICE,
                .max = 1000 * 1000,
                .help = "How many times is a value allowed to change.",
                .update = zero_disable,
            },
            {
                .name = "Changes max",
                .priv = &count_hi,
                .icon_type = IT_DICE,
                .max = 1000 * 1000,
                .help = "How many times is a value allowed to change.",
                .update = zero_disable,
            },
            {
                .name = "Frequency min",
                .priv = &freq_lo,
                .icon_type = IT_DICE,
                .max = 1000 * 1000,
                .help = "Changes / elapsed time.",
                .update = zero_disable,
            },
            {
                .name = "Frequency max",
                .priv = &freq_hi,
                .icon_type = IT_DICE,
                .max = 1000 * 1000,
                .help = "Changes / elapsed time.",
                .update = zero_disable,
            },
            {
                .name = "Start delay",
                .priv = &start_delay,
                .max = 60*60,
                .update = start_delay_upd,
            },
            MENU_EOL,
        }
    }
};

unsigned int mem_spy_init()
{
    menu_add("Debug", mem_spy_menu, COUNT(mem_spy_menu));
    start();
    return 0;
}

unsigned int mem_spy_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(mem_spy_init)
    MODULE_DEINIT(mem_spy_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Description", "Spy your memory")
MODULE_STRINGS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(mem_spy)
    MODULE_CONFIG(look_for)
    MODULE_CONFIG(halfshutter_related)
    MODULE_CONFIG(fixed_addresses)
    MODULE_CONFIG(start_addr)
    MODULE_CONFIG(var_count)
    MODULE_CONFIG(value_lo)
    MODULE_CONFIG(value_hi)
    MODULE_CONFIG(count_lo)
    MODULE_CONFIG(count_hi)
    MODULE_CONFIG(freq_lo)
    MODULE_CONFIG(freq_hi)
    MODULE_CONFIG(start_delay)
MODULE_CONFIGS_END()
