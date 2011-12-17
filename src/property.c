/** \file
 * Property handler installation
 *
 * These handlers are registered to allow Magic Lantern to interact with
 * the Canon "properties" that are used to exchange globals.
 */

#include "dryos.h"
#include "property.h"


// This must be two instructions long!
static void
prop_token_handler_generic(
	void * token
)
{
	asm( "str r0, [pc, #-12]" );
}

void prop_handler_init(struct prop_handler * handler)
{
	memcpy(
		handler->token_handler,
		prop_token_handler_generic,
		8
	);

	prop_register_slave(
		&handler->property,
		1,
		handler->handler,
		&handler->token,
		(void(*)(void*))&handler->token_handler
	);
}

static void
prop_init( void* unused )
{
	extern struct prop_handler _prop_handlers_start[];
	extern struct prop_handler _prop_handlers_end[];
	struct prop_handler * handler = _prop_handlers_start;

	for( ; handler < _prop_handlers_end ; handler++ )
	{
		// Copy the generic token handler into the structure
		memcpy(
			handler->token_handler,
			prop_token_handler_generic,
			8
		);

		prop_register_slave(
			&handler->property,
			1,
			handler->handler,
			&handler->token,
			(void(*)(void*))&handler->token_handler
		);
	}
}

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
