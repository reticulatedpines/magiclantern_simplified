// some defines which disable PROP_HANDLER's, TASK_CREATE's, TASK_OVERRIDE's and INIT_FUNC's

#define PROP_HANDLER(id) \
static void * _prop_handler_##id( \
	unsigned		property, \
	void *			token, \
	uint32_t *		buf, \
	unsigned		len \
) \



#define TASK_CREATE( NAME, ENTRY, ARG, PRIORITY, FLAGS ) 
#define TASK_OVERRIDE( NAME, ENTRY ) 
#define INIT_FUNC( NAME, ENTRY ) 

