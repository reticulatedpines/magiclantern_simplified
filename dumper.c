/*
 * Imported from dump_toolkit
 */
#define ROM0_ADDRESS	0xF8000000
#define ROM0_SIZE	0x800000
#define ROM1_ADDRESS	0xF0000000
#define ROM1_SIZE	0x800000

#define O_WRONLY        1
#define O_CREAT         0x200


typedef void * (*ft_open)(const char *name, int flags, int mode);	// mode is for VxW only
typedef int (*ft_close)(void * file);
typedef int (*ft_write)(void * file, void * buffer, unsigned long nbytes);
typedef void * (*ft_creat)(char *name, int flags);
typedef void *(*ft_shutdown)(void);

#define INVALID_HANDLE	((void *)0xFFFFFFFF)
#define NULL			((void *)0)

int main( void )
{
	void * rom_start;
	void * rom1_start = NULL;
	unsigned long rom_size, rom1_size;
	ft_open open;
	ft_creat creat;
	ft_write write;
	ft_close close;
	ft_shutdown shdn = (ft_shutdown)0;
	char fname[12];

	void *f = (void *)0;


	fname[0] = 'A'; fname[1] = ':'; fname[2] = '/';
	fname[3] = 'R'; fname[4] = 'O'; fname[5] = 'M';
	fname[6] = '0'; fname[7] = '.'; fname[8] = 'b';
	fname[9] = 'i'; fname[10] = 'n'; fname[11] = '\0';
	

	switch( *(unsigned long *)0x800000 ){
		case 0x80000190:	// 40D
			// 40D updater 1.1.1 params
			// 0x0080721C tUpdMgr
			open	= (ft_open)		0x00989A3C;
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
			open	= (ft_open)		0x0082DEC8;
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
			open	= (ft_open)		0x0082DF2C;
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
    f = open(fname, O_CREAT|O_WRONLY, 0644);
	if( INVALID_HANDLE == f ) f = creat( fname, O_WRONLY );	// O_CREAT is not working in DRYOS
    if( INVALID_HANDLE != f ){
            write(f, rom_start, rom_size);
            close(f);
			f = INVALID_HANDLE;
	}
	if( rom1_size ){
		fname[6] = '1';
		f = open(fname, O_CREAT|O_WRONLY, 0644);
		if( INVALID_HANDLE == f ) f = creat( fname, O_WRONLY );	// O_CREAT is not working in DRYOS
		if (INVALID_HANDLE !=f ){
				write(f, rom1_start, rom1_size);
				close(f);
		}
	}
	if( shdn ) shdn();
	while(1);
	return 0;
}
