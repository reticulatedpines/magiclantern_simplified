/** \file
 * Key/value parser until we have a proper config
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

#include "dryos.h"
#include "config.h"
#include "version.h"
#include "bmp.h"
#include "module.h"
#include "menu.h"
#include "property.h"
#include "beep.h"

static int config_selected = 0;
static char config_selected_by_key[9] = "";
static char config_selected_by_mode[9] = "";
static char config_selected_by_name[9] = "";

static int config_ok = 0;
static int config_deleted = 0;

static char* config_file_buf = 0;
static int config_file_size = 0;
static int config_file_pos = 0;

extern struct config_var _config_vars_start[];
extern struct config_var _config_vars_end[];
static struct semaphore *config_save_sem = 0;


static struct config *config_parse_line(const char *line)
{
    int name_len = 0;
    int value_len = 0;
    static struct config _cfg;
    struct config * cfg = &_cfg;

    // Trim any leading whitespace
    int i = 0;
    while( line[i] && isspace( line[i] ) )
        i++;

    // Copy the name to the name buffer
    while( line[i]
    && !isspace( line[i] )
    && line[i] != '='
    && name_len < MAX_NAME_LEN
    )
        cfg->name[ name_len++ ] = line[i++];

    if( name_len == MAX_NAME_LEN )
        goto parse_error;

    // And nul terminate it
    cfg->name[ name_len ] = '\0';

    // Skip any white space and = signs
    while( line[i] && isspace( line[i] ) )
        i++;
    if( line[i++] != '=' )
        goto parse_error;
    while( line[i] && isspace( line[i] ) )
        i++;

    // Copy the value to the value buffer
    while( line[i] && value_len < MAX_VALUE_LEN )
        cfg->value[ value_len++ ] = line[ i++ ];

    // Back up to trim any white space
    while( value_len > 0 && isspace( cfg->value[ value_len-1 ] ) )
        value_len--;

    // And nul terminate it
    cfg->value[ value_len ] = '\0';

    DebugMsg( DM_MAGIC, 3,
        "%s: '%s' => '%s'",
        __func__,
        cfg->name,
        cfg->value
    );

    return cfg;

parse_error:
    DebugMsg( DM_MAGIC, 3,
        "%s: PARSE ERROR: len=%d,%d string='%s'",
        __func__,
        name_len,
        value_len,
        line
    );
    
    bmp_printf(FONT_LARGE, 10, 150, "CONFIG PARSE ERROR");
    bmp_printf(FONT_MED, 10, 200,
        "%s: PARSE ERROR:\nlen=%d,%d\nstring='%s'",
        __func__,
        name_len,
        value_len,
        line
    );

    msleep(2000);
    //~ FreeMemory( cfg );
    call("dumpf");
//~ malloc_error:
    return 0;
}

static int get_char_from_config_file(char* out)
{
    if (config_file_pos >= config_file_size) return 0;
    *out = config_file_buf[config_file_pos++];
    return 1;
}

static int read_line(char *buf, size_t size)
{
    size_t          len = 0;

    while( len < size )
    {
        int rc = get_char_from_config_file(buf+len);
        if( rc <= 0 )
            return -1;

        if( buf[len] == '\r' )
            continue;
        if( buf[len] == '\n' )
        {
            buf[len] = '\0';
            return len;
        }

        len++;
    }

    return -1;
}


static void config_auto_parse(struct config *cfg)
{
    for(struct config_var *var = _config_vars_start; var < _config_vars_end ; var++ )
    {
#if defined(POSITION_INDEPENDENT)
        var->name = PIC_RESOLVE(var->name);
        var->value = PIC_RESOLVE(var->value);
#endif
        if( !streq( var->name, cfg->name ) )
            continue;

        DebugMsg( DM_MAGIC, 3, "%s: '%s' => '%s'", __func__, cfg->name, cfg->value);

        *(int*) var->value = atoi( cfg->value );

        return;
    }

    DebugMsg( DM_MAGIC, 3, "%s: '%s' unused?", __func__, cfg->name );
}


int config_save_file(const char *filename)
{
    int count = 0;

    DebugMsg( DM_MAGIC, 3, "%s: saving to %s", __func__, filename );
    
    #define MAX_SIZE 10240
    char* msg = alloc_dma_memory(MAX_SIZE);
    msg[0] = '\0';
  
    snprintf( msg, MAX_SIZE,
        "# Magic Lantern %s (%s)\n"
        "# Built on %s by %s\n",
        build_version,
        build_id,
        build_date,
        build_user
    );

    struct tm now;
    LoadCalendarFromRTC( &now );

    snprintf(msg + strlen(msg), MAX_SIZE - strlen(msg),
        "# Configuration saved on %04d/%02d/%02d %02d:%02d:%02d\n",
        now.tm_year + 1900,
        now.tm_mon + 1,
        now.tm_mday,
        now.tm_hour,
        now.tm_min,
        now.tm_sec
    );

    for(struct config_var *var = _config_vars_start; var < _config_vars_end ; var++ )
    {
        if (*(int*)var->value == var->default_value)
            continue;

        snprintf(msg + strlen(msg), MAX_SIZE - strlen(msg) - 1,
            "%s = %d\r\n",
            var->name,
            *(int*) var->value
        );

        count++;
    }
    
    FILE * file = FIO_CreateFile( filename );
    if( file == INVALID_PTR )
    {
        free_dma_memory(msg);
        return -1;
    }
    
    FIO_WriteFile(file, msg, strlen(msg));

    FIO_CloseFile( file );
    
    free_dma_memory(msg);
    
    return count;
}


static struct config *config_parse()
{
    char line_buf[1000];
    struct config * cfg = 0;
    int count = 0;

    while( read_line(line_buf, sizeof(line_buf) ) >= 0 )
    {
        //~ bmp_printf(FONT_SMALL, 0, 0, "cfg line: %s      ", line_buf);
        
        // Ignore any line that begins with # or is empty
        if( line_buf[0] == '#' ||  line_buf[0] == '\0' )
            continue;
        
        DebugMsg(DM_MAGIC, 3, "cfg line: %s", line_buf);
        struct config * new_config = config_parse_line( line_buf );
        if( !new_config )
            goto error;

        cfg = new_config;
        count++;

        config_auto_parse( cfg );
    }

    DebugMsg( DM_MAGIC, 3, "%s: Read %d config values", __func__, count );
    return cfg;

error:
    DebugMsg( DM_MAGIC, 3, "%s: ERROR", __func__ );
    return NULL;
}

int config_autosave = 1;

int config_flag_file_setting_load(char* file)
{
    uint32_t size;
    return ( FIO_GetFileSize( file, &size ) == 0 );
}

void config_flag_file_setting_save(char* file, int setting)
{
    FIO_RemoveFile(file);
    if (setting)
    {
        FILE* f = FIO_CreateFile(file);
        FIO_CloseFile(f);
    }
}

static MENU_SELECT_FUNC(config_autosave_toggle)
{
    char autosave_flag_file[0x80];
    
    snprintf(autosave_flag_file, sizeof(autosave_flag_file), "%sAUTOSAVE.NEG", get_config_dir());
    config_flag_file_setting_save(autosave_flag_file, !!config_autosave);
    msleep(50);
    config_autosave = !config_flag_file_setting_load(autosave_flag_file);
}

int config_parse_file(const char *filename)
{
    char autosave_flag_file[0x80];
    
    snprintf(autosave_flag_file, sizeof(autosave_flag_file), "%sAUTOSAVE.NEG", get_config_dir());
    config_autosave = !config_flag_file_setting_load(autosave_flag_file);

    config_file_buf = (void*)read_entire_file(filename, &config_file_size);
    config_file_pos = 0;
    config_parse();
    free_dma_memory(config_file_buf);
    config_file_buf = 0;
    return 1;
}

static struct config_var* config_var_lookup(int* ptr)
{
    for(struct config_var *var = _config_vars_start; var < _config_vars_end ; var++ )
    {
        if (var->value == ptr)
        {
            return var;
        }
    }

#ifdef CONFIG_MODULES
    return module_config_var_lookup(ptr);
#else
    return 0;
#endif
}

static struct config_var * get_config_var_struct(const char * name)
{
    for(struct config_var *  var = _config_vars_start ; var < _config_vars_end ; var++ )
    {
        if (streq(var->name, name))
        {
            return var;
        }
    }
    
#ifdef CONFIG_MODULES
    return module_get_config_var(name);
#else
    return 0;
#endif
}

int get_config_var(const char * name)
{
    struct config_var * var = get_config_var_struct(name);
    
    if(var && var->value)
    {
        return *(var->value);
    }
    
    return 0;
}

static int set_config_var_struct(struct config_var * var, int new_value)
{
    if(var && var->value)
    {
        //check if the callback routine exists
        if(var->update)
        {
            //run the callback routine
            int cbr_result = var->update(var, *(var->value), new_value);
            //if the cbr returns false, it means we are not allowed to change the value
            if(cbr_result)
            {
                *(var->value) = new_value;
                return cbr_result;
            }
        }
        else
        {
            //no cbr so just set the value
            *(var->value) = new_value;
            return 1;
        }
    }
    
    return 0;
}

int set_config_var(const char * name, int new_value)
{
    return set_config_var_struct(get_config_var_struct(name), new_value);
}

int set_config_var_ptr(int* ptr, int new_value)
{
    struct config_var * var = config_var_lookup(ptr);
    
    if(ptr && !var)
    {
        //this is not actually a config var, so just set the value
        *ptr = new_value;
        return 1;
    }
    
    return set_config_var_struct(var, new_value);
}


int config_var_was_changed(int* ptr)
{
    struct config_var * var = config_var_lookup(ptr);
    if (!var) return 0;
    return var->default_value != *(var->value);
}

int config_var_restore_default(int* ptr)
{
    struct config_var * var = config_var_lookup(ptr);
    if (!var) return 0;
    *(var->value) = var->default_value;
    return 1;
}

#ifdef CONFIG_MODULES

/** module config files */

static void
module_config_parse(module_entry_t * module) {
    char line_buf[ 1000 ];

    while( read_line(line_buf, sizeof(line_buf) ) >= 0 )
    {
        // Ignore any line that begins with # or is empty
        if( line_buf[0] == '#'
        ||  line_buf[0] == '\0' )
            continue;
        
        struct config * new_config = config_parse_line( line_buf );
        if( !new_config ) return;

        /* check for all registered config variables from this module */
        for (module_config_t * mconfig = module->config; mconfig && mconfig->name; mconfig++)
        {
            /* check for config variable with the same name */
            if(streq(new_config->name, mconfig->ref->name))
                *mconfig->ref->value = atoi(new_config->value);
        }
    }
}

unsigned int module_config_load(char *filename, module_entry_t *module)
{
    if (!module->config)
        return -1;
    
    config_file_buf = (void*)read_entire_file(filename, &config_file_size);
    if (!config_file_buf)
        return -1;
    config_file_pos = 0;
    module_config_parse(module);
    free_dma_memory(config_file_buf);
    config_file_buf = 0;
    return 0;
}

unsigned int module_config_save(char *filename, module_entry_t *module)
{
    if (!module->config)
        return -1;

    char* msg = alloc_dma_memory(MAX_SIZE);
    msg[0] = '\0';

    snprintf( msg, MAX_SIZE,
        "# Config file for module %s (%s)\n\n",
        module->name, module->filename
    );
    
    int count = 0;
    for (module_config_t * mconfig = module->config; mconfig && mconfig->name; mconfig++)
    {
        if (*(int*)mconfig->ref->value == mconfig->ref->default_value)
            continue;

        snprintf(msg + strlen(msg), MAX_SIZE - strlen(msg) - 1,
            "%s = %d\r\n",
            mconfig->ref->name,
            *(int*) mconfig->ref->value
        );
        
        count++;
    }
    
    if (count == 0)
    {
        /* everything is default, just delete the config file */
        FIO_RemoveFile(filename);
        goto finish;
    }
    
    FILE * file = FIO_CreateFile( filename );
    if( file == INVALID_PTR )
    {
        free_dma_memory(msg);
        return -1;
    }
    
    FIO_WriteFile(file, msg, strlen(msg));

    FIO_CloseFile( file );
finish:
    free_dma_memory(msg);
    return 0;
}

#endif


#ifdef CONFIG_CONFIG_FILE
#ifdef CONFIG_PICOC
static char last_preset_file[50] = "";
static int preset_just_saved = 0;
static int preset_scripts_dirty = 0;

static char *find_picoc_config_filename()
{
    for (int i = 0; i < 10; i++)
    {
        snprintf(last_preset_file, sizeof(last_preset_file), "ML/SCRIPTS/PRESET%d.C", i);

        if (GetFileSize(last_preset_file) == 0xFFFFFFFF) // this file does not exist
            return last_preset_file;
    }
    return 0;
}

// if the user tries to save more presets at a time,
// he will fill the script directory with identical files
// so.. let's calm him down :)
static int preset_user_angry = 0;

static void config_save_as_picoc(void* priv, int delta)
{
    if (preset_just_saved)
    {
        preset_user_angry = 1;
        return;
    }

    char* fn = find_picoc_config_filename();
    if (fn)
    {
        menu_save_current_config_as_picoc_preset(fn);
        preset_just_saved = 1;
        preset_scripts_dirty = 1;
    }
}

static MENU_UPDATE_FUNC(config_save_as_picoc_update)
{
    static int last_displayed = 0;
    int t = get_ms_clock_value_fast();

    if (preset_just_saved == 2 && t - last_displayed > 2000) // if this menu was not displayed for a while, we can save a new preset
    {
        preset_just_saved = 0;
        preset_user_angry = 0;
    }

    if (preset_scripts_dirty)
    {
        MENU_SET_RINFO("Restart");
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Restart camera so the new preset appears in Scripts menu.");
    }

    if (preset_just_saved)
    {
        MENU_SET_NAME(last_preset_file + strlen("ML/SCRIPTS/"));
        if (preset_user_angry)
            MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Change some settings before saving a new preset.");
        last_displayed = t;
        preset_just_saved = 2;
    }
}

#endif // picoc

static MENU_UPDATE_FUNC(delete_config_update)
{
    if (config_deleted)
    {
        MENU_SET_RINFO("Restart");
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Restart your camera to complete the process.");
    }

    MENU_SET_HELP("Only the current preset: %s", get_config_dir());
}

static MENU_UPDATE_FUNC(config_save_update)
{
    if (config_deleted)
    {
        MENU_SET_RINFO("Undo");
    }
    
    MENU_SET_HELP("%s", get_config_dir());
}

static void delete_config( void * priv, int delta )
{
    char* path = get_config_dir();
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( path, &file );
    if( IS_ERROR(dirent) )
        return;

    do
    {
        if (file.mode & ATTR_DIRECTORY)
        {
            continue; // is a directory
        }
        
        char fn[0x80];
        snprintf(fn, sizeof(fn), "%s%s", path, file.name);
        FIO_RemoveFile(fn);
    }
    while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_FindClose(dirent);

    config_deleted = 1;
    
    if (config_autosave)
    {
        /* at shutdown, config autosave may re-create the config files we just deleted */
        /* => disable this feature in RAM only, until next reboot, without commiting it to card */
        config_autosave = 0;
    }
}

/* config presets */

static const char* config_preset_file = 
    "ML/SETTINGS/CURRENT.SET";    /* contains the name of current preset */
static int config_preset_index = 0;         /* preset being used right now */
static int config_new_preset_index = 0;     /* preset that will be used after restart */
static int config_preset_num = 3;           /* total presets available */
static char* config_preset_choices[16] = {  /* preset names (reusable as menu choices) */
    "OFF",
    "Startup mode",
    "Startup key",
    "Preset 1   ",
    "Preset 2   ",
    "Preset 3   ",
    "Preset 4   ",
    "Preset 5   ",
    "Preset 6   ",
    "Preset 7   ",
    "Preset 8   ",
    "Preset 9   ",
    "Preset 10  ",
    "Preset 11  ",
    "Preset 12  ",
    "Preset 13  ", /* space needed: 8.3 */
};

static char config_dir[0x80];
static char* config_preset_name;

char* get_config_dir()
{
    return config_dir;
}

/* null if no preset */
char* get_config_preset_name()
{
    return config_preset_name;
}

static struct menu_entry cfg_menus[];

static void config_preset_scan()
{
    char* path = "ML/SETTINGS/";
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( path, &file );
    if(!IS_ERROR(dirent))
    {
        do
        {
            if (file.mode & ATTR_DIRECTORY)
            {
                if (file.name[0] == '.')
                    continue;
                
                /* special names for keys pressed at startup */
                if (streq(file.name + strlen(file.name)-4, ".KEY"))
                    continue;

                /* special names for mode-based config presets */
                if (streq(file.name + strlen(file.name)-4, ".MOD"))
                    continue;
                
                /* we have reserved statically 12 chars for each preset */
                snprintf(
                    config_preset_choices[config_preset_num], 12,
                    "%s", file.name
                );
                config_preset_num++;
                if (config_preset_num >= COUNT(config_preset_choices))
                    break;
            }
        }
        while( FIO_FindNextEx( dirent, &file ) == 0);
        FIO_FindClose(dirent);
    }
    
    /* update the Config Presets menu */
    cfg_menus[0].children[0].max = config_preset_num - 1;
}

static MENU_SELECT_FUNC(config_save_select)
{
    config_save();
}

static MENU_SELECT_FUNC(config_preset_toggle)
{
    menu_numeric_toggle(&config_new_preset_index, delta, 0, config_preset_num);
    
    if (!config_new_preset_index)
    {
        FIO_RemoveFile(config_preset_file);
    }
    else
    {
        FILE* f = FIO_CreateFile(config_preset_file);
        if (config_new_preset_index == 1)
            my_fprintf(f, "Startup mode");
        else if (config_new_preset_index == 2)
            my_fprintf(f, "Startup key");
        else
            my_fprintf(f, "%s", config_preset_choices[config_new_preset_index]);
        FIO_CloseFile(f);
    }
}

static MENU_UPDATE_FUNC(config_preset_update)
{
    int preset_changed = (config_new_preset_index != config_preset_index);
    char* current_preset_name = get_config_preset_name();
    MENU_SET_RINFO(current_preset_name);

    if (config_new_preset_index == 1) /* startup shooting mode */
    {
        char current_mode_name[9];
        snprintf(current_mode_name, sizeof(current_mode_name), "%s", (char*) get_shootmode_name(shooting_mode_custom));
        if (streq(config_selected_by_mode, current_mode_name))
        {
            MENU_SET_HELP("Config preset is selected by startup mode (on the mode dial).");
        }
        else
        {
            MENU_SET_RINFO("%s->%s", current_preset_name, current_mode_name);
            if (config_selected_by_mode[0])
            {
                MENU_SET_HELP("Camera was started in %s; restart to load the config for %s.", config_selected_by_mode, current_mode_name);
            }
            else
            {
                MENU_SET_HELP("Restart to load the config for %s mode.", current_mode_name);
            }
        }
    }
    else if (config_new_preset_index == 2) /* startup key */
    {
        MENU_SET_HELP("At startup, press&hold MENU/PLAY/"INFO_BTN_NAME" to select the cfg preset.");
    }
    else /* named preset */
    {
        if (preset_changed)
        {
            MENU_SET_HELP("The new config preset will be used after you restart your camera.");
            MENU_SET_RINFO("Restart");
        }
    }
}

int handle_select_config_file_by_key_at_startup(struct event * event)
{
    if (!config_selected)
    {
        char* key_name = 0;
        switch (event->param)
        {
            case BGMT_MENU:
                key_name = "MENU";
                break;
            case BGMT_INFO:
                key_name = INFO_BTN_NAME;
                break;
            case BGMT_PLAY:
                key_name = "PLAY";
                break;
        }
        if (key_name)
        {
            /* we are not able to check the filesystem at this point */
            snprintf(config_selected_by_key, sizeof(config_selected_by_key), "%s", key_name);
            return 0;
        }
    }
    
    return 1;
}

static char* config_choose_startup_preset()
{
    int size = 0;

    /* by default, work in ML/SETTINGS dir */
    snprintf(config_dir, sizeof(config_dir), "ML/SETTINGS/");

    /* check for a preset file selected in menu */
    char* preset_name = (char*) read_entire_file(config_preset_file, &size);
    if (preset_name)
    {
        if (streq(preset_name, "Startup mode"))
        {
            /* will handle later */
            config_preset_index = config_new_preset_index = 1;
        }
        else if (streq(preset_name, "Startup key"))
        {
            /* will handle later */
            config_preset_index = config_new_preset_index = 2;
        }
        else
        {
            snprintf(config_selected_by_name, sizeof(config_selected_by_name), preset_name);
            char preset_dir[0x80];
            snprintf(preset_dir, sizeof(preset_dir), "ML/SETTINGS/%s", preset_name);
            if (!is_dir(preset_dir)) { FIO_CreateDirectory(preset_dir); }
            if (is_dir(preset_dir))
            {
                snprintf(config_dir, sizeof(config_dir), "%s/", preset_dir);
            }
        }
        free_dma_memory(preset_name);
    }

    /* scan the preset files and populate the menu */
    config_preset_scan();

    /* special cases: key pressed at startup, or startup mode */

    /* key pressed at startup */
    if (config_preset_index == 2)
    {
        if (config_selected_by_key[0])
        {
            char preset_dir[0x80];
            snprintf(preset_dir, sizeof(preset_dir), "ML/SETTINGS/%s.KEY", config_selected_by_key);
            if (!is_dir(preset_dir)) { FIO_CreateDirectory(preset_dir); }
            if (is_dir(preset_dir))
            {
                /* success */
                snprintf(config_dir, sizeof(config_dir), "%s/", preset_dir);
                return config_selected_by_key;
            }
        }
        /* didn't work */
        return 0;
    }
    else config_selected_by_key[0] = 0;

    /* startup shooting mode (if selected in menu) */
    if (config_preset_index == 1)
    {
        snprintf(config_selected_by_mode, sizeof(config_selected_by_mode), "%s", get_shootmode_name(shooting_mode_custom));
        char preset_dir[0x80];
        snprintf(preset_dir, sizeof(preset_dir), "ML/SETTINGS/%s.MOD", config_selected_by_mode);
        if (!is_dir(preset_dir)) { FIO_CreateDirectory(preset_dir); }
        if (is_dir(preset_dir))
        {
            /* success */
            snprintf(config_dir, sizeof(config_dir), "%s/", preset_dir);
            return config_selected_by_mode;
        }
        /* didn't work */
        return 0;
    }

    /* lookup the current preset in menu */
    for (int i = 0; i < config_preset_num; i++)
    {
        if (streq(config_preset_choices[i], config_selected_by_name))
        {
            config_preset_index = config_new_preset_index = i;
            return config_selected_by_name;
        }
    }

    /* using default config */
    return 0;
}


static struct menu_entry cfg_menus[] = {
{
    .name = "Config files",
    .select = menu_open_submenu,
    .update = config_preset_update,
    .submenu_width = 710,
    .help = "Config auto save, manual save, restore defaults...",
    .children =  (struct menu_entry[]) {
        {
            .name = "Config preset",
            .priv = &config_new_preset_index,
            .min = 0,
            .max = 2,
            .choices = (const char **) config_preset_choices,
            .select = config_preset_toggle,
            .update = config_preset_update,
            .help = "Choose a configuration preset."
        },
        {
            .name = "Config AutoSave",
            .priv = &config_autosave,
            .max  = 1,
            .select = config_autosave_toggle,
            .help = "If enabled, ML settings are saved automatically at shutdown."
        },
        {
            .name = "Save config now",
            .select        = config_save_select,
            .update        = config_save_update,
            .help = "Save ML settings to current preset directory."
        },
        {
            .name = "Restore ML defaults",
            .select        = delete_config,
            .update        = delete_config_update,
            .help  = "This restores ML default settings, by deleting all CFG files.",
        },

        #ifdef CONFIG_PICOC
        {
            .name = "Export as PicoC script",
            .select = config_save_as_picoc,
            .update = config_save_as_picoc_update,
            .help =  "Export current menu settings to ML/SCRIPTS/PRESETn.C.",
            .help2 = "The preset will appear in Scripts menu. Edit/rename on PC.",
        },
        #endif

        MENU_EOL,
    },
},
};
#endif

/* called at startup, after init_func's */
void config_load()
{
#ifdef CONFIG_CONFIG_FILE
    config_selected = 1;
    config_preset_name = config_choose_startup_preset();

    if (config_preset_name)
    {
        NotifyBox(2000, "Config: %s", config_preset_name);
        if (!DISPLAY_IS_ON) beep();
    }
    
    char config_file[0x80];
    snprintf(config_file, sizeof(config_file), "%smagic.cfg", get_config_dir());
    config_parse_file(config_file);
#endif

    config_ok = 1;
}

// this can be called from more tasks (gui, prop handler, menu), so it needs to be thread safe
void config_save()
{
#ifdef CONFIG_CONFIG_FILE
    take_semaphore(config_save_sem, 0);
    update_disp_mode_bits_from_params();
    char config_file[0x80];
    snprintf(config_file, sizeof(config_file), "%smagic.cfg", get_config_dir());
    config_save_file(config_file);
    config_menu_save_flags();
    module_save_configs();
    if (config_deleted) config_autosave = 1; /* this can be improved, because it's not doing a proper "undo" */
    config_deleted = 0;
    give_semaphore(config_save_sem);
#endif
}

void config_save_at_shutdown()
{
#ifdef CONFIG_CONFIG_FILE
    static int config_saved = 0;
    if (config_ok && config_autosave && !config_saved)
    {
        config_saved = 1;
        config_save();
        msleep(100);
    }
#endif
}

static void config_menu_init()
{
#ifdef CONFIG_CONFIG_FILE
    menu_add( "Prefs", cfg_menus, COUNT(cfg_menus) );
    config_save_sem = create_named_semaphore("config_save_sem",1);
#endif
}

INIT_FUNC("config", config_menu_init);
