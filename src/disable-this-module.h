// some defines which disable PROP_HANDLER's, TASK_CREATE's, TASK_OVERRIDE's and INIT_FUNC's

#ifdef PROP_HANDLER
#undef PROP_HANDLER
#endif

#ifdef TASK_CREATE
#undef TASK_CREATE
#endif

#ifdef TASK_OVERRIDE
#undef TASK_OVERRIDE
#endif

#ifdef INIT_FUNC
#undef INIT_FUNC
#endif


#define PROP_HANDLER(id) \
static void _prop_handler_##id( \
        unsigned                property, \
        void *                  token, \
        uint32_t *              buf, \
        unsigned                len \
) \



#define TASK_CREATE( NAME, ENTRY, ARG, PRIORITY, FLAGS ) 
#define TASK_OVERRIDE( NAME, ENTRY ) 
#define INIT_FUNC( NAME, ENTRY ) 

