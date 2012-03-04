/** \file
 * Property handler installation
 *
 * Rather than registering a handler for each property (which seems to overload DryOS),
 * it's probably better to have a single global property handler.
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

INIT_FUNC( __FILE__, prop_init );
