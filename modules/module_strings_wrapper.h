
/* this struct supplies additional information like license, author etc - optional */
typedef struct
{
    const char *name;
    const char *value;
} module_strpair_t;

#define MODULE_STRINGS_SECTION

#define MODULE_STRINGS_START()                                  MODULE_STRINGS_START_(MODULE_STRINGS_PREFIX)
#define MODULE_STRINGS_START_(varname)                          MODULE_STRINGS_START__(varname)
#define MODULE_STRINGS_START__(varname)                         module_strpair_t varname[] MODULE_STRINGS_SECTION = {
#define MODULE_STRING(field,value)                                  { field, value },
#define MODULE_STRINGS_END()                                        { (const char *)0, (const char *)0 }\
                                                                };

static const char* module_get_string(module_strpair_t *strings, const char* name)
{
    if(strings)
    {
        for( ; strings->name != NULL; strings++)
        {
            if(!strcmp(strings->name, name))
            {
                return strings->value;
            }
        }
    }
    
    return NULL;
}