// simple line-based text file read routines

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

char* config_file_buf = 0;
int config_file_size = 0;
int config_file_pos = 0;
int get_char_from_config_file(char* out)
{
	if (config_file_pos >= config_file_size) return 0;
	*out = config_file_buf[config_file_pos++];
	return 1;
}

int
read_line(
	char *			buf,
	size_t			size
)
{
	size_t			len = 0;

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


extern struct config_var	_config_vars_start[];
extern struct config_var	_config_vars_end[];

static void
config_auto_parse(
	struct config *		cfg
)
{
	struct config_var *		var = _config_vars_start;

	for( ; var < _config_vars_end ; var++ )
	{
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
	const char *		filename
)
{
	struct config_var * var = _config_vars_start;
	int count = 0;

	DebugMsg( DM_MAGIC, 3, "%s: saving to %s", __func__, filename );
	
	#define MAX_SIZE 10240
	char* msg = alloc_dma_memory(MAX_SIZE);
	char* msgc = CACHEABLE(msg);
	
	snprintf( msgc, MAX_SIZE,
		"# Magic Lantern %s (%s)\n"
		"# Build on %s by %s\n",
		build_version,
		build_id,
		build_date,
		build_user
	);

	struct tm now;
	LoadCalendarFromRTC( &now );

	snprintf(msgc + strlen(msgc), MAX_SIZE - strlen(msgc),
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
			snprintf(msgc + strlen(msgc), MAX_SIZE - strlen(msgc),
				"%s = %d\r\n",
				var->name,
				*(unsigned*) var->value
			);
		else
			snprintf(msgc + strlen(msgc), MAX_SIZE - strlen(msgc),
				"%s = %s\r\n",
				var->name,
				*(const char**) var->value
			);

		count++;
	}
	
	FIO_RemoveFile(filename);
	FILE * file = FIO_CreateFile( filename );
	if( file == INVALID_PTR )
		return -1;
	
	FIO_WriteFile(file, msg, strlen(msgc));

	FIO_CloseFile( file );
	return count;
}


struct config *
config_parse() {
	char line_buf[ 1000 ];
	struct config *	cfg = 0;
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

int
config_parse_file(
	const char *		filename
)
{
	config_file_buf = read_entire_file(filename, &config_file_size);
	config_file_pos = 0;
	config_parse();
	free_dma_memory(config_file_buf);
	config_file_buf = 0;
	return 1;
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

int config_flag_file_setting_load(char* file)
{
	unsigned size;
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
