#include "dryos.h"
#include "bmp.h"
#include "gui.h"
#include "menu.h"
#include "state-object.h"
#include "cache_hacks.h"

/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];
extern void create_task_cmd_shell(const char * name);
void ml_big_init_task(); // forward declaration
int bmp_vram_idle_ptr;

void start_debug_mode() {
	int file;
	char filename[20] = "A:/DEBUG.LOG"; // default name
	//int filesize;
	time_t t;
	struct tm tm;

	file = (int) FIO_OpenFile(filename, O_CREAT | O_WRONLY , 644);
	if(file > 0) {
		// redirect stdout and stderr to our file
		ioGlobalStdSet(1, file);
		ioGlobalStdSet(2, file);
	}

	char separator[] = "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
	//printf(separator);
	//printf("\n");
}



static void busy_wait(int n)
{
	int i,j;
	static volatile int k = 0;
	for (i = 0; i < n; i++)
		for (j = 0; j < 100000; j++)
			k++;
}
#if 0
asm(
	"MOV R1, #0x46\n"
	"MOV R0, #0x220000\n"
	"ADD R0, R0, #0xC0000000\n"

	"STR R1, [R0,#0xA0]\n"
	"STR R1, [R0,#0xF0]\n"
);
#endif

void blink(int times)
{
	if (!times) times=2;
	while(times--) {
		LEDBLUE = LEDON;
		LEDRED = LEDON;
		busy_wait(50);
		LEDBLUE = LEDOFF;
		LEDRED = LEDOFF;
		busy_wait(50);
	}
}

void test_cache_hacks() {
	int (*return_zero)() = (void*) 0xFFA2FBC4;

	LEDRED = LEDOFF;
	LEDBLUE = LEDOFF;

	if (return_zero()) {
		while(1) {
			LEDBLUE = LEDON;
			busy_wait(100);
			LEDBLUE = LEDOFF;
			busy_wait(100);
		}
	} else {
		while(1) {
			LEDRED = LEDON;
			busy_wait(100);
			LEDRED = LEDOFF;
			busy_wait(100);
		}
	}

	while(1) {
		LEDBLUE = LEDON;
		LEDRED = LEDOFF;
		busy_wait(100);
		LEDBLUE = LEDOFF;
		LEDRED = LEDON;
		busy_wait(100);
	}
}

static inline void
zero_bss( void )
{/*{{{*/
	uint32_t *bss = _bss_start;
	while( bss < _bss_end )
		*(bss++) = 0;
}/*}}}*/

void cache_hacks_helper_set_our_control_register(void)
{
#if 0
	uint32_t r;
	// 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
	// 1  0  1  1  1  1  1  0  0  0  0  0  1  0  0  0  1  0  1  0  0  0  0  0  0  0  0  0  0  0  0  0
	asm volatile (
			"MRC     p15, 0, %0,c1,c0, 0	\n"
			: "=r"(r) : :
	);

	LEDBLUE=LEDON;
	LEDRED=LEDON;
	busy_wait(200);
	LEDBLUE=LEDOFF;
	LEDRED=LEDOFF;
	busy_wait(100);
	for (int i=0; i< 32; i++) {
		if ( (r>>i) & 1 ) {
			LEDBLUE=LEDON;
		} else {
			LEDRED=LEDON;
		}

		busy_wait(50);
		LEDBLUE=LEDOFF;
		LEDRED=LEDOFF;
		busy_wait(100);
	}

	while(1);
#endif

	asm volatile (
			//"MOV	R1, #0x78			\n" // OFW was setting bit 3:6 only (reserved)
			"MOV	R1, #0				\n" // we set few bits more
			//"MRC	p15, 0, R1,c1,c0, 0	\n"

			"ORR	R1, R1, #0x01		\n" // MPU - we need it, otherwise it will disable the caches
			"ORR	R1, R1, #0x04		\n" // D Cache - w/o this the data is slow
			"ORR	R1, R1, #0x08		\n" // reserved - OFW requested
			"ORR	R1, R1, #0x10		\n" // reserved - OFW requested
			"ORR	R1, R1, #0x20		\n" // reserved - OFW requested
			"ORR	R1, R1, #0x40		\n" // reserved - OFW requested
			"ORR	R1, R1, #0x1000		\n" // I Cache - we wont have cache hacks w/o this
			"ORR	R1, R1, #0x10000	\n" // D TCM - these would be set by the OFW later
			"ORR	R1, R1, #0x40000	\n" // I TCM - these would be set by the OFW later
			"MCR	p15, 0, R1,c1,c0, 0	\n"
	);
}

void cache_hacks_helper_0xFFB40344(void)
{
	asm volatile (
		"MRC	p15, 0, R2,c1,c0, 0	\n"
		"BIC	R2, R2, #4				\n"
		"ORR	R2, R2, #0x1000		\n" // we enable I cache
		"ORR	R2, R2, #0x01		\n" // MPU - we need it, otherwise it will disable the caches
		"ORR	R1, R1, #0x04		\n" // D Cache - w/o this the data is slow
		"MCR	p15, 0, R2,c1,c0, 0	\n"
	);
	blink(2);
}

unsigned int cache_hacks_helper_orr_0x11000(unsigned int arg)
{
	// OFW: ORR R0, R0, #0x10000   @ to enable TCM
	// we add bit 12 to enable I Cache too
	return arg | 0x11000;
}

static inline void disable_cache_clearing()
{/*{{{*/
	/* this is a evil hack to disable cache clearing all on way to ML tasks */
	// 0xAF: these are all locations of cache clear I've found

	// add flags to enable the I/D Caches (where they are disabled)
#if 1
	cache_fake(0xFFB40344, NOP_INSTR, TYPE_ICACHE); // disable D cache
#else
	cache_fake(0xFFB4033C, NOP_INSTR, TYPE_ICACHE);
	cache_fake(0xFFB40340, NOP_INSTR, TYPE_ICACHE);
	cache_fake(0xFFB40344, BL_INSTR(0xFFB40344, &cache_hacks_helper_0xFFB40344), TYPE_ICACHE);
#endif

	cache_fake(0xFF810078, BL_INSTR(0xFF810078, &cache_hacks_helper_orr_0x11000), TYPE_ICACHE);
	//cache_fake(0xFF810078, 0xE3800A11, TYPE_ICACHE);


	// I cache

	// disable_i_cache() calls
	cache_fake(0xFFB34EB8, NOP_INSTR, TYPE_ICACHE); // disable_I_cache
	cache_fake(0xFFB34A10, NOP_INSTR, TYPE_ICACHE); // disable_D_cache
	//cache_fake(0xFFB34A14, NOP_INSTR, TYPE_ICACHE); // disable_I_cache

	// disable_I_cache():
#if 0
	// option 1: MCR     p15, 0, R2,c1,c0, 0   @ R2 = R2 & ~0x1000 - disable I cache
	cache_fake(0xFFB40418, NOP_INSTR, TYPE_ICACHE);
	// option 2: BIC     R2, R2, #0x1000       @ disable I cache
	//cache_fake(0xFFB40414, NOP_INSTR, TYPE_ICACHE);
	// MCR p15, 0, R0(==0),c7,c5,  0 @ Invalidate entire I cache (flush branch target cache)
	cache_fake(0xFFB40420, NOP_INSTR, TYPE_ICACHE);
#else
	// disable_i_cache(), we directly return from it
	cache_fake(0xFFB40404, RET_INSTR, TYPE_ICACHE);
#endif

	// MCR p15, 0, R1(==0),c7,c7,  0 @ Invalidate I&D caches or unified cache (flush branch target cache)
	cache_fake(0xFF810068, NOP_INSTR, TYPE_ICACHE);

	// MCR p15, 0, R1(==0),c7,c7,  0 @ Invalidate I&D caches or unified cache (flush branch target cache)
	cache_fake(0xFF810414, NOP_INSTR, TYPE_ICACHE);

	// MCR p15, 0, R0(==?),c7,c5,  0 @ Invalidate entire I cache (flush branch target cache)
	cache_fake(0xFFB403A0, NOP_INSTR, TYPE_ICACHE);

	// MCR p15, 0, R0(==?),c7,c5,  1 @ Invalidate I cache line
	cache_fake(0xFFB403AC, NOP_INSTR, TYPE_ICACHE);


	// D cache (we wont touch the D cache for now, since we patch only instructions) /*{{{*/

	// MCR p15, 0, R3(==0),c7,c14, 2 @ Clean and invalidate D cache line
	//cache_fake(0xFFB2DE4C, 0xE1A00000, TYPE_ICACHE);

	// MCR p15, 0, R3(==0),c7,c14, 2 @ Clean and invalidate D cache line
	//cache_fake(0xFFB2DFE8, 0xE1A00000, TYPE_ICACHE);

	// MCR p15, 0, R3(==0),c7,c14, 2 @ Clean and invalidate D cache line
	//cache_fake(0xFFB40320, 0xE1A00000, TYPE_ICACHE);

	// MCR p15, 0, R0(==?),c7,c10, 1 @ Clean D cache line
	// cache_fake(0xFFB40350, 0xE1A00000, TYPE_ICACHE);

	// MCR p15, 0, R0(==?),c7,c10, 2 @ Clean D cache line
	// cache_fake(0xFFB40368, 0xE1A00000, TYPE_ICACHE);

	// MCR p15, 0, R0(==0),c7,c6,  0 @ Invalidate entire D cache
	//cache_fake(0xFFB4038C, 0xE1A00000, TYPE_ICACHE);

	// MCR p15, 0, R0(==?),c7,c6,  1 @ Invalidate D cache line
	//cache_fake(0xFFB40394, 0xE1A00000, TYPE_ICACHE);

	// MCR p15, 0, R0(==?),c7,c14, 2 @ Clean and invalidate D cache line
	//cache_fake(0xFFB403D8, 0xE1A00000, TYPE_ICACHE);

	// MCR p15, 0, R0(==0),c7,c14, 1 @ Clean and invalidate D cache line
	//cache_fake(0xFFB403FC, 0xE1A00000, TYPE_ICACHE);
	/*}}}*/

}/*}}}*/

void ml_init_task()
{/*{{{*/
	LEDBLUE = LEDON;
	_card_led_on();
	msleep(200);
	hijack_gui_main_task();
	bmp_vram_idle_ptr = (int)malloc(360*240);
	ml_big_init_task();
	LEDBLUE = LEDOFF;
	_card_led_off();
	//~ hijack_event_dispatches();
}/*}}}*/

void ml_hijack_create_task_cmd_shell(const char * name)
{/*{{{*/
	blink(20);

	// call original create_task_cmd_shell to start taskCmdShell
	create_task_cmd_shell(name);

	// create ml_init_task to start ML
	task_create("ml_init_task", 0x1f, 0x2000, ml_init_task, 0);
}/*}}}*/

void dump_with_buffer(int addr, int len, char* filename);
void old_init()
{/*{{{*/
	//start_debug_mode();
	dump_with_buffer(0x0, 0x800000, "A:/0x0.RAM");
	task_create("ml_init_task", 0x1f, 0x2000, ml_init_task, 0);
}/*}}}*/

static inline void configure_cache_replaces()
{/*{{{*/
	/* reserve 512 KB or RAM for ML (original: MOV R1, 0xa00000; modified: MOV R1, 0x980000) */
	cache_fake(0xFF8110BC, 0xE3A01726, TYPE_ICACHE);

	/* replace create_task_cmd_shell with our modified version to start ml_init_task */
	cache_fake(0xFF811218, BL_INSTR(0xFF811218, &ml_hijack_create_task_cmd_shell), TYPE_ICACHE);

#ifdef CONFIG_PTP
	/* replace one ptp_register funtion call with ml_hijack_ptp_register_handlers */
	/* sub_FFBA12E0 - 0xFFBA12E4 : BL sub_FFB9D4E4 */
	//cache_fake(0xFFBA12E4, BL_INSTR(0xFFBA12E4, &ml_hijack_ptp_register_handlers), TYPE_ICACHE); // OK
#endif
}/*}}}*/

void copy_and_restart()
{/*{{{*/
	//blink(1);

	zero_bss();
	init_code_run(); // old boot method

	/* lock down caches */
	cache_lock();

	disable_cache_clearing();

	configure_cache_replaces();


	cache_fake(0xFFA2FBC4, 0xE3A00001, TYPE_ICACHE); // ret_zero
//#define TEST_CACHE_HACKS 0xFF8104A8 // replace one NOP with the test
#define TEST_CACHE_HACKS 0xFFB34A18 // this is after the clear cache calls
//#define TEST_CACHE_HACKS 0xFFB40404 // first instruction in clear_I_cache
	cache_fake(0xFFB34A14, NOP_INSTR, TYPE_ICACHE); // NOP the clear_I_cache() call
	cache_fake(TEST_CACHE_HACKS, BL_INSTR(TEST_CACHE_HACKS, &test_cache_hacks), TYPE_ICACHE);

	//init_code_run(); // old boot method

	cache_fake(0xFF81005C, BL_INSTR(0xFF81005C, &cache_hacks_helper_set_our_control_register), TYPE_ICACHE);
	cache_fake(0xFF810408, BL_INSTR(0xFF810408, &cache_hacks_helper_set_our_control_register), TYPE_ICACHE);
	//cache_fake(0xFF81005C, NOP_INSTR, TYPE_ICACHE);
	//cache_fake(0xFF810408, NOP_INSTR, TYPE_ICACHE);

	cache_fake(0xFFB36618, NOP_INSTR, TYPE_ICACHE); // TODO: dangerous part of change_control_register(),   investigate further
	cache_fake(0xFFB36648, NOP_INSTR, TYPE_ICACHE); // TODO: dangerous part of change_control_register_2(), investigate further
	cache_fake(0xFFB40638, NOP_INSTR, TYPE_ICACHE); // TODO: dangerous part of change_control_register_3(), investigate further

	//cache_fake(0xFF81005C, BL_INSTR(0xFF81005C, &test_cache_hacks), TYPE_ICACHE); // test
	//cache_fake(0xFF81005C, LOOP_INSTR, TYPE_ICACHE); // test
	//cache_fake(0xFF81005C, NOP_INSTR, TYPE_ICACHE); // test


	/* now start the original firmware (OFW) */
	//extern void firmware_entry_2_params(int, int);
	//firmware_entry_2_params(2, 0x1079);


	//cache_fake(0xFF810054, 0xE3A01078, TYPE_ICACHE); // MOV R1, #0x78
	//cache_fake(0xFF810058, 0xE2811A01, TYPE_ICACHE); // ADD R1, R1, 0x1004 - enable caches
	firmware_entry();


	// unreachable
	while(1) {
		LEDBLUE = LEDON;
		LEDRED = LEDOFF;
		busy_wait(100);
		LEDBLUE = LEDOFF;
		LEDRED = LEDON;
		busy_wait(100);
	}
}/*}}}*/

// //////////////////////////////////////////////////////////////////////// //

void dumpmem(int addr, int len)
{/*{{{*/
	FIO_RemoveFile("A:/0x0.BIN");
	FILE* f = FIO_CreateFile("A:/0x0.BIN");
	if ((int32_t)f!=-1) {
		FIO_WriteFile(f, (void*)addr, len);
		LEDBLUE = LEDON;
		FIO_CloseFile(f);
	}
}/*}}}*/

//~ dump memory 64kb at a time using a buffer.
void dump_with_buffer(int addr, int len, char* filename)
{/*{{{*/
	FIO_RemoveFile(filename);
	FILE* f = FIO_CreateFile(filename);
	if ((int32_t)f!=-1) {
		int address = addr;
		while (address<addr+len) {
			static int z = 0;
			if (z) LEDBLUE=LEDON; else LEDBLUE=LEDOFF;
			z = !z;
			static char buf[0x10000];
			memcpy(buf, (void*)address, 0x10000);
			FIO_WriteFile(f, buf, 0x10000);
			address += 0x10000;
		}
		FIO_CloseFile(f);
		LEDBLUE=LEDOFF;
	}
}/*}}}*/


//~ Useful for intercepting state changes and events.
//~ Modules
#define ShootCapture (*(struct Module **)0x4E74)
#define ShootDevelop (*(struct Module **)0x4E78)
#define ShootBlack   (*(struct Module **)0x4EA4)
#define Fcreate      (*(struct Module **)0x4ED0)
#define Fstorage     (*(struct Module **)0x4ED8)
#define Rstorage     (*(struct Module **)0x4EE0)
#define Factory      (*(struct Module **)0xD148)

//~ Managers
#define GenMgr       (*(struct Manager **)0x1B9C)
#define EventMgr     (*(struct Manager **)0x4EE8)
#define FileCache    (*(struct Manager **)0x4F1C)
#define RscMgr       (*(struct Manager **)0x4F28)
#define DPMGR_T      (*(struct Manager **)0x504C)
#define DPOFMGR_T    (*(struct Manager **)0x510C)
#define TOMgr        (*(struct Manager **)0x5910)
#define FileMgr      (*(struct Manager **)0x592C)
#define PropMgr      (*(struct Manager **)0xF508)
#define DbgMgr       (*(struct Manager **)0x101FC)

int (*EventDispatchHandler)(int,int,int,int) = 0;
static int eventdispatch_handler(int arg0, int arg1, int arg2, int arg3)
{/*{{{*/
	int ans = EventDispatchHandler(arg0, arg1, arg2, arg3);
	DryosDebugMsg(0, 3, "[MAGIC] name/arg1/arg2/arg3: %s/0x%x/0x%x/0x%x", MEM(arg0), arg1, arg2, arg3);

	return ans;
}/*}}}*/

static int hijack_manager(struct Manager * manager)
{/*{{{*/
	EventDispatchHandler = (void *)manager->taskclass_ptr->eventdispatch_func_ptr;
	manager->taskclass_ptr->eventdispatch_func_ptr = (void *)eventdispatch_handler;

	return 0; //~ not used.
}/*}}}*/

static int hijack_module(struct Module * module)
{/*{{{*/
	EventDispatchHandler = (void *)module->stageclass_ptr->eventdispatch_func_ptr;
	module->stageclass_ptr->eventdispatch_func_ptr = (void *)eventdispatch_handler;

	return 0; //~ not used.
}/*}}}*/

/**
 To hijack an EventDispatch of a manager or module, remember:
 - Modules have a StageClass
 - Managers have a TaskClass
 
 The structures for these are already defined in state-object.h
 NOTE: you can only hijack one manager or module at a time. Never try to hijack more than one! (could brick camera).
 **/
static void hijack_event_dispatches( void )
{/*{{{*/
	//~ hijack_manager(PropMgr);
	//~ hijack_module(ShootCapture);
}/*}}}*/

void rewrite_version_string()
{/*{{{*/
	// only 3 characters available :(
	char * additional_version = (char *)0x1DA0;
	additional_version[0] = '-';
	additional_version[1] = 'M';
	additional_version[2] = 'L';
	additional_version[3] = '\0';
}/*}}}*/

static void
call_init_funcs( void * priv )
{/*{{{*/
	// Call all of the init functions
	extern struct task_create _init_funcs_start[];
	extern struct task_create _init_funcs_end[];
	struct task_create * init_func = _init_funcs_start;

	for( ; init_func < _init_funcs_end ; init_func++ ) {
		DebugMsg( DM_MAGIC, 3,
			"Calling init_func %s (%x)",
			init_func->name,
			(unsigned) init_func->entry
		);
		thunk entry = (thunk) init_func->entry;
		entry();
	}
}/*}}}*/

int _hold_your_horses = 1; // 0 after config is read
int ml_started = 0; // 1 after ML is fully loaded
int magic_off_request = 0;
int ml_gui_initialized = 0; // 1 after gui_main_task is started

// Only after this task finished, the others are started
// From here we can do file I/O and maybe other complex stuff
void ml_big_init_task()
{/*{{{*/
	call("DisablePowerSave");
	menu_init();
	debug_init();
	call_init_funcs( 0 );

	msleep(200); // leave some time for property handlers to run

	config_parse_file( "ML/SETTINGS/magic.cfg" );
	debug_init_stuff();

	_hold_your_horses = 0; // config read, other overriden tasks may start doing their job

	// Create all of our auto-create tasks
	extern struct task_create _tasks_start[];
	extern struct task_create _tasks_end[];
	struct task_create * task = _tasks_start;

	int ml_tasks = 0;
	for( ; task < _tasks_end ; task++ ) {

		// for debugging: uncomment this to start only some specific tasks
		// tip: use something like grep -nr TASK_CREATE ./ to find all task names
#if 1
		if (
				//~ streq(task->name, "audio_meter_task") ||
				//~ streq(task->name, "audio_level_task") ||
				//~ streq(task->name, "bitrate_task") ||
				//~ streq(task->name, "cartridge_task") ||
				streq(task->name, "cls_task") ||
				//~ streq(task->name, "console_task") ||
				streq(task->name, "debug_task") ||
				//~ streq(task->name, "dmspy_task") ||
				//~ streq(task->name, "focus_task") ||
				//~ streq(task->name, "focus_misc_task") ||
				//~ streq(task->name, "fps_task") ||
				//~ streq(task->name, "iso_adj_task") ||
				//~ streq(task->name, "joypress_task") ||
				//~ streq(task->name, "light_sensor_task") ||
				//~ streq(task->name, "livev_hiprio_task") ||
				//~ streq(task->name, "livev_loprio_task") ||
				streq(task->name, "menu_task") ||
				streq(task->name, "menu_redraw_task") ||
				//~ streq(task->name, "morse_task") ||
				//~ streq(task->name, "movtweak_task") ||
				streq(task->name, "ms100_clock_task") || // missing ?
				streq(task->name, "notifybox_task") ||
				streq(task->name, "clock_task") ||
				streq(task->name, "shoot_task") ||
				streq(task->name, "tweak_task") ||
				0 )
#endif
		{
			task_create(
				task->name,
				task->priority,
				task->stack_size,
				task->entry,
				task->arg
			);
			ml_tasks++;
		}
	}
	//~ bmp_printf( FONT_MED, 0, 85,
		//~ "Magic Lantern is up and running... %d tasks started.",
		//~ ml_tasks
	//~ );
	msleep(500);
	ml_started = 1;
	rewrite_version_string();
}/*}}}*/

void hold_your_horses(int showlogo)
{/*{{{*/
	while (_hold_your_horses) {
		msleep( 100 );
	}
}/*}}}*/

// gcc mempcy has odd alignment issues?
void
my_memcpy(void * dest, const void * src, size_t len)
{/*{{{*/
	while( len-- > 0 )
		*(uint8_t*)dest++ = *(const uint8_t*)src++;
}/*}}}*/

/**
 * Custom ML assert handler
 */
static char assert_msg[1000] = "";
int (*old_assert_handler)(char*,char*,int,int) = 0;
const char* get_assert_msg() { return assert_msg; }

void ml_assert_handler(char* msg, char* file, int line, const char* func)
{/*{{{*/
	snprintf(assert_msg, sizeof(assert_msg),
		"ML ASSERT:\n%s\n"
		"at %s:%d (%s), task %s\n",
		msg, 
		file, line, func, get_task_name_from_id(get_current_task())
	);
	request_crash_log(2);
}/*}}}*/

void dump_edmac()
{/*{{{*/
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 5; j++) {
			bmp_printf(FONT_MED, 50 + j*120, 50+i*20, "%x ", MEM(0xC0F04000 + 0x100*i + 4*j));
		}
		int addr = 0xC0F04000 + 0x100*i + 8;
		if (MEM(addr)) {
			int size = MEM(0xC0F04000 + 0x100*i + 0xc);
			int w = size & 0xFFFF;
			int h = ((size >> 16) & 0xFFFF) + 1;
			if (w <= 0 || h <= 0) {
				w = 1024;
				h = 1024;
			}
			char fn[50];
			snprintf(fn, sizeof(fn), CARD_DRIVE"%x.DAT", addr);
			bmp_printf(FONT_MED, 0, 0, "%s (%dx%d)...", fn, w, h);
			dump_with_buffer(MEM(0xC0F04000 + 0x100*i + 8), w*h, fn);
		}
	}
}/*}}}*/

