/** \file
 * Magic Lantern debugging and reverse engineering code
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
//#include "lua.h"

extern void bootdisk_disable();


int testvar=0;


void call_dispcheck( void * priv )
{
	call( "dispcheck" );
}


static PROP_INT( PROP_EFIC_TEMP, efic_temp );


PROP_HANDLER( PROP_HDMI_CHANGE_CODE )
{
	DebugMsg( DM_MAGIC, 3, "They try to set code to %d", buf[0] );
	return prop_cleanup( token, property );
}


static void
efic_temp_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		//23456789012
		"CMOS temp:  %ld",
		efic_temp
	);
}

static void
mvr_time_const_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
  /*
	uint8_t * mvr_struct = (void*) 0x1ed4;
	uint8_t * mvr_hdr = *(void**)( 0x1ed4 + 4 );
	struct state_object ** const mvr_state_object = (void*) 0x68a4;
  */
	struct tm now;
	LoadCalendarFromRTC( &now );

	bmp_printf(
		FONT_MED, // selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Date %4d/%2d/%2d %02d:%02d:%02d",
		now.tm_year + 1900,
		now.tm_mon,
		now.tm_mday,
		now.tm_hour,
		now.tm_min,
		now.tm_sec
	);
}



CONFIG_INT( "debug.draw-prop",		draw_prop, 0 );

static void
draw_prop_select( void * priv )
{
	draw_prop = !draw_prop;
}

CONFIG_INT( "debug.mem-spy",		mem_spy, 0 );
CONFIG_INT( "debug.mem-spy.start.lo",	mem_spy_start_lo,	0 ); // start from here
CONFIG_INT( "debug.mem-spy.start.hi",	mem_spy_start_hi,	0 ); // start from here
CONFIG_INT( "debug.mem-spy.len",	mem_spy_len,	0x1000 );         // look at ### int32's
CONFIG_INT( "debug.mem-spy.bool",	mem_spy_bool,	0 );         // only display booleans (0,1,-1)
CONFIG_INT( "debug.mem-spy.small",	mem_spy_small,	1 );         // only display small numbers (less than 10)

#define mem_spy_start ((uint32_t)mem_spy_start_lo | ((uint32_t)mem_spy_start_hi << 16))

static void
mem_spy_select( void * priv )
{
	mem_spy = !mem_spy;
}


static void
save_config( void * priv )
{
	config_save_file( global_config, "B:/magic.cfg" );
}

//----------------begin qscale-----------------
//~ CONFIG_INT( "h264.qscale", qscale, -8 );  // not reliable
CONFIG_INT( "h264.qscale.max.neg", qscale_max_neg, 1 );
CONFIG_INT( "h264.qscale.min.neg", qscale_min_neg, 16 );

int16_t qscale = 0;
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define QSCALE_MAX MAX(-(int)qscale_min_neg, -(int)qscale_max_neg) // idiot-proof :)
#define QSCALE_MIN MIN(-(int)qscale_min_neg, -(int)qscale_max_neg)
#define QSCALE_OFF (QSCALE_MAX + 1)

void mvrFixQScale(uint16_t *);
void mvrSetDefQScale(int16_t *);

void vbr_set()
{
	uint16_t param = (qscale == QSCALE_OFF) ? 0 : 1;                  // select fixed rate or VBR
	mvrFixQScale(&param);
	if (qscale != QSCALE_OFF) mvrSetDefQScale(&qscale);
}

void vbr_toggle( void * priv )
{
	qscale = MIN(qscale, QSCALE_OFF);
	qscale -= 1;
	if (qscale < QSCALE_MIN)
		qscale = QSCALE_OFF;
	vbr_set();
}

void vbr_toggle_reverse( void * priv )
{
	qscale = MIN(qscale, QSCALE_OFF);
	qscale += 1;
	if (qscale > QSCALE_OFF)
		qscale = QSCALE_MIN;
	vbr_set();
}

static void
vbr_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	if (qscale == QSCALE_OFF)
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"QScale:     OFF "
		);
	else
		bmp_printf(
			selected ? MENU_FONT_SEL : MENU_FONT,
			x, y,
			"QScale:     %d ",
			qscale
		);
}
//-------------------------end qscale--------------

static uint32_t* dbg_memmirror = 0;
static uint32_t* dbg_memchanges = 0;

static void dbg_memspy_init() // initial state of the analyzed memory
{
	//~ bmp_printf(FONT_MED, 10,10, "memspy init @ %x ... %x", mem_spy_start, mem_spy_len);
	//mem_spy_len is number of int32's
	if (!dbg_memmirror) dbg_memmirror = AllocateMemory(mem_spy_len*4 + 100); // local copy of mem area analyzed
	if (!dbg_memmirror) return;
	if (!dbg_memchanges) dbg_memchanges = AllocateMemory(mem_spy_len*4 + 100); // local copy of mem area analyzed
	if (!dbg_memchanges) return;
	int i;
	//~ bmp_printf(FONT_MED, 10,10, "memspy alloc");
	uint32_t crc = 0;
	for (i = 0; i < mem_spy_len; i++)
	{
		uint32_t addr = mem_spy_start + i*4;
		dbg_memmirror[i] = *(uint32_t*)(addr);
		dbg_memchanges[i] = 0;
		crc += dbg_memmirror[i];
	}
	//~ bmp_printf(FONT_MED, 10,10, "memspy: %x", crc);
}
static void dbg_memspy_update()
{
	if (!dbg_memmirror) return;
	if (!dbg_memchanges) return;
	int i;
	int k=0;
	for (i = 0; i < mem_spy_len; i++)
	{
		uint32_t fnt = FONT_SMALL;
		uint32_t addr = mem_spy_start + i*4;
		uint32_t oldval = dbg_memmirror[i];
		uint32_t newval = *(uint32_t*)(addr);
		if (oldval != newval)
		{
			//~ bmp_printf(FONT_MED, 10,460, "memspy: %8x: %8x => %8x", addr, oldval, newval);
			dbg_memmirror[i] = newval;
			if (dbg_memchanges[i] < 100) dbg_memchanges[i]++;
			fnt = FONT(FONT_SMALL, 5, COLOR_BG);
		}

		if (mem_spy_bool && newval != 0 && newval != 1 && newval != 0xFFFFFFFF) continue;
		if (mem_spy_small && newval > 10) continue;

		// show addresses which change, but not those which change like mad
		if (dbg_memchanges[i] > 0 && dbg_memchanges[i] < 50)
		{
			int x = 10 + 8 * 20 * (k % 4);
			int y = 10 + 12 * (k / 4);
			bmp_printf(fnt, x, y, "%8x:%2d:%8x", addr, dbg_memchanges[i], newval);
			k = (k + 1) % 120;
		}
	}
}


static void dbg_draw_props(int changed);
static unsigned dbg_last_changed_propindex = 0;
int screenshot_sec = 0;
static void
debug_loop_task( void ) // screenshot, draw_prop
{
	dbg_memspy_init();
	while(1)
	{
		if (screenshot_sec)
		{
			bmp_printf( FONT_SMALL, 0, 0, "Screenshot in 10 seconds");
			while( screenshot_sec-- )
				msleep( 1000 );
			call_dispcheck(0);
		}
		else if (draw_prop)
		{
			dbg_draw_props(dbg_last_changed_propindex);
			msleep(10);
		}
		else if (mem_spy)
		{
			dbg_memspy_update();
			msleep(10);
		}
		else msleep(1000);
	}
}

static void screenshot_start(void)
{
	screenshot_sec = 10;
}


struct menu_entry debug_menus[] = {
	{
		.display	= vbr_print,
		.select		= vbr_toggle,
		.select_reverse	= vbr_toggle_reverse,
	},
	{
		.priv		= "Save config",
		.select		= save_config,
		.display	= menu_print,
	},
	{
		.display	= efic_temp_display,
	},
	//~ {
		//~ .priv		= "Draw palette",
		//~ .select		= bmp_draw_palette,
		//~ .display	= menu_print,
	//~ },
	{
		.priv		= "Screenshot (10 s)",
		.select		= screenshot_start,
		.display	= menu_print,
	},
	{
		.priv		= "Dump dmlog",
		.select		= (void*) dumpf,
		.display	= menu_print,
	},
	{
		.priv		= "Toggle draw_prop",
		.select		= draw_prop_select,
		.display	= menu_print,
	},
	{
		.priv		= "Toggle mem_spy",
		.select		= mem_spy_select,
		.display	= menu_print,
	},

#if 0
	{
		.priv		= "Enable full HD",
		.select		= enable_full_hd,
		.display	= display_full_hd,
	},
	{
		.display	= mvr_time_const_display,
		.select		= mvr_time_const_select,
	},
#endif
};


static void * debug_token;

static void
debug_token_handler(
	void *			token,
	void *			arg1,
	void *			arg2,
	void *			arg3
)
{
	debug_token = token;
	DebugMsg( DM_MAGIC, 3, "token %08x arg=%08x %08x %08x",
		(unsigned) token,
		(unsigned) arg1,
		(unsigned) arg2,
		(unsigned) arg3
	);
}

static int dbg_propn = 0;
static unsigned dbg_props[30] = {0};
static unsigned dbg_props_len[30] = {0};
static unsigned dbg_props_a[30] = {0};
static unsigned dbg_props_b[30] = {0};
static unsigned dbg_props_c[30] = {0};
static unsigned dbg_props_d[30] = {0};
static unsigned dbg_props_e[30] = {0};
static unsigned dbg_props_f[30] = {0};
static void dbg_draw_props(int changed)
{
	dbg_last_changed_propindex = changed;
	int i; 
	for (i = 0; i < dbg_propn; i++)
	{
		unsigned x = 80;
		unsigned y = 32 + i * font_small.height;
		unsigned property = dbg_props[i];
		unsigned len = dbg_props_len[i];
		unsigned fnt = FONT_SMALL;
		if (i == changed) fnt = FONT(FONT_SMALL, 5, COLOR_BG);
		bmp_printf(fnt, x, y,
			"%08x %04x: %8lx %8lx %8lx %8lx %8lx %8lx",
			property,
			len,
			len > 0x00 ? dbg_props_a[i] : 0,
			len > 0x04 ? dbg_props_b[i] : 0,
			len > 0x08 ? dbg_props_c[i] : 0,
			len > 0x0c ? dbg_props_d[i] : 0,
			len > 0x10 ? dbg_props_e[i] : 0,
			len > 0x14 ? dbg_props_f[i] : 0
		);
	}
}

static void *
debug_property_handler(
	unsigned		property,
	void *			UNUSED( priv ),
	void *			buf,
	unsigned		len
)
{
	const uint32_t * const addr = buf;

	DebugMsg( DM_MAGIC, 3, "Prop %08x: %d: %08x %08x %08x %08x",
		property,
		len,
		len > 0x00 ? addr[0] : 0,
		len > 0x04 ? addr[1] : 0,
		len > 0x08 ? addr[2] : 0,
		len > 0x0c ? addr[3] : 0
	);
		
	if( !draw_prop )
		goto ack;

	// maybe the property is already in the array
	int i;
	for (i = 0; i < dbg_propn; i++)
	{
		if (dbg_props[i] == property)
		{
			dbg_props_len[i] = len;
			dbg_props_a[i] = addr[0];
			dbg_props_b[i] = addr[1];
			dbg_props_c[i] = addr[2];
			dbg_props_d[i] = addr[3];
			dbg_props_e[i] = addr[4];
			dbg_props_f[i] = addr[5];
			dbg_draw_props(i);
			goto ack; // return with cleanup
		}
	}
	// new property
	if (dbg_propn >= 30) dbg_propn = 29; // too much is bad :)
	dbg_props[dbg_propn] = property;
	dbg_props_len[dbg_propn] = len;
	dbg_props_a[dbg_propn] = addr[0];
	dbg_props_b[dbg_propn] = addr[1];
	dbg_props_c[dbg_propn] = addr[2];
	dbg_props_d[dbg_propn] = addr[3];
	dbg_props_e[dbg_propn] = addr[4];
	dbg_props_f[dbg_propn] = addr[5];
	dbg_propn++;
	dbg_draw_props(dbg_propn);


ack:
	return prop_cleanup( debug_token, property );
}



#define num_properties 8192
unsigned property_list[ num_properties ];


void
debug_init( void )
{
	draw_prop = 0;
	return;
#if 1
	unsigned i, j, k;
	unsigned actual_num_properties = 0;
	//for( i=0 ; i<=0x8 ; i+=8 )
	i = 8;
	{
		for( j=0 ; j<=0x8 ; j++ )
		{
			for( k=0 ; k<0x40 ; k++ )
			{
				unsigned prop = 0
					| (i << 28) 
					| (j << 16)
					| (k <<  0);

				if( prop != 0x80030014
				&&  prop != 0x80030015
				&&  prop != 0x80050000
				&&  prop != 0x80050004
				&&  prop != 0x80050005
				&&  prop != 0x80050010
				&&  prop != 0x8005000f
				) {
					property_list[ actual_num_properties++ ] = prop;
				}

/*
				if( i !=0 && j != 0 )
				property_list[ actual_num_properties++ ] = 0
					| (2 << 24) 
					| (j << 16)
					| (k <<  0);
*/

				if( actual_num_properties > num_properties )
					goto thats_all;
			}
		}
	}

thats_all:
#else
	int actual_num_properties = 0;
	int i;
	for( i=0 ;i<0xFF ; i++ )
		property_list[actual_num_properties++] = 
			0x02010000 | i;
#endif

	prop_register_slave(
		property_list,
		actual_num_properties,
		debug_property_handler,
		(void*) 0xdeadbeef,
		debug_token_handler
	);

	menu_add( "Debug", debug_menus, COUNT(debug_menus) );
}

CONFIG_INT( "debug.timed-dump",		timed_dump, 0 );

CONFIG_INT( "debug.dump_prop", dump_prop, 0 );
CONFIG_INT( "debug.dumpaddr", dump_addr, 0 );
CONFIG_INT( "debug.dumplen", dump_len, 0 );

CONFIG_INT( "magic.disable_bootdiskf",	disable_bootdiskf, 0 );


static FILE * g_aj_logfile = INVALID_PTR;
unsigned int aj_create_log_file( char * name)
{
   g_aj_logfile = FIO_CreateFile( name );
   if ( g_aj_logfile == INVALID_PTR )
   {
      bmp_printf( FONT_SMALL, 120, 40, "FCreate: Err %s", name );
      return( 0 );  // FAILURE
   }
   return( 1 );  // SUCCESS
}

void aj_close_log_file( void )
{
   if (g_aj_logfile == INVALID_PTR)
      return;
   FIO_CloseFile( g_aj_logfile );
   g_aj_logfile = INVALID_PTR;
}

void dump_seg(uint32_t start, uint32_t size, char* filename)
{
    DEBUG();
    aj_create_log_file(filename);
    FIO_WriteFile( g_aj_logfile, (const void *) start, size );
    aj_close_log_file();
    DEBUG();
}

void dump_big_seg(int k, char* filename)
{
    DEBUG();
    aj_create_log_file(filename);
    
    int i;
    for (i = 0; i < 16; i++)
    {
		DEBUG();
		uint32_t start = (k << 28 | i << 24);
		bmp_printf(FONT_LARGE, 50, 50, "DUMP %x %8x ", i, start);
		FIO_WriteFile( g_aj_logfile, (const void *) start, 0x1000000 );
    }
   
    aj_close_log_file();
    DEBUG();
}

static void dump_vram()
{
	dump_big_seg(1, "B:/1.bin");
	dump_big_seg(4, "B:/4.bin");
}

static void
dump_task( void )
{
	//lua_State * L = lua_open();
  int i;

	// It was too early to turn these down in debug_init().
	// Only record important events for the display and face detect
	dm_set_store_level( 0, 7 ); // default value for ALL
	dm_set_store_level( DM_DISP, 7 );
	dm_set_store_level( DM_WINSYS, 7 );
	dm_set_store_level( DM_LVCFG, 7 );
	dm_set_store_level( DM_MP, 7 );
	dm_set_store_level( DM_MP_MOV, 7 );
	dm_set_store_level( DM_RSC, 0 );
	dm_set_store_level( 50, 3 );
	dm_set_store_level( DM_PTP, 15 );
	
	// increase jpcore debugging (breaks liveview?)
	//dm_set_store_level( 0x15, 2 );
	//dm_set_store_level( 0x2f, 0x16 );

	msleep(5000);
	//bmp_draw_palette();
	//dispcheck();

	unsigned x=10;
	unsigned y=32;


	if (disable_bootdiskf!=0) {
	  bmp_printf( FONT_SMALL, x, y, "**disable_bootdiskf**%s","" );
	  bootdisk_disable();
	}

	//	dump_vram();

	unsigned int *bmp_ram_addr = bmp_vram_info;
	for (i=0; i<2; i++)
	  DebugMsg( DM_MAGIC, 3, "bmp_vram[]=0x%08x, 0x%08x, 0x%08x", bmp_ram_addr[3*i+0],  bmp_ram_addr[3*i+1], bmp_ram_addr[3*i+2] );
	unsigned int *vram_info_addr = vram_info;
	for (i=0; i<3; i++)
	  DebugMsg( DM_MAGIC, 3, "vram_info[]=0x%08x, w=%4d, h=%4d, p=%4d, n=%4d", 
		    vram_info_addr[5*i+0],  vram_info_addr[5*i+1], vram_info_addr[5*i+2], vram_info_addr[5*i+3], vram_info_addr[5*i+4] );
	//unsigned int *stateobj_disp = 0x23DC+0xb0; // see FF062CEC SetBitmapVramAddress 60D?
	//unsigned int *stateobj_disp = 0x23D8+0x90; // see FF05DAF8 SetBitmapVramAddress, 550d 109
		unsigned int *stateobj_disp = 0x238C+0xA8; // see FF062AFC SetBitmapVramAddress, 1100D 104
	DebugMsg( DM_MAGIC, 3, "stateobj_disp+0xa8[]=0x%08x,0x%08x,0x%08x,", stateobj_disp[0], stateobj_disp[1], stateobj_disp[2]);
	
	FILE *f;
	/*	f = FIO_CreateFile( "B:/ff010000.bin" );
	if (f) {
	  FIO_WriteFile( f, 0xff010000, 0x900000);
	  FIO_CloseFile( f );
	  bmp_printf( FONT_LARGE, 50, 300, "ff010000.bin saved" );
	  }*/
	f = FIO_CreateFile( "B:/f8000000.bin" );
	if (f) {
	  FIO_WriteFile( f, 0xf8000000, 0x800000);
	  FIO_CloseFile( f );
	  bmp_printf( FONT_LARGE, 50, 300, "f8000000.bin saved" );
	  }
	f = FIO_CreateFile( "B:/ffff0000.bin" );
	if (f) {
	  FIO_WriteFile( f, 0xffff0000, 0xffff);
	  FIO_CloseFile( f );
	  bmp_printf( FONT_LARGE, 50, 300, "ffff0000.bin saved" );
	}

	DebugMsg( DM_MAGIC, 3, "%s: calling dumpf", __func__ );
	dumpf();

}


TASK_CREATE( "dump_task", dump_task, 0, 0x1f, 0x1000 );
TASK_CREATE( "debug_loop_task", debug_loop_task, 0, 0x1f, 0x1000 );

CONFIG_INT( "debug.timed-start",	timed_start, 0 );

