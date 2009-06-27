/** \file
 * Key/value parser until we have a proper config
 */
#include "dryos.h"
#include "config.h"

// Don't use isspace since we don't have it
static inline int
is_space( char c )
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Don't use strcmp since we don't have it
static inline int
streq( const char * a, const char * b )
{
	while( *a && *b )
		if( *a++ != *b++ )
			return -1;
	return *a == *b;
}

struct config *
config_parse_line(
	const char *		line
)
{
	struct config *		config = malloc( sizeof(*config) );
	if( !config )
		goto malloc_error;

	config->next = 0;

	// Trim any leading whitespace
	int i = 0;
	while( line[i] && is_space( line[i] ) )
		i++;

	// Copy the name to the name buffer
	int name_len = 0;
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
	int value_len = 0;
	while( line[i] && value_len < MAX_VALUE_LEN )
		config->value[ value_len++ ] = line[ i++ ];

	// Back up to trim any white space
	while( value_len > 0 && is_space( config->value[ value_len-1 ] ) )
		value_len--;

	// And nul terminate it
	config->value[ value_len ] = '\0';

	return config;

parse_error:
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



struct config *
config_parse(
	FILE *			file
) {
	char line_buf[ MAX_NAME_LEN + MAX_VALUE_LEN ];
	struct config *	config = 0;

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
	}

	return config;

error:
	while( config )
	{
		struct config * next = config->next;
		free( config );
		config = next;
	}

	return NULL;
}


struct config *
config_value(
	struct config *		config,
	const char *		name
)
{
	while( config )
	{
		if( streq( config->name, name ) )
			return config;

		config = config->next;
	}

	return NULL;
}

struct config head = { .name = "config.file", .value = "" };
struct config fail = { .name = "config.failure", .value = "1" };

struct config *
config_parse_file(
	const char *		filename
)
{
	FILE * file = FIO_Open( filename, 0 );
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
