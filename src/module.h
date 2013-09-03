#ifndef _module_h_
#define _module_h_

#define MODULE_PATH                   CARD_DRIVE"ML/MODULES/"

/* module info structures */
#define MODULE_INFO_PREFIX            __module_info_
#define MODULE_STRINGS_PREFIX         __module_strings_
#define MODULE_PARAMS_PREFIX          __module_params_
#define MODULE_PROPHANDLERS_PREFIX    __module_prophandlers_
#define MODULE_CBR_PREFIX             __module_cbr_
#define MODULE_CONFIG_PREFIX          __module_config_


#define MODULE_PROPHANDLER_PREFIX     __module_prophandler_

#define MODULE_MAGIC                  0x5A
#define STR(x)                        STR_(x)
#define STR_(x)                       #x

#define MODULE_COUNT_MAX              32
#define MODULE_NAME_LENGTH            8
#define MODULE_FILENAME_LENGTH        64
#define MODULE_STATUS_LENGTH          64


/* some callbacks that may be needed by modules. more to come. ideas? needs? */
#define CBR_PRE_SHOOT                 1 /* called before image is taken */
#define CBR_POST_SHOOT                2 /* called after image is taken */
#define CBR_SHOOT_TASK                3 /* called periodically from shoot task */
#define CBR_DISPLAY_FILTER            4
/**
 * Display filters (things that alter the LiveView image)
 * 
 * - ctx=0: should return 1 if the display filter mode should be enabled, 0 if not
 *          (but must not do any heavy processing!!!)
 * 
 * - else: ctx is a (struct display_filter_buffers *), see vram.h
 *         (contains src_buf and dst_buf)
 *         in this case, the cbr should paint the new image in dst_buf,
 *         usually by processing the data from src_buf
 *
 * - note: some cameras do not support display filters; in this case, this CBR will not be called
 */
#define CBR_SECONDS_CLOCK             5 /* called every second */
#define CBR_VSYNC                     6 /* called for every LiveView frame; can do display tricks; must not do any heavy processing!!! */
#define CBR_KEYPRESS                  7 /* when a key was pressed, this cbr gets the translated key as ctx */
#define CBR_KEYPRESS_RAW              8 /* when a key was pressed, this cbr gets the raw (struct event *) as ctx */
#define CBR_VSYNC_SETPARAM            9 /* called from every LiveView frame; can change FRAME_ISO, FRAME_SHUTTER_TIMER, just like for HDR video */

/* portable key codes. intentionally defines to make numbers hardcoded so changed order wont change the integer number */
#define MODULE_KEY_PRESS_HALFSHUTTER       ( 1)
#define MODULE_KEY_UNPRESS_HALFSHUTTER     ( 2)
#define MODULE_KEY_PRESS_FULLSHUTTER       ( 3)
#define MODULE_KEY_UNPRESS_FULLSHUTTER     ( 4)
#define MODULE_KEY_WHEEL_UP                ( 5)
#define MODULE_KEY_WHEEL_DOWN              ( 6)
#define MODULE_KEY_WHEEL_LEFT              ( 7)
#define MODULE_KEY_WHEEL_RIGHT             ( 8)
#define MODULE_KEY_PRESS_SET               ( 9)
#define MODULE_KEY_UNPRESS_SET             (10)
#define MODULE_KEY_JOY_CENTER              (11)
#define MODULE_KEY_PRESS_UP                (12)
#define MODULE_KEY_PRESS_UP_RIGHT          (13)
#define MODULE_KEY_PRESS_UP_LEFT           (14)
#define MODULE_KEY_PRESS_RIGHT             (15)
#define MODULE_KEY_PRESS_LEFT              (16)
#define MODULE_KEY_PRESS_DOWN_RIGHT        (17)
#define MODULE_KEY_PRESS_DOWN_LEFT         (18)
#define MODULE_KEY_PRESS_DOWN              (19)
#define MODULE_KEY_UNPRESS_UDLR            (20)
#define MODULE_KEY_PRESS_ZOOMIN            (21)     
#define MODULE_KEY_MENU                    (22)
#define MODULE_KEY_INFO                    (23)
#define MODULE_KEY_PLAY                    (24)
#define MODULE_KEY_TRASH                   (25)
#define MODULE_KEY_RATE                    (26)
#define MODULE_KEY_REC                     (27)
#define MODULE_KEY_LV                      (28)
#define MODULE_KEY_Q                       (29)
#define MODULE_KEY_PICSTYLE                (30)
#define MODULE_KEY_PRESS_FLASH_MOVIE       (31)
#define MODULE_KEY_UNPRESS_FLASH_MOVIE     (32)
#define MODULE_KEY_PRESS_DP                (33)
#define MODULE_KEY_UNPRESS_DP              (34)


#define MODULE_KEY_CANON     0
#define MODULE_KEY_PORTABLE  1


/* update major if older modules will *not* be compatible */
#define MODULE_MAJOR 3
/* update minor if older modules will be compatible, but newer module will not run on older magic lantern versions */
#define MODULE_MINOR 0
/* update patch if nothing regarding to compatibility changes */
#define MODULE_PATCH 0

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
    const char *long_name;
    unsigned int (*init) ();
    unsigned int (*deinit) ();
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
} module_parminfo_t;

/* this struct supplies additional information like license, author etc - optional */
typedef struct
{
    const char *name;
    const char *value;
} module_strpair_t;

/* this struct supplies a list of callback routines to call - optional */
typedef struct
{
    const char *name;
    const char *symbol;
    unsigned int type;
    unsigned int (*handler) (unsigned int);
    unsigned int ctx;
} module_cbr_t;

/* this struct supplies a list of config entries liek CONFIG_INT - optional */
typedef struct
{
    const char *name;
    const struct config_var *ref;
} module_config_t;

/* this struct supplies additional information like license, author etc - optional */
typedef struct
{
    const char *name;
    void (*handler)(unsigned int property, void * priv, void * addr, unsigned int len);
    const unsigned int property;
    const unsigned int property_length;
} module_prophandler_t;


/* index of all loaded modules */
typedef struct
{
    char name[MODULE_NAME_LENGTH+1];
    char filename[MODULE_FILENAME_LENGTH+1];
    char long_filename[MODULE_FILENAME_LENGTH+1];
    char status[MODULE_STATUS_LENGTH+1];
    char long_status[MODULE_STATUS_LENGTH+1];
    module_info_t *info;
    module_strpair_t *strings;
    module_parminfo_t *params;
    module_prophandler_t **prop_handlers;
    module_cbr_t *cbr;
    module_config_t *config;
    int valid;
    int enabled;
    int error;
} module_entry_t;


#define MODULE_INFO_START()                                     MODULE_INFO_START_(MODULE_INFO_PREFIX,MODULE_NAME)
#define MODULE_INFO_START_(prefix,modname)                      MODULE_INFO_START__(prefix,modname)
#define MODULE_INFO_START__(prefix,modname)                     module_info_t prefix##modname = \
                                                                {\
                                                                    .api_magic = MODULE_MAGIC, \
                                                                    .api_major = MODULE_MAJOR, \
                                                                    .api_minor = MODULE_MINOR, \
                                                                    .api_patch = MODULE_PATCH, \
                                                                    .name = #modname,
#define MODULE_INIT(func)                                           .init = &func,
#define MODULE_DEINIT(func)                                         .deinit = &func,
#define MODULE_LONGNAME(name)                                       .long_name = name,
#define MODULE_CB_SHOOT_TASK(func)                                  .cb_shoot_task = &func,
#define MODULE_CB_PRE_SHOOT(func)                                   .cb_pre_shoot = &func,
#define MODULE_CB_POST_SHOOT(func)                                  .cb_post_shoot = &func,
#define MODULE_INFO_END()                                       };
                                                                
#define MODULE_STRINGS_START()                                  MODULE_STRINGS_START_(MODULE_STRINGS_PREFIX,MODULE_NAME)
#define MODULE_STRINGS_START_(prefix,modname)                   MODULE_STRINGS_START__(prefix,modname)
#define MODULE_STRINGS_START__(prefix,modname)                  module_strpair_t prefix##modname[] = {
#define MODULE_STRING(field,value)                                  { field, value },
#define MODULE_STRINGS_END()                                        { (const char *)0, (const char *)0 }\
                                                                };                                                                
#define MODULE_CBRS_START()                                     MODULE_CBRS_START_(MODULE_CBR_PREFIX,MODULE_NAME)
#define MODULE_CBRS_START_(prefix,modname)                      MODULE_CBRS_START__(prefix,modname)
#define MODULE_CBRS_START__(prefix,modname)                     module_cbr_t prefix##modname[] = {
#define MODULE_CBR(cb_type,cbr,context)                         { .name = #cb_type, .symbol = #cbr, .type = cb_type, .handler = cbr, .ctx = context },
#define MODULE_CBRS_END()                                           { (void *)0, (void *)0, 0, (void *)0, 0 }\
                                                                };
                                                            
#define MODULE_CONFIGS_START()                                  MODULE_CONFIGS_START_(MODULE_CONFIG_PREFIX,MODULE_NAME)
#define MODULE_CONFIGS_START_(prefix,modname)                   MODULE_CONFIGS_START__(prefix,modname)
#define MODULE_CONFIGS_START__(prefix,modname)                  module_config_t prefix##modname[] = {
#define MODULE_CONFIG(cfg)                                      { .name = #cfg, .ref = &__config_##cfg },
#define MODULE_CONFIGS_END()                                        { (void *)0, (void *)0 }\
                                                                };
                                                                
#define MODULE_PARAMS_START()                                   MODULE_PARAMS_START_(MODULE_PARAMS_PREFIX,MODULE_NAME)
#define MODULE_PARAMS_START_(prefix,modname)                    MODULE_PARAMS_START__(prefix,modname)
#define MODULE_PARAMS_START__(prefix,modname)                   module_parminfo_t prefix##modname[] = {
#define MODULE_PARAM(var,typestr,descr)                             { .parameter = &var, .name = #var, .type = typestr, .desc = descr },
#define MODULE_PARAMS_END()                                         { (void *)0, (const char *)0, (const char *)0, (const char *)0 }\
                                                                };

#define MODULE_PROPHANDLERS_START()                             MODULE_PROPHANDLERS_START_(MODULE_PROPHANDLERS_PREFIX,MODULE_NAME,MODULE_PROPHANDLER_PREFIX)
#define MODULE_PROPHANDLERS_START_(prefix,modname,ph_prefix)    MODULE_PROPHANDLERS_START__(prefix,modname,ph_prefix)
#define MODULE_PROPHANDLERS_START__(prefix,modname,ph_prefix)   module_prophandler_t *prefix##modname[] = {
#define MODULE_PROPHANDLER(id)                                      MODULE_PROPHANDLER_(MODULE_PROPHANDLERS_PREFIX,MODULE_NAME,MODULE_PROPHANDLER_PREFIX,id)
#define MODULE_PROPHANDLER_(prefix,modname,ph_prefix,id)            MODULE_PROPHANDLER__(prefix,modname,ph_prefix,id)
#define MODULE_PROPHANDLER__(prefix,modname,ph_prefix,id)           &ph_prefix##modname##_##id##_block,
#define MODULE_PROPHANDLERS_END()                                   NULL\
                                                                };

#if defined(MODULE)
extern char *module_card_drive;
#define MODULE_CARD_DRIVE                                       module_card_drive
#define PROP_HANDLER(id)                                        MODULE_PROP_ENTRY_(MODULE_PROPHANDLER_PREFIX,MODULE_NAME, id, #id)
#define MODULE_PROP_ENTRY_(prefix,modname,id,idstr)             MODULE_PROP_ENTRY__(prefix,modname,id,idstr)
#define MODULE_PROP_ENTRY__(prefix,modname,id,idstr)            void prefix##modname##_##id(unsigned int, void *, void *, unsigned int);\
                                                                module_prophandler_t prefix##modname##_##id##_block = { \
                                                                    .name            = idstr, \
                                                                    .handler         = &prefix##modname##_##id, \
                                                                    .property        = id, \
                                                                    .property_length = 0, \
                                                                }; \
                                                                void prefix##modname##_##id( \
                                                                        unsigned int property, \
                                                                        void *       token, \
                                                                        void *       buf, \
                                                                        unsigned int len \
                                                                )
#endif


/* load all available modules. will be used on magic lantern boot */
void module_load_all(void);
void module_unload_all(void);

/* explicitely load a standalone module. this is comparable to an executable */
void *module_load(char *filename);
int module_exec(void *module, char *symbol, int count, ...);
int module_unload(void *module);
unsigned int module_get_symbol(void *module, char *symbol);

/* execute all callback routines of given type. maybe it will get extended to support varargs */
int module_exec_cbr(unsigned int type);

int module_set_config_cbr(unsigned int (*load_func)(char *, module_entry_t *), unsigned int (save_func)(char *, module_entry_t *));
int module_unset_config_cbr();

#ifdef MODULE
#include "module_strings.h"
#endif

#endif
