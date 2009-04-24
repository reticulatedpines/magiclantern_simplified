/*
 * Imported from dump_toolkit
 */
#include <stdint.h>

#define ROM0_ADDRESS	0xF8000000
#define ROM0_SIZE	0x800000
#define ROM1_ADDRESS	0xF0000000
#define ROM1_SIZE	0x800000

#define O_WRONLY        1
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

	fname[8] = 'b'; fname[9] = 'i'; fname[10] = 'n'; fname[11] = '\0';
#if 0
		case 0x80000190:	// 40D
			// 40D updater 1.1.1 params
			// 0x0080721C tUpdMgr
			open	= (ft_open)	0x00989A3C;
			creat	= (ft_creat)	0x00989A4C;
			write	= (ft_write)	0x0098953C;
			close	= (ft_close)	0x009896A8;
			shdn	= (ft_shutdown)	0x00807950;
			// ROM params
			rom_start	= (void *)ROM0_ADDRESS;
			rom_size	= ROM0_SIZE;
			rom1_size	= 0;
			fname[6] = 'F';
			break;
		case 0x80000261:	// 50D
			// 50D updater 1.0.3 params
			// 0x00805A84 tUpdMgr
			open	= (ft_open)	0x0082DEC8;
			creat	= (ft_creat)	0x0082DF7C;
			write	= (ft_write)	0x0082E464;
			close	= (ft_close)	0x0082E024;
			shdn	= (ft_shutdown)	0x00806084;
			// ROM params
			rom_start	= (void *)ROM0_ADDRESS;
			rom_size	= ROM0_SIZE;
			rom1_start	= (void *)ROM1_ADDRESS;
			rom1_size	= ROM1_SIZE;
			break;
		case 0x80000218:	// 5D2
			// 5D2 updater 1.0.7 params
			// 0x00805AB8 tUpdMgr
			open	= (ft_open)	0x0082DF2C;
			creat	= (ft_creat)	0x0082DFE0;
			write	= (ft_write)	0x0082E4C8;
			close	= (ft_close)	0x0082E088;
			shdn	= (ft_shutdown)	0x008060B8;
			// ROM params
			rom_start	= (void *)ROM0_ADDRESS;
			rom_size	= ROM0_SIZE;
			rom1_start	= (void *)ROM1_ADDRESS;
			rom1_size	= ROM1_SIZE;
			break;
		default:			// Unsupported platform, hang
			while(1);
	}
#endif
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
