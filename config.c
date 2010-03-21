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

// Don't use isspace since we don't have it
static inline int
is_space( char c )
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

struct config *
config_parse_line(
	const char *		line
)
{
	int name_len = 0;
	int value_len = 0;
	struct config *		config = malloc( sizeof(*config) );
	if( !config )
		goto malloc_error;

	config->next = 0;

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
		config->name[ name_len++ ] = line[i++];

	if( name_len == MAX_NAME_LEN )
		goto parse_error;

	// And nul terminate it
	config->name[ name_len ] = '\0';

	// Skip any white space and = signs
	while( line[i] && is_space( line[i] ) )
		i++;
	if( line[i++] != '=' )
		goto parse_error;
	while( line[i] && is_space( line[i] ) )
		i++;

	// Copy the value to the value buffer
	while( line[i] && value_len < MAX_VALUE_LEN )
		config->value[ value_len++ ] = line[ i++ ];

	// Back up to trim any white space
	while( value_len > 0 && is_space( config->value[ value_len-1 ] ) )
		value_len--;

	// And nul terminate it
	config->value[ value_len ] = '\0';

	DebugMsg( DM_MAGIC, 3,
		"%s: '%s' => '%s'",
		__func__,
		config->name,
		config->value
	);

	return config;

parse_error:
	DebugMsg( DM_MAGIC, 3,
		"%s: PARSE ERROR: len=%d,%d string='%s'",
		__func__,
		name_len,
		value_len,
		line
	);

	free( config );
malloc_error:
	return 0;
}


int
read_line(
	FILE *			file,
	char *			buf,
	size_t			size
)
{
	size_t			len = 0;

	while( len < size )
	{
		int rc = FIO_ReadFile( file, buf+len, 1 );
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


extern struct config_var	_config_vars_start[];
extern struct config_var	_config_vars_end[];

static void
config_auto_parse(
	struct config *		config
)
{
	struct config_var *		var = _config_vars_start;

	for( ; var < _config_vars_end ; var++ )
	{
		if( !streq( var->name, config->name ) )
			continue;

		DebugMsg( DM_MAGIC, 3,
			"%s: '%s' => '%s'",
			__func__,
			config->name,
			config->value
		);

		if( var->type == 0 )
		{
			*(unsigned*) var->value = atoi( config->value );
		} else {
			*(char **) var->value = config->value;
		}

		return;
	}

	DebugMsg( DM_MAGIC, 3,
		"%s: '%s' unused?",
		__func__,
		config->name
	);
}


int
config_save_file(
	struct config *		config,
	const char *		filename
)
{
	FILE * file = FIO_CreateFile( filename );
	if( file == INVALID_PTR )
		return -1;

	struct config_var * var = _config_vars_start;
	int count = 0;

	DebugMsg( DM_MAGIC, 3, "%s: saving to %s", __func__, filename );

	fprintf( file,
		"# Magic Lantern %s (%s)\n"
		"# Build on %s by %s\n",
		build_version,
		build_id,
		build_date,
		build_user
	);

	struct tm now;
	LoadCalendarFromRTC( &now );

	fprintf( file,
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
			fprintf( file,
				"%s = %d\n",
				var->name,
				*(unsigned*) var->value
			);
		else
			fprintf( file,
				"%s = %s\n",
				var->name,
				*(const char**) var->value
			);

		count++;
	}

	FIO_CloseFile( file );
	return count;
}


struct config *
config_parse(
	FILE *			file
) {
	char line_buf[ MAX_NAME_LEN + MAX_VALUE_LEN ];
	struct config *	config = 0;
	int count = 0;

	while( read_line( file, line_buf, sizeof(line_buf) ) >= 0 )
	{
		// Ignore any line that begins with # or is empty
		if( line_buf[0] == '#'
		||  line_buf[0] == '\0' )
			continue;

		struct config * new_config = config_parse_line( line_buf );
		if( !new_config )
			goto error;

		new_config->next = config;
		config = new_config;
		count++;

		config_auto_parse( config );
	}

	DebugMsg( DM_MAGIC, 3, "%s: Read %d config values", __func__, count );
	return config;

error:
	DebugMsg( DM_MAGIC, 3, "%s: ERROR Deleting config", __func__ );
	while( config )
	{
		struct config * next = config->next;
		DebugMsg( DM_MAGIC, 3, "%s: Deleting '%s' => '%s'",
			__func__,
			config->name,
			config->value
		);
		free( config );
		config = next;
	}

	return NULL;
}


char *
config_value(
	struct config *		config,
	const char *		name
)
{
	while( config )
	{
		if( streq( config->name, name ) )
			return config->value;

		config = config->next;
	}

	return NULL;
}


int
config_int(
	struct config *		config,
	const char *		name,
	int			def
)
{
	const char *		str = config_value( config, name );
	if( !str )
	{
		DebugMsg( DM_MAGIC, 3,
			"Config '%s', using default %d",
			name,
			def
		);

		return def;
	}

	def = atoi( str );
	DebugMsg( DM_MAGIC, 3,
		"Config '%s', using user value %d ('%s')",
		name,
		def,
		str
	);

	return def;
}


struct config head = { .name = "config.file", .value = "" };
struct config fail = { .name = "config.failure", .value = "1" };

struct config *
config_parse_file(
	const char *		filename
)
{
	FILE * file = FIO_Open( filename, O_SYNC );
	strcpy( head.value, filename );
	if( file == INVALID_PTR )
	{
		head.next = &fail;
		return &head;
	}

	struct config * config = config_parse( file );
	FIO_CloseFile( file );
	head.next = config;
	return &head;
}



int
atoi(
	const char *		s
)
{
	int value = 0;

	// Only handles base ten for now
	while( 1 )
	{
		char c = *s++;
		if( !c || c < '0' || c > '9' )
			break;
		value = value * 10 + c - '0';
	}

	return value;
}

