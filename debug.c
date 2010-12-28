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
#include "gui.h"
#include "lens.h"
//#include "lua.h"

extern void bootdisk_disable();

#if 0
void
display_full_hd(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	unsigned * gui_struct = (void*) 0x3548;
	unsigned * ps_struct = (void*) 0x11bb8;
	// disp=0x08 == 3 == 1080p, 5 == vga
	// vid=0x0c == 0
	// hdmi=0x10 == 3, 5 == vga
	// c4=0xc4 == 0, 1 == vga

	unsigned (*GetDisplayType)(void) = (void*) 0xFF863590;

	uint32_t * lv_struct = (void*) 0x37fc;

	bmp_printf(
		FONT_MED,
		x, y,
		//23456789012
#if 0
		"disp=%x vid=%x hdmi=%x c4=%x ps=%x type=%x/%x",
		gui_struct[0x08 / 4],
		gui_struct[0x0C / 4],
		gui_struct[0x10 / 4],
		gui_struct[0xC4 / 4],
		ps_struct[0x230 / 4],
		hdmi_config.disp_type,
		hdmi_config.off_0x0c
#else
		"mvr %d/%x",
		mvr_struct->fps,
		mvr_struct->bit_rate
#endif
	);

	bmp_hexdump( FONT_SMALL, 300, 400,
		(void*) 0x7b40,
		64
	);
}


void enable_full_hd( void * priv )
{
#if 0
	if( mvr_struct->fps == 0 )
		mvr_struct->fps = 30;
	uint8_t bitrate = 60;
	//call( "mvrSetBitRate", &bitrate );
	void (*mvrSetBitRate)( uint8_t * bitrate ) = (void*) 0xff84f990;
	mvrSetBitRate( &bitrate );
#endif
	void (*mvrSetQScale)( int8_t * ) = (void*) 0xff9715e0;
	int8_t scale = -30;
	mvrSetQScale( &scale );
	return;

	DebugMsg( DM_MAGIC, 3, "Attempting to set HDMI to full HD" );

#if 1
	thunk ChangeHDMIOutputSizeToFULLHD = (thunk) 0xFFA96260;
	//void (*SetDisplayType)(int) = (void*) 0xFF8620DC;
	void (*SetDisplayType)(int) = (void*) 0xFFB4835C;

	//SetDisplayType( 0 );
	unsigned * gui_struct = (void*) 0x3548;
	unsigned * ps_struct = (void*) 0x11bb8;
	ChangeHDMIOutputSizeToFULLHD();
	//gui_struct[ 0xC4/4 ] = 0; // not resized?
	//ps_struct[ 0x230/4 ] = 6; // who knows

	DebugMsg( DM_MAGIC, 3, "Full HD done?" );
#else
	// This will select the image on the full screen.
	// but not change the output resolution
	void (*lv_output_device)( const char ** arg ) = (void*) 0xFF833218;
	const char * full_hd = "1080full";
	lv_output_device( &full_hd );
#endif
}
#endif


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

#if 0
static void
mvr_time_const_select( void * priv )
{
/*
	void (*mvr_set_time_const)(int *) = (void*) 0xff9716cc;
	void (*mvr_setd_fullhd)(int *) = (void*) 0xff9716cc;
	//int args[] = { 640, 480 };
	//DebugMsg( DM_MAGIC, 3, "calling mvr_setd_fullhd %d %d", args[0], args[1] );
	//mvr_setd_fullhd( args );

	uint32_t buf[] = { 8 };
	//prop_request_change( 0x207000c, buf, sizeof(buf) );
	void (*lv_magnify)( int * ) = (void*) 0xff83359c;
	lv_magnify( buf );
	void (*mvrSetBitRate)( int * ) = (void*) 0xff84f990;
	//int rate = 24;
	//mvrSetBitRate( &rate );
	mvr_struct->is_vga	= 0;
	mvr_struct->width	= 1920;
	mvr_struct->height	= 1080;
	mvr_struct->fps		= 24;
*/

	uint8_t * mvr_hdr = *(void**)( 0x1ed4 + 4 );
	*(unsigned *)( mvr_hdr + 0x60 ) = 2400;
	*(unsigned *)( mvr_hdr + 0x64 ) = 100;
	*(unsigned *)( mvr_hdr + 0x68 ) = 24;
}
#endif


CONFIG_INT( "debug.draw-prop",		draw_prop, 0 );

static void
draw_prop_select( void * priv )
{
	draw_prop = !draw_prop;
}

static int dbg_propn = 0;
static void 
draw_prop_reset( void * priv )
{
	dbg_propn = 0;
}


CONFIG_INT( "debug.mem-spy",		mem_spy, 0 );
CONFIG_INT( "debug.mem-spy.start.lo",	mem_spy_start_lo,	0 ); // start from here
CONFIG_INT( "debug.mem-spy.start.hi",	mem_spy_start_hi,	0 ); // start from here
CONFIG_INT( "debug.mem-spy.len",	mem_spy_len,	0x400 );         // look at ### int32's
CONFIG_INT( "debug.mem-spy.bool",	mem_spy_bool,	0 );         // only display booleans (0,1,-1)
CONFIG_INT( "debug.mem-spy.small",	mem_spy_small,	0 );         // only display small numbers (less than 10)

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
	bmp_printf(FONT_MED, 10,10, "memspy init @ %x ... (+%x) ... %x", mem_spy_start, mem_spy_len, mem_spy_start + mem_spy_len * 4);
	msleep(2000);
	//mem_spy_len is number of int32's
	if (!dbg_memmirror) dbg_memmirror = AllocateMemory(mem_spy_len*4 + 100); // local copy of mem area analyzed
	if (!dbg_memmirror) return;
	if (!dbg_memchanges) dbg_memchanges = AllocateMemory(mem_spy_len*4 + 100); // local copy of mem area analyzed
	if (!dbg_memchanges) return;
	int i;
	bmp_printf(FONT_MED, 10,10, "memspy alloc");
	uint32_t crc = 0;
	for (i = 0; i < mem_spy_len; i++)
	{
		uint32_t addr = mem_spy_start + i*4;
		dbg_memmirror[i] = *(uint32_t*)(addr);
		dbg_memchanges[i] = 0;
		crc += dbg_memmirror[i];
		bmp_printf(FONT_MED, 10,10, "memspy: %8x => %8x ", addr, dbg_memmirror[i]);
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
		//~ else continue;

		if (mem_spy_bool && newval != 0 && newval != 1 && newval != 0xFFFFFFFF) continue;
		if (mem_spy_small && newval > 10) continue;

		// show addresses which change, but not those which change like mad
		if (dbg_memchanges[i] > 5 && dbg_memchanges[i] < 50)
		{
			int x = 10 + 8 * 22 * (k % 4);
			int y = 10 + 12 * (k / 4);
			bmp_printf(fnt, x, y, "%8x:%2d:%8x", addr, dbg_memchanges[i], newval);
			k = (k + 1) % 120;
		}
	}

	for (i = 0; i < 10; i++)
	{
		int x = 10 + 8 * 22 * (k % 4);
		int y = 10 + 12 * (k / 4);
		bmp_printf(FONT_SMALL, x, y, "                    ");
		k = (k + 1) % 120;
	}
}

PROP_INT(PROP_SHUTTER_COUNT, shutter_count);

void display_info()
{
	bmp_enabled = 1;
	bmp_printf(FONT_MED, 20, 400, "Shutter Count: %d", shutter_count);
	bmp_printf(FONT_MED, 20, 420, "CMOS Temperat: %d", efic_temp);
	bmp_printf(FONT_MED, 20, 440, "Lens: %s          ", lens_info.name);
}
void display_clock()
{
	bmp_enabled = 1;
	int bg = bmp_getpixel(15, 430);
	uint32_t fnt = FONT(FONT_LARGE, 80, bg);

	struct tm now;
	LoadCalendarFromRTC( &now );
	bmp_printf(fnt, 200, 410, "%02d:%02d", now.tm_hour, now.tm_min);
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
		if (gui_state == GUISTATE_MENUDISP)
		{
			display_info();
		}
		
		if (!lv_drawn() && gui_state == GUISTATE_IDLE && !gui_menu_shown() && bmp_getpixel(2,10) != 2)
		{
			display_clock();
			display_shooting_info();
		}
		
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
		else msleep(100);
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
	//~ {
		//~ .display	= efic_temp_display,
	//~ },
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
	//~ {
		//~ .priv		= "Toggle draw_prop",
		//~ .select		= draw_prop_select,
		//~ .select_reverse = draw_prop_reset,
		//~ .display	= menu_print,
	//~ },
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

//~ static int dbg_propn = 0;
#define MAXPROP 10
static unsigned dbg_props[MAXPROP] = {0};
static unsigned dbg_props_len[MAXPROP] = {0};
static unsigned dbg_props_a[MAXPROP] = {0};
static unsigned dbg_props_b[MAXPROP] = {0};
static unsigned dbg_props_c[MAXPROP] = {0};
static unsigned dbg_props_d[MAXPROP] = {0};
static unsigned dbg_props_e[MAXPROP] = {0};
static unsigned dbg_props_f[MAXPROP] = {0};
static void dbg_draw_props(int changed)
{
	dbg_last_changed_propindex = changed;
	int i; 
	for (i = 0; i < dbg_propn; i++)
	{
		unsigned x = 80;
		unsigned y = 15 + i * font_small.height;
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
	if (dbg_propn >= MAXPROP) dbg_propn = MAXPROP-1; // too much is bad :)
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



//~ #define num_properties 8192
//~ unsigned property_list[ num_properties ];


void
debug_init( void )
{
	draw_prop = 0;

/*
#if 1
	unsigned i, j, k;
	unsigned actual_num_properties = 0;
	
	unsigned is[] = {0x80, 0xe, 0x5, 0x4, 0x2, 0x1, 0x0};
	for( i=0 ; i<=COUNT(is) ; i++ )
	{
		for( j=0 ; j<=0x8 ; j++ )
		{
			for( k=0 ; k<0x40 ; k++ )
			{
				unsigned prop = 0
					| (is[i] << 24) 
					| (j << 16)
					| (k <<  0);

				property_list[ actual_num_properties++ ] = prop;

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
	); */

	menu_add( "Debug", debug_menus, COUNT(debug_menus) );
}

CONFIG_INT( "debug.timed-dump",		timed_dump, 0 );

//~ CONFIG_INT( "debug.dump_prop", dump_prop, 0 );
//~ CONFIG_INT( "debug.dumpaddr", dump_addr, 0 );
//~ CONFIG_INT( "debug.dumplen", dump_len, 0 );

CONFIG_INT( "magic.disable_bootdiskf",	disable_bootdiskf, 0 );

void show_logo()
{
	gui_stop_menu();
	msleep(1000);
	struct bmp_file_t * bmp = bmp_load("B:/logo.bmp");
	int i;
	for (i = 0; i < 100; i++)
	{
		bmp_draw(bmp,0,0);
		bmp_enabled = 0;
		msleep(10);
		bmp_enabled = 1;
	}
}


static void
dump_task( void )
{

	//lua_State * L = lua_open();
	//~ show_logo();
	//~ clrscr();
	// Parse our config file
	const char * config_filename = "B:/magic.cfg";
	global_config = config_parse_file( config_filename );
	bmp_printf( FONT_MED, 0, 70,
		"Config file %s: %s",
		config_filename,
		global_config ? "YES" : "NO"
	);

	// It was too early to turn these down in debug_init().
	// Only record important events for the display and face detect
	//~ dm_set_store_level( DM_DISP, 4 );
	//~ dm_set_store_level( DM_LVFD, 4 );
	//~ dm_set_store_level( DM_LVCFG, 4 );
	//~ dm_set_store_level( DM_LVCDEV, 4 );
	//~ dm_set_store_level( DM_LV, 4 );
	//~ dm_set_store_level( DM_RSC, 4 );
	dm_set_store_level( 0, 4 ); // catch all?
	
	// increase jpcore debugging (breaks liveview?)
	//dm_set_store_level( 0x15, 2 );
	//dm_set_store_level( 0x2f, 0x16 );

	//msleep(1000);
	//bmp_draw_palette();
	//dispcheck();

	if (lv_drawn())
	{
		msleep(2000);
		clrscr();
	}


	unsigned x=10;
	unsigned y=32;

	if (disable_bootdiskf!=0) {
	  bmp_printf( FONT_SMALL, x, y, "**disable_bootdiskf**%s","" );
	  bootdisk_disable();
	}

	if( timed_dump == 0 )
		goto end;

	int sec = timed_dump;

	DebugMsg( DM_MAGIC, 3, "%s: Will do debug dump in %d sec",
		__func__,
		sec
	);

	while( sec-- )
	{
		//~ bmp_printf( FONT_SMALL, 600, 400, "dump %2d", sec );
		msleep( 1000 );
	}

	DebugMsg( DM_MAGIC, 3, "%s: calling dumpf", __func__ );
	dumpf();

end:
	debug_loop_task();
}


TASK_CREATE( "dump_task", dump_task, 0, 0x1f, 0x1000 );
//~ TASK_CREATE( "debug_loop_task", debug_loop_task, 0, 0x1f, 0x1000 );

CONFIG_INT( "debug.timed-start",	timed_start, 0 );

static void
movie_start( void )
{
	int sec = timed_start;
	if( sec == 0 )
		return;

	const int x = 320;
	const int y = 150;

	while( --sec > 0 )
	{
		msleep( 1000 );
		bmp_printf(
			FONT(
				FONT_LARGE,
				sec > 4 ? COLOR_WHITE : COLOR_RED,
				0
			),
			x, y,
			"T-%d",
			sec
		);
	}

	bmp_printf( FONT(FONT_LARGE,COLOR_WHITE,0), x, y, "GO!" );

	call( "MovieStart" );

	msleep( 1000 );

	bmp_printf( FONT(FONT_LARGE,COLOR_WHITE,0), x, y, "   " );
}

//~ TASK_CREATE( "movie_start", movie_start, 0, 0x1f, 0x1000 );
