/**
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!! VERY IMPORTANT !!!                                                                                                   !!!
 * !!! For new ports, DISABLE prop_request_change first (see below) !!!                                                     !!!
 * !!! BEFORE enabling it, check and double-check that meaning and valid range of values for each prop_request_change call  !!!
 * !!! are identical to the ones from fully working ports.                                                                  !!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/** \file
 * Property handler installation
 * 
 * Rather than registering a handler for each property (which seems to overload DryOS),
 * it's probably better to have a single global property handler.
 *
 * Old implementation: property-old.c
 */

#include "dryos.h"
#include "property.h"
#include "bmp.h"

static void * global_token;

static void global_token_handler( void * token)
{
    global_token = token;
}


static void *
global_property_handler(
    unsigned        property,
    void *          UNUSED_ATTR( priv ),
    void *          buf,
    unsigned        len
)
{
    //~ bfnt_puts("Global prop", 0, 0, COLOR_BLACK, COLOR_WHITE);

    extern struct prop_handler _prop_handlers_start[];
    extern struct prop_handler _prop_handlers_end[];
    struct prop_handler * handler = _prop_handlers_start;

    for( ; handler < _prop_handlers_end ; handler++ )
    {
        if (handler->property == property)
        {
            //~ bmp_printf(FONT_LARGE, 0, 0, "%x %x...", property, handler->handler);
            handler->handler(property, priv, buf, len);
            //~ bmp_printf(FONT_LARGE, 0, 0, "%x %x :)", property, handler->handler);
        }
    }
    return (void*)_prop_cleanup(global_token, property);
}

static unsigned property_list[256];

void
prop_init( void* unused )
{
    int actual_num_properties = 0;

    extern struct prop_handler _prop_handlers_start[];
    extern struct prop_handler _prop_handlers_end[];
    struct prop_handler * handler = _prop_handlers_start;

    for( ; handler < _prop_handlers_end ; handler++ )
    {
        int duplicate = 0;
        for (int i = 0; i < actual_num_properties; i++)
        {
            if (_prop_handlers_start[i].property == handler->property)
            {
                duplicate = 1;
                break;
            }
        }

        if (!duplicate)
        {
            property_list[actual_num_properties] = handler->property;
            actual_num_properties++;
        }
        if (actual_num_properties >= COUNT(property_list))
        {
            bfnt_puts("Too many prop handlers", 0, 0, COLOR_BLACK, COLOR_WHITE);
            break;
        }
    }

    prop_register_slave(
        property_list,
        actual_num_properties,
        global_property_handler,
        &global_token,
        global_token_handler
    );
}

extern void * prop_cleanup(void * token, unsigned property) { return 0; } // dummy

// for reading simple integer properties
int get_prop(int prop)
{
    int* data = 0;
    size_t len = 0;
    int err = prop_get_value(prop, (void **) &data, &len);
    if (!err) return data[0];
    return 0;
}

// for strings
char* get_prop_str(int prop)
{
    char* data = 0;
    size_t len = 0;
    int err = prop_get_value(prop, (void **) &data, &len);
    if (!err) return data;
    return 0;
}

// prop_get_value may take a long time to run, so let's try to use a small cache
int get_prop_len_uncached(int prop)
{
    int* data = 0;
    size_t len = 0;
    int err = prop_get_value(prop, (void **) &data, &len);
    if (!err) return (int)len;
    return 0;
}

// plc = property length cache
// circular buffer
static int plc_prop[32] = {0};
static int plc_len[32] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static int plc_i = 0;

int get_prop_len(int prop)
{
    for (int i = 0; i < 32; i++)
    {
        if (plc_prop[i] == prop && plc_len[i] >= 0)
            return plc_len[i];
    }
    return get_prop_len_uncached(prop);
}

/**
 * This is just a safe wrapper for changing camera settings (well... only slightly safer than Canon's) 
 * Double-check the len parameter => less chances that our call will cause permanent damage.
 */

/**
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!! VERY IMPORTANT !!!                                                                                                   !!!
 * !!! For new ports, DISABLE this function first!!!                                                                        !!!
 * !!! BEFORE enabling it, check and double-check that meaning and valid range of values for each prop_request_change call  !!!
 * !!! are identical to the ones from fully working ports.                                                                  !!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

void prop_request_change(unsigned property, const void* addr, size_t len)
{
    int correct_len = get_prop_len((int)property);
    
    if (property == PROP_BATTERY_REPORT && len == 1) goto ok; // exception: this call is correct for polling battery level
    
    if (correct_len != (int)len)
    {
        #define PROP_LEN_INCORRECT 0
        bmp_printf(FONT_LARGE, 100, 100, "%x:%x:%x", property, correct_len, len);
        ASSERT(PROP_LEN_INCORRECT);
        info_led_blink(10,50,50);
        return;
    }

ok:
    //~ console_printf("prop:%x data:%x len:%x\n", property, MEM(addr), len);
    _prop_request_change(property, addr, len);
}


/**
 * For new ports, disable this function on first boots (although it should be pretty much harmless).
 */
INIT_FUNC( __FILE__, prop_init );
