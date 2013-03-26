#ifndef _module_h_
#define _module_h_


#define MODULE_MAGIC 0x5A

/* update major if older modules will *not* be compatible */
#define MODULE_MAJOR 0
/* update minor if older modules will be compatible, but newer module will not run on older magic lantern versions */
#define MODULE_MINOR 1
/* update patch if nothing regarding to compatibility changes */
#define MODULE_PATCH 1

/* module description that every module should deliver - optional. 
   if not supplied, only the symbols are visible to other plugins.
   this might be useful for libraries.
*/
typedef struct
{
    /* major determines the generic compatibilty, minor is backward compatible (e.g. new hooks) */
    const unsigned char api_magic;
    const unsigned char api_major;
    const unsigned char api_minor;
    const unsigned char api_patch;
    
    /* the plugin's name and init/deinit functions */
    const char *name;
    unsigned int (*init) ();
    unsigned int (*deinit) ();
    
    /* some callbacks that may be needed by modules. more to come. ideas? needs? */
    void (*cb_shoot_task) (); /* called periodically from shoot task */
    void (*cb_pre_shoot) (); /* called before image is taken */
    void (*cb_post_shoot) (); /* called after image is taken */
} module_info_t;

/* modules can have parameters - optional */
typedef struct
{
    /* pointer to parameter in memory */
    const void *parameter;
    /* stringified type like "uint32_t", "int32_t". restrict to stdint.h types */
    const char *type;
    /* name of the parameter, must match to variable name */
    const char *name;
    /* description for the user */
    const char *desc;
} module_parmtinfo_t;

/* this struct supplies additional information like license, author etc - optional */
typedef struct
{
    const char *name;
    const char *value;
} module_strpair_t;


#define MODULE_INFO_START(module)     module_info_t __module_info_##module = \
                                      {\
                                          .api_magic = MODULE_MAGIC, \
                                          .api_major = MODULE_MAJOR, \
                                          .api_minor = MODULE_MINOR, \
                                          .api_patch = MODULE_PATCH, \
                                          .name = #module,
#define MODULE_INIT(func)                 .init = &func,
#define MODULE_DEINIT(func)               .deinit = &func,
#define MODULE_CB_SHOOT_TASK(func)        .cb_shoot_task = &func,
#define MODULE_CB_PRE_SHOOT(func)         .cb_pre_shoot = &func,
#define MODULE_CB_POST_SHOOT(func)        .cb_post_shoot = &func,
#define MODULE_INFO_END()             };

#define MODULE_STRINGS_START(module)  module_strpair_t __module_strings_##module[] = {
#define MODULE_STRING(field,value)        { field, value },
#define MODULE_STRINGS_END()              { (const char *)0, (const char *)0 }\
                                      };

#define MODULE_PARAMS_START(module)  module_parmtinfo_t __module_params_##module[] = {
#define MODULE_PARAM(var,type,desc)       { &var, #var, type, desc },
#define MODULE_PARAMS_END()               { (void *)0, (const char *)0, (const char *)0, (const char *)0 }\
                                      };

#endif
