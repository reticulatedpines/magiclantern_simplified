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

// Don't use isspace since we don't have it
static inline int
is_space( char c )
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static struct config *
config_parse_line(
    const char *        line
)
{
    int name_len = 0;
    int value_len = 0;
    static struct config _cfg;
    struct config * cfg = &_cfg;

    // Trim any leading whitespace
    int i = 0;
    while( line[i] && is_space( line[i] ) )
        i++;

    // Copy the name to the name buffer
    while( line[i]
    && !is_space( line[i] )
    && line[i] != '='
    && name_len < MAX_NAME_LEN
    )
        cfg->name[ name_len++ ] = line[i++];

    if( name_len == MAX_NAME_LEN )
        goto parse_error;

    // And nul terminate it
    cfg->name[ name_len ] = '\0';

    // Skip any white space and = signs
    while( line[i] && is_space( line[i] ) )
        i++;
    if( line[i++] != '=' )
        goto parse_error;
    while( line[i] && is_space( line[i] ) )
        i++;

    // Copy the value to the value buffer
    while( line[i] && value_len < MAX_VALUE_LEN )
        cfg->value[ value_len++ ] = line[ i++ ];

    // Back up to trim any white space
    while( value_len > 0 && is_space( cfg->value[ value_len-1 ] ) )
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

char* config_file_buf = 0;
int config_file_size = 0;
int config_file_pos = 0;
static int get_char_from_config_file(char* out)
{
    if (config_file_pos >= config_file_size) return 0;
    *out = config_file_buf[config_file_pos++];
    return 1;
}

int
read_line(
    char *          buf,
    size_t          size
)
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


extern struct config_var    _config_vars_start[];
extern struct config_var    _config_vars_end[];

static void
config_auto_parse(
    struct config *     cfg
)
{
    struct config_var *     var = _config_vars_start;

    for( ; var < _config_vars_end ; var++ )
    {
#if defined(POSITION_INDEPENDENT)
        var->name = PIC_RESOLVE(var->name);
        var->value = PIC_RESOLVE(var->value);
#endif
        if( !streq( var->name, cfg->name ) )
            continue;

        DebugMsg( DM_MAGIC, 3,
            "%s: '%s' => '%s'",
            __func__,
            cfg->name,
            cfg->value
        );

        if( var->type == 0 )
        {
            *(unsigned*) var->value = atoi( cfg->value );
        } else {
            *(char **) var->value = cfg->value;
        }

        return;
    }

    DebugMsg( DM_MAGIC, 3,
        "%s: '%s' unused?",
        __func__,
        cfg->name
    );
}


int
config_save_file(
    const char *        filename
)
{
    struct config_var * var = _config_vars_start;
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

    for( ; var < _config_vars_end ; var++ )
    {
        if( var->type == 0 )
            snprintf(msg + strlen(msg), MAX_SIZE - strlen(msg) - 1,
                "%s = %d\r\n",
                var->name,
                *(unsigned*) var->value
            );
        else
            snprintf(msg + strlen(msg), MAX_SIZE - strlen(msg) - 1,
                "%s = %s\r\n",
                var->name,
                *(const char**) var->value
            );

        count++;
    }
    
    FILE * file = FIO_CreateFileEx( filename );
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


static struct config *
config_parse() {
    char line_buf[ 1000 ];
    struct config * cfg = 0;
    int count = 0;

    while( read_line(line_buf, sizeof(line_buf) ) >= 0 )
    {
        //~ bmp_printf(FONT_SMALL, 0, 0, "cfg line: %s      ", line_buf);
        
        // Ignore any line that begins with # or is empty
        if( line_buf[0] == '#'
        ||  line_buf[0] == '\0' )
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
#define CONFIG_AUTOSAVE_FLAG_FILE CARD_DRIVE "ML/SETTINGS/AUTOSAVE.NEG"

static int config_flag_file_setting_load(char* file)
{
    uint32_t size;
    return ( FIO_GetFileSize( file, &size ) == 0 );
}

static void config_flag_file_setting_save(char* file, int setting)
{
    FIO_RemoveFile(file);
    if (setting)
    {
        FILE* f = FIO_CreateFileEx(file);
        FIO_CloseFile(f);
    }
}

void
config_autosave_toggle(void* priv)
{
    config_flag_file_setting_save(CONFIG_AUTOSAVE_FLAG_FILE, !!config_autosave);
    msleep(50);
    config_autosave = !config_flag_file_setting_load(CONFIG_AUTOSAVE_FLAG_FILE);
}

int
config_parse_file(
    const char *        filename
)
{
    config_autosave = !config_flag_file_setting_load(CONFIG_AUTOSAVE_FLAG_FILE);

    config_file_buf = (void*)read_entire_file(filename, &config_file_size);
    if (!config_file_buf)
    {
        // if config file is not present, force Config Autosave: On
        if (!config_autosave) config_autosave_toggle(0);
        return 0;
    }
    config_file_pos = 0;
    config_parse();
    free_dma_memory(config_file_buf);
    config_file_buf = 0;
    return 1;
}



int
atoi(
    const char *        s
)
{
    int value = 0;
    int sign = 1;

    // Only handles base ten for now
    while( 1 )
    {
        char c = *s++;
        if (c == '-')
        {
            sign = -1;
            continue;
        }
        if( !c || c < '0' || c > '9' )
            break;
        value = value * 10 + c - '0';
    }

    return value * sign;
}

struct config_var* get_config_vars_start() {
	return _config_vars_start;
}

struct config_var* get_config_vars_end() {
	return _config_vars_end;
}

