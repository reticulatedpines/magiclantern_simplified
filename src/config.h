/** \file
 * Key/value parser until we have a proper config.
 *
 * Auto-parsed variables will be assigned when read.
 * To create a configuration parameter:
 * <code>
 * CONFIG_INT( "name", variable, default_value );
 * </code>
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef _config_h_
#define _config_h_

#define MAX_NAME_LEN            64
#define MAX_VALUE_LEN           60


struct config
{
//      struct config *         next;
        char                    name[ MAX_NAME_LEN ];
        char                    value[ MAX_VALUE_LEN ];
};

//extern struct config * global_config;


extern char *
config_value(
        struct config *         config,
        const char *            name
);

extern int
config_int(
        struct config *         config,
        const char *            name,
        int                     def
);


extern int
config_parse_file(
        const char *            filename
);


extern int
config_save_file(
        const char *            filename
);

struct config_var;

//return false in this cbr to block the value from being changed
typedef int (*config_var_change_func)(struct config_var *, int old_value, int new_value);

#define CONFIG_VAR_CHANGE_FUNC(func) \
    int func(struct config_var * var, int old_value, int new_value)

/** Create an auto-parsed config variable */
struct config_var
{
        const char * name;
        //int        type;   //!< 0 == int, 1 == char *
        int *        value;
        int          default_value;
        config_var_change_func change_cbr;
};

#define _CONFIG_VAR( NAME, TYPE_ENUM, VAR, VALUE, CHANGE_CBR ) \
static __attribute__((used)) struct config_var \
__attribute__((section(".config_vars"))) \
__config_##VAR = \
{ \
        .name           = NAME, \
/*        .type           = TYPE_ENUM, */ \
        .value          = (int*) &VAR, \
        .default_value  = (int) VALUE, \
        .change_cbr     = CHANGE_CBR, \
}

#define CONFIG_INT( NAME, VAR, VALUE ) \
        int VAR = VALUE; \
        _CONFIG_VAR( NAME, 0, VAR, VALUE, NULL )

#define CONFIG_INT_EX( NAME, VAR, VALUE, CHANGE_CBR ) \
        int VAR = VALUE; \
        static CONFIG_VAR_CHANGE_FUNC(CHANGE_CBR); \
        _CONFIG_VAR( NAME, 0, VAR, VALUE, CHANGE_CBR )

#define _CONFIG_ARRAY_ELEMENT( NAME, TYPE_ENUM, VAR, INDEX, VALUE ) \
struct config_var \
__attribute__((section(".config_vars"))) \
__config_##VAR##INDEX = \
{ \
        .name           = NAME, \
/*        .type           = TYPE_ENUM, */ \
        .value          = &(VAR[INDEX]), \
        .default_value  = VALUE, \
}

#define CONFIG_ARRAY_ELEMENT( NAME, VAR, INDEX, VALUE ) \
        _CONFIG_ARRAY_ELEMENT( NAME, 0, VAR, INDEX, VALUE )
        
struct config_var* get_config_vars_start ();
struct config_var* get_config_vars_end ();

/* set a config var by name (returns true if change was successful) */
int set_config_var(const char * name, int new_value);

/* set a config var by pointer (for menu backend) */
int set_config_var_ptr(int* ptr, int new_value);

/* lookup a config var by name */
int get_config_var(const char * name);

/* return the current settings directory (usually ML/SETTINGS, but not if you use a custom preset) */
extern char* get_config_dir();

/* return true if the specified config variable (identified by a pointer to its current value)
 * is no longer at its default value */
int config_var_was_changed(int* ptr);

void config_save();
void config_save_at_shutdown(); /* CBR */
void config_load();

/* simple boolean settings that live outside of config files (just by presence of a file) */
int config_flag_file_setting_load(const char * file);
void config_flag_file_setting_save(const char * file, int setting);

#endif
