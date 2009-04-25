/*
 * Imported from dump_toolkit
 */
#include <stdint.h>

#define ROM0_ADDRESS	0xF8000000
#define ROM0_SIZE	0x800000
#define ROM1_ADDRESS	0xF0000000
#define ROM1_SIZE	0x800000

#define O_WRONLY        0x001
#define O_CREAT         0x200

#pragma long_calls
extern void * open(const char *name, int flags, int mode);	// mode is for VxW only
extern int close(void * file);
extern int write(void * file, void * buffer, unsigned long nbytes);
extern void * creat(char *name, int flags);
extern void shutdown(void);
#pragma no_long_calls

#define INVALID_HANDLE	((void *)0xFFFFFFFF)
#define NULL			((void *)0)

#define LOG() \
	do { \
		uint32_t error = __LINE__; \
		if( logfile != INVALID_HANDLE ) \
			write( logfile, &error, sizeof(error) ); \
	} while(0)

int main( void )
{
	void * const	rom_start	= (void *) ROM0_ADDRESS;
	uint32_t	rom_size	= ROM0_SIZE;
	void * const	rom1_start	= (void *) ROM1_ADDRESS;
	uint32_t	rom1_size	= ROM1_SIZE;

	char fname[12];

	fname[0] = 'A'; fname[1] = ':'; fname[2] = '/';
	fname[3] = 'R'; fname[4] = 'O'; fname[5] = 'M'; fname[6] = '0';
	fname[7] = '.';
	fname[8] = 'l'; fname[9] = 'o'; fname[10] = 'g';
	fname[11] = '\0';
	

	const uint32_t model_id = *(uint32_t*) 0x800000;

	// Verify that we are on a 5D
	if( model_id != 0x80000218 )
		while(1);

	void * logfile = open( fname, O_WRONLY | O_CREAT, 0666 );
	if( logfile == INVALID_HANDLE )
		logfile = creat( fname, O_WRONLY );
	if( logfile == INVALID_HANDLE )
		shutdown();
	write( logfile, &model_id, sizeof(model_id) );

	// Change the name to .bin
	fname[8] = 'b'; fname[9] = 'i'; fname[10] = 'n'; fname[11] = '\0';

	void *f = open(fname, O_CREAT|O_WRONLY, 0644);
	if( INVALID_HANDLE == f )
	{
		LOG();
		f = creat( fname, O_WRONLY );	// O_CREAT is not working in DRYOS
	}
	if( INVALID_HANDLE == f )
	{
		LOG();
		shutdown();
	}

	write( f, rom_start, rom_size);
	close( f );

	if( rom1_size == 0 )
	{
		LOG();
		shutdown();
	}

	// Change the name from ROM0 to ROM1
	fname[6] = '1';
	f = open(fname, O_CREAT|O_WRONLY, 0644);
	if( INVALID_HANDLE == f )
	{
		LOG();
		f = creat( fname, O_WRONLY );	// O_CREAT is not working in DRYOS
	}
	if( INVALID_HANDLE == f )
	{
		LOG();
		shutdown();
	}

	LOG();
	write(f, rom1_start, rom1_size);
	close(f);

	LOG();
	close( logfile );
	shutdown();

	// If shutdown didn't work, we spin
	LOG();
	close( logfile );

	while(1);
	return 0;
}
