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

extern struct prop_handler _prop_handlers_start[];
extern struct prop_handler _prop_handlers_end[];

static void * global_token;

static void global_token_handler( void * token)
{
    global_token = token;
}

static int current_prop_handler = 0;

static void *
global_property_handler(
    unsigned        property,
    void *          UNUSED_ATTR( priv ),
    void *          buf,
    unsigned        len
)
{
#ifdef CONFIG_5DC
    if (property == 0x80010001) return (void*)_prop_cleanup(global_token, property);
#endif
    
    //~ bfnt_puts("Global prop", 0, 0, COLOR_BLACK, COLOR_WHITE);
    struct prop_handler * handler = _prop_handlers_start;

    for( ; handler < _prop_handlers_end ; handler++ )
    {
        if (handler->property == property)
        {
            //~ bmp_printf(FONT_LARGE, 0, 0, "%x %x...", property, handler->handler);
            /* cache length of property if not set yet */
            if(handler->property_length == 0)
            {
                handler->property_length = len;
            }
            
            /* execute handler, if any */
            if(handler->handler != NULL)
            {
                current_prop_handler = property;
                handler->handler(property, priv, buf, len);
                current_prop_handler = 0;
            }
            //~ bmp_printf(FONT_LARGE, 0, 0, "%x %x :)", property, handler->handler);
        }
    }
    return (void*)_prop_cleanup(global_token, property);
}

static unsigned property_list[256];

static void
prop_init( void* unused )
{
    int actual_num_properties = 0;
    struct prop_handler * handler = _prop_handlers_start;

    for( ; handler < _prop_handlers_end ; handler++ )
    {
#if defined(POSITION_INDEPENDENT)
        handler->handler = PIC_RESOLVE(handler->handler);
#endif
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

#if 0
// for reading simple integer properties
// not reliable in realtime scenarios (race condition?)
int _get_prop(int prop)
{
    int* data = 0;
    size_t len = 0;
    int err = prop_get_value(prop, (void **) &data, &len);
    if (!err) return data[0];
    return 0;
}

// for strings
// not reliable in realtime scenarios (race condition?)
char* _get_prop_str(int prop)
{
    char* data = 0;
    size_t len = 0;
    int err = prop_get_value(prop, (void **) &data, &len);
    if (!err) return data;
    return 0;
}
#endif
/* not reliable


// prop_get_value may take a long time to run, so let's try to use a small cache
int _get_prop_len_uncached(int prop)
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

int _get_prop_len(int prop)
{
    for (int i = 0; i < 32; i++)
    {
        if (plc_prop[i] == prop && plc_len[i] >= 0)
            return plc_len[i];
    }
    int len = get_prop_len_uncached(prop);
    plc_prop[plc_i] = prop;
    plc_len[plc_i] = len;
    plc_i = (plc_i + 1) % 32;
    return len;
}*/

/* return cached length of property */
static uint32_t prop_get_prop_len(uint32_t property)
{
    struct prop_handler * handler = NULL;

    for(handler = _prop_handlers_start; handler < _prop_handlers_end; handler++ )
    {
        if (handler->property == property)
        {
            return handler->property_length;
        }
    }
    
    return 0;
}


/**
 * This is just a safe wrapper for changing camera settings (well... only slightly safer than Canon's) 
 * Double-check the len parameter => less chances that our call will cause permanent damage.
 */

/**
 * You can also pass len=0; in this case, the length will be detected automatically.
 * Don't abuse this, only use it for properties where length is camera-specific, 
 * and if you call something with len=0, don't forget to back it up with an ASSERT
 * which checks if len is not higher than the max len assumed by ML.
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
#ifdef CONFIG_PROP_REQUEST_CHANGE

	#if defined(CONFIG_40D)
	if(property != PROP_AFPOINT) {
		return;
	}
	#endif
    
    #ifdef CONFIG_DIGIC_V
    if (property == PROP_VIDEO_MODE) // corrupted video headers on 5D3
        return;
    #endif

    int correct_len = prop_get_prop_len((int)property);
    if (len == 0) len = correct_len;
    if (len == 0)
    {
        char msg[100];
        snprintf(msg, sizeof(msg), "PROP_LEN(%x) = 0", property);
        bmp_printf(FONT(FONT_LARGE, COLOR_WHITE, COLOR_RED), 100, 100, msg);
        ml_assert_handler(msg, __FILE__, __LINE__, __func__);
        info_led_blink(10,50,50);
        return;
    }
    
    if (property == PROP_BATTERY_REPORT && len == 1) goto ok; // exception: this call is correct for polling battery level
    
    if (property == PROP_REMOTE_SW1 || property == PROP_REMOTE_SW2)
        ASSERT(len <= 4); // some cameras have len=2, others 4; we pass a single integer as param, so max len is 4
    
    if (correct_len != (int)len)
    {
        char msg[100];
        snprintf(msg, sizeof(msg), "PROP_LEN(%x) correct:%x called:%x", property, correct_len, len);
        bmp_printf(FONT(FONT_MED, COLOR_WHITE, COLOR_RED), 0, 100, msg);
        ml_assert_handler(msg, __FILE__, __LINE__, __func__);
        info_led_blink(10,50,50);
        return;
    }

    ok:
    //~ console_printf("prop:%x data:%x len:%x\n", property, MEM(addr), len);
    
    _prop_request_change(property, addr, len);
#endif
}


/**
 * For new ports, disable this function on first boots (although it should be pretty much harmless).
 */
INIT_FUNC( __FILE__, prop_init );

/* register those as dummy handlers to make sure we receive them (for getting prop length) */
REGISTER_PROP_HANDLER(PROP_REMOTE_SW1, NULL);
REGISTER_PROP_HANDLER(PROP_REMOTE_SW2, NULL);
REGISTER_PROP_HANDLER(PROP_LV_LENS_DRIVE_REMOTE, NULL);
REGISTER_PROP_HANDLER(PROP_REMOTE_AFSTART_BUTTON, NULL);
REGISTER_PROP_HANDLER(PROP_WB_MODE_PH, NULL);
REGISTER_PROP_HANDLER(PROP_WB_KELVIN_PH, NULL);
