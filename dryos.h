/** \file
 * DryOS structures and functions.
 *
 * These are reverse engineered from the 5D Mark 2 firmware
 * version 1.0.7.
 *
 * \note Do not forget to update the stubs-5d2.107.S file with
 * new functions as they are added here!
 */
#ifndef _dryos_h_
#define _dryos_h_

#include "arm-mcr.h"

typedef void (*thunk)(void);


/** Panic and abort the camera */
extern void __attribute__((noreturn))
DryosPanic(
	uint32_t		arg0,
	uint32_t		arg1
);


/** Debug messages and debug manager */
extern void
DebugMsg(
	int			subsys,
	int			level,
	const char *		fmt,
	...
);

struct dm_state
{
	const char *		type; // off_0x00
	uint32_t		off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	void *			signature; // off_0x10
	uint32_t		unknown[ (788 - 0x14)/4 ];
};

extern struct dm_state * dm_state_ptr;
extern struct state_object * dm_state_object;
extern void dmstart( void ); // post the start event
extern void dmStart( void ); // initiate the start
extern void dmstop( void );
extern void dumpentire( void );
extern void dumpf( void );
extern void dm_set_store_level( uint32_t class, uint32_t level );

extern void
dm_event_dispatch(
	int			input,
	int			dwParam,
	int			dwEventId
);


/** Tasks and contexts */


struct context
{
	uint32_t		cpsr;
	uint32_t		r[13];
	uint32_t		lr;
	uint32_t		pc;
};

struct task
{
	uint32_t		off_0x00;	// always 0?
	uint32_t		off_0x04;	// stack maybe?
	uint32_t		off_0x08;	// flags?
	void *			entry;		// off 0x0c
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	char *			name;		// off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		off_0x30;
	uint32_t		off_0x34;
	uint32_t		off_0x38;
	uint32_t		off_0x3c;
	uint32_t		off_0x40;
	uint32_t		off_0x44;
	uint32_t		off_0x48;
	struct context *	context;	// off 0x4C
	uint32_t		pad_1[12];
};

#define SIZE_CHECK_STRUCT( struct_name, size ) \
	static uint8_t __attribute__((unused)) \
	__size_check_##struct_name[ \
		sizeof( struct struct_name ) == size ? 0 : -1 \
	]


/*
 * Audio information structure at 0x7324.
 * This controls the AGC system.
 */
struct audio_info
{
	uint8_t			off_0x00;
	uint8_t			off_0x01;
	uint8_t			off_0x02;
	uint8_t			off_0x03;
	struct semaphore *	sem_interval;	// off_0x04
	uint32_t		task_created;	// off_0x08
	uint32_t		asif_started;	// off_0x0c
	uint32_t		initialized;	// off_0x10
	struct semaphore *	sem_task;	// off_0x14
	uint32_t		off_0x18;
	int32_t			sample_count;	// off_0x1c
	int32_t			gain;		// off_0x20, from 0 to -41
	uint32_t		max_sample;	// off_0x24
} __attribute__((packed));

SIZE_CHECK_STRUCT( audio_info, 0x28 );

extern struct audio_info audio_info;

extern void sound_dev_start_observer( void );
extern void sound_dev_stop_observer( void );


/**
 * Sound device structure.
 */
struct sound_dev
{
	uint8_t pad0[ 0x70 ];
	struct semaphore *	sem;	 // off 0x70
};

extern struct sound_dev * sound_dev;

// Calls the unlock function when done
extern void
sound_dev_active_in(
	void			(*unlock_func)( void * ),
	void *			arg
);


/** Return the head of the running task list */
extern struct task *
get_current_task(void);


/** Put the current task to sleep for msec miliseconds */
extern void
msleep(
	int			msec
);

//CANON_FUNC( 0xFF81612C, void, sched_yield, ( void ) );
//CANON_FUNC( 0xFF816904, void, sched_yield, ( void ) );
//CANON_FUNC( 0xFF81601C, void, sched_yield, ( void ) );

/** Maybe give up the CPU; use msleep() instead */
extern void
sched_yield(
	uint32_t		must_be_zero
);


/** Official initial task.
 * \note Overridden by reboot shim.
 * \internal
 */
extern void
init_task( void );

/** Official routine to create the init task.
 * \internal
 */
extern void
create_init_task( void );

/** unknown */
extern int
task_save_state( void * buf );


extern int
RegisterEventProcedure_im1(
	const char *		name,
	void 			(*handler)( void )
);

extern int
UnregisterEventProcedure(
	const char *		name
);


extern void
EP_SetMovieManualExposureMode(
	uint32_t *		enable_ptr
);

extern void
EP_SetDebugLogMode(
	uint32_t *		enable_ptr
);

extern void
EP_SetLVAEDebugPort(
	uint32_t *		enable_ptr
);

extern void *
new_task_struct( int );


/** Create a new user level task.
 *
 * The arguments are not really known yet.
 */
extern struct task *
create_task(
	const char * name,
	uint32_t priority,
	uint32_t unknown0,
	void * entry,
	void * unknown1
);


/** Bootstrap a new task.
 * \internal
 * \note This is never directly called by the user; it is the entry
 * point used by create_task() to call the user task and then to pass
 * the return code to what ever cleans up after the task exits.
 */
extern void
task_trampoline(
	struct task *		task
);


struct semaphore;

extern struct semaphore *
create_named_semaphore(
	const char *		name,
	int			initial_value
);

extern int
take_semaphore(
	struct semaphore *	semaphore,
	int			timeout_interval
);

extern int
give_semaphore(
	struct semaphore *	semaphore
);


extern int
oneshot_timer(
	uint32_t		msec,
	void			(*handler_if_expired)(void*),
	void			(*handler)(void*),
	void *			arg
);


extern void audio_set_alc_on(void);
extern void audio_set_alc_off(void);
extern void audio_set_filter_off(void);
extern void audio_set_windcut(int);
extern void audio_set_sampling_param(int, int, int);
extern void audio_set_volume_in(int,int);
extern void audio_start_asif_observer(void);
extern void audio_level_task(void);
extern void audio_interval_unlock(void*);


/** Official Canon sound device task.
 * \internal
 */
extern void sound_dev_task(void);

/** \name File I/O flags.
 *
 * \note I don't know how many of these are supported
 * @{
 */
#define O_RDONLY             00
#define O_WRONLY             01
#define O_RDWR               02
#define O_CREAT            0100 /* not fcntl */
#define O_EXCL             0200 /* not fcntl */
#define O_NOCTTY           0400 /* not fcntl */
#define O_TRUNC           01000 /* not fcntl */
#define O_APPEND          02000
#define O_NONBLOCK        04000
#define O_NDELAY        O_NONBLOCK
#define O_SYNC           010000
#define O_FSYNC          O_SYNC
#define O_ASYNC          020000
/* @} */


extern int
open(
	const char *		name,
	int			flags,
	int			mode
);

extern void
close(
	int			fd
);

#define INVALID_PTR		((const void *)0xFFFFFFFF)

extern void *
FIO_CreateFile(
	const char *		name
);


extern int
FIO_WriteFile(
	void *			file,
	const void *		buf,
	uint32_t		len_in_bytes
);


extern void
FIO_CloseFile(
	void *			file
);

extern void
FIO_CloseSync(
	void *			file
);


extern void
write_debug_file(
	const char *		name,
	const void *		buf,
	size_t			len
);


extern void
bzero32(
	void *			buf,
	size_t			len
);



/** Firmware entry points */
extern void firmware_entry(void);
extern void reloc_entry(void);
extern void __attribute__((noreturn)) cstart(void);


/** Windowing system elements */
struct winsys_struct
{
	void *			vram_instance; // off 0x00
	struct vram_object *	vram_object; // off 0x04
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	struct semaphore *	sem; // off 0x24
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		flag_0x30;
	uint32_t		flag_0x34;
};

SIZE_CHECK_STRUCT( winsys_struct, 0x38 );

extern struct winsys_struct winsys_struct;


typedef int (*window_callback)( void * );

/** Returned by window_create_maybe() at 0xFFA6BC00 */
struct window
{
	const char *		type; // "Window Instance" at 0x14920
	uint32_t		off_0x04;	// initial=0
	uint32_t		off_0x08;	// initial=0
	uint32_t		off_0x0c;	// initial=0
	uint32_t		x;		// off_0x10;
	uint32_t		y;		// off_0x14;
	uint32_t		width;		// off_0x18; r5
	uint32_t		height;		// off_0x1c; r6
	window_callback		callback;	// off 0x20
	void *			arg;		// off_0x24;
	uint32_t		wx_maybe;	// off_0x28;
	uint32_t		wy_maybe;	// off_0x2c;
};

SIZE_CHECK_STRUCT( window, 0x30 );

extern struct window *
window_create(
	uint32_t		x,
	uint32_t		y,
	uint32_t		w,
	uint32_t		h,
	window_callback		callback,
	void *			arg
);

/** Returns 0 if it handled the message, 1 if it did not? */
typedef int (*dialog_handler_t)(
	int			self_id,
	void *			arg,
	uint32_t		event
);

struct dialog;
struct dialog_list;
struct dialog_item;


/** These are chock-full of callbacks.  I don't know what most of them do. */
struct dialog_item
{
	const char *		type;			// "DIALOGITEM" at 0xFFCA7B1c
	struct dialog_list *	next;			// maybe parent? 0x04
	struct dialog_list *	prev;			// maybe 0x08
	struct dialog_callbacks * callbacks;		// maybe object? 0x0c
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	void			(*move_callback)( struct dialog *, int x, int y ); //		off_0x2c;
	uint32_t		off_0x30;
	uint32_t		off_0x34;
	uint32_t		off_0x38;
	uint32_t		off_0x3c;
};
SIZE_CHECK_STRUCT( dialog_item, 0x40 );


/** Dialog children?  Maybe? */
struct dialog_list
{
	uint16_t		index;			// off 0x00
	uint16_t		off_0x02;
	struct dialog_item *	item;			// off 0x04 maybe
	uint32_t		arg1;			// off 0x08, passed to creat
	uint32_t		arg2;			// off_0x0c, passed to creat
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	struct dialog_list *	next;			// off 0x1c
};


/** Dialog box gui elements */
struct dialog
{
	const char *		type;			// "DIALOG" at 0x147F8
	struct window *		window;			// off 0x04
	void *			arg0;			// off 0x08
	struct langcon *	langcon;		// off 0x0c
	struct dispsw_control *	disp_sw_controller;	// off 0x10
	struct publisher *	publisher;		// off 0x14
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;		// initial=0
	uint32_t		off_0x24;		// initial=0
	uint32_t		off_0x28;		// initial=2
	uint32_t		off_0x2c;		// initial=0
	uint32_t		off_0x30;		// initial=0
	uint32_t		off_0x34;		// initial=0
	uint32_t		off_0x38;		// initial=0
	struct task *		gui_task;		// off 0x3c
	uint32_t		off_0x40;		// pointed to by 0x08
	uint32_t		off_0x44;		// initial=0
	uint32_t		off_0x48;
	uint32_t		off_0x4c;
	uint32_t		off_0x50;
	uint32_t		off_0x54;
	uint32_t		off_0x58;
	void *			child_list_maybe;	// off_0x5c;
	dialog_handler_t	handler;		// off 0x60
	uint32_t		handler_arg;		// off_0x64;
	uint32_t		off_0x68;		// initial=0
	uint16_t		off_0x6c;		// initial=0
	uint16_t		off_0x6e;		// initial=0
	uint16_t		off_0x70;
	uint16_t		off_0x72;		// initial=0
	uint32_t		off_0x74;		// initial=1
	uint32_t		off_0x78;		// initial=0
	uint32_t		off_0x7c;		// initial=0
	struct publisher *	publisher_callback;	// off_0x80;
	uint32_t		off_0x84;
	uint32_t		const_40000000_0;	// off_0x88;
	uint32_t		off_0x8c;		// initial=0xA
	uint32_t		off_0x90;		// initial=0xA
	uint32_t		id;			// off_0x94;
	uint32_t		level_maybe;		// off_0x98;
	uint32_t		const_40000000_1;	// off_0x9c;
	uint16_t		off_0xa0;		// initial=0
	uint16_t		off_0xa2;
	uint32_t		off_0xa4;
	uint32_t		off_0xa8;		// initial=0
	uint32_t		off_0xac;		// initial=0
};

SIZE_CHECK_STRUCT( dialog, 0xB0 );


extern struct dialog *
dialog_create(
	int			id,
	int			level_maybe,
	dialog_handler_t	handler,
	void *			arg1,
	void *			arg2
);

extern void
dialog_draw(
	struct dialog *		dialog
);

extern void
dialog_window_prepare(
	struct dialog *		dialog,
	void *			unused
);

/** type 0 == 720, 1 == 960? */
extern void
dialog_set_origin_type(
	struct dialog *		dialog,
	int			type
);

extern void
dialog_resize(
	struct dialog *		dialog,
	int			w,
	int			h,
	int			unknown
);

extern void
dialog_window_resize(
	struct dialog *		dialog,
	int			w,
	int			h,
	int			unknown
);

extern void
dialog_move(
	struct dialog *		dialog,
	int			x,
	int			y
);

extern void
dialog_move_item(
	struct dialog *		dialog,
	int			x,
	int			y,
	int			index
);

extern void
dialog_label_item(
	struct dialog *		dialog,
	uint32_t		id,
	const char *		label,
	int			len_maybe,
	int			unknown
);



struct lvram_info
{
	uint32_t		off_0x00;
	uint32_t		off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		width;		// off_0x18;
	uint32_t		height;		// off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		off_0x30;
	uint32_t		off_0x34;
	uint32_t		off_0x38;
	uint32_t		off_0x3c;
	uint32_t		off_0x40;
	uint32_t		off_0x44;
	uint32_t		off_0x48;
	uint32_t		off_0x4c;
	uint32_t		off_0x50;
	uint32_t		off_0x54;
	uint32_t		off_0x58;
	uint32_t		off_0x5c;
	uint32_t		off_0x60;
};
SIZE_CHECK_STRUCT( lvram_info, 0x64 );
extern struct lvram_info lvram_info;

/* Copies lvram info from 0x2f33c */
extern void
copy_lvram_info(
	struct lvram_info *	dest
);

/** Display types.
 *
 * 0 == 720p LCD
 * 3 == 960 HDMI
 * 6 == 960 HDMI
 * All others unknown.
 */
extern int
gui_get_display_type( void );


extern void
color_palette_push(
	int			palette_id
);


#define GOT_TOP_OF_CONTROL		0x800
#define LOST_TOP_OF_CONTROL		0x801
#define INITIALIZE_CONTROLLER		0x802
#define TERMINATE_WINSYS		0x804
#define DELETE_DIALOG_REQUEST		0x805
#define PRESS_RIGHT_BUTTON		0x807
#define PRESS_LEFT_BUTTON		0x809
#define PRESS_UP_BUTTON			0x80B
#define PRESS_DOWN_BUTTON		0x80D
#define PRESS_MENU_BUTTON		0x80F
#define PRESS_SET_BUTTON		0x812
#define PRESS_INFO_BUTTON		0x829
#define ELECTRONIC_SUB_DIAL_RIGHT	0x82B
#define ELECTRONIC_SUB_DIAL_LEFT	0x82C
#define PRESS_DISP_BUTTON		0x10000000
#define PRESS_DIRECT_PRINT_BUTTON	0x10000005
#define PRESS_FUNC_BUTTON		0x10000007
#define PRESS_PICTURE_STYLE_BUTTON	0x10000009
#define OPEN_SLOT_COVER			0x1000000B
#define CLOSE_SLOT_COVER		0x1000000C
#define START_SHOOT_MOVIE		0x1000008A
#define RESIZE_MAYBE			0x10000085


/** Movie recording.
 *
 * State information is in this structure.  A pointer to the global
 * object is at 0x1ee0.  It is of size 0x1b4.
 *
 * The state object is in 0x68a4.
 */
struct mvr_struct
{
	const char *		type;	 // "MovieRecorder"
};

extern struct mvr_struct * mvr_struct;
extern struct state_object * mvr_state;

/** State objects.
 *
 * Size 0x20
 */
typedef struct state_object * (*state_function_t)(
	void *			arg1,
	void *			arg2,
	void *			arg3
);

struct state_callback
{
	uint32_t		next_state;
	state_function_t	handler;
};


struct state_object
{
	const char *		type;		// off 0x00, "StateObject" 
	const char *		name;		// off 0x04, arg 0
	uint32_t		auto_sequence;	// off 0x08, arg 1

	// off 0x0c, always 0xff99a228 ?
	void			(*callback)(
		struct state_object *	self,
		void *			arg1,
		uint32_t		input,
		void *			arg3,
		void *			arg4
	);

	// Points to an array of size [max_inputs][max_states]
	struct state_callback *	callbacks;	// off 0x10
	uint32_t		max_inputs;	// off 0x14, arg 2
	uint32_t		max_states;	// off 0x18, arg 3
	uint32_t		state;		// off 0x1c, initially 0
};

SIZE_CHECK_STRUCT( state_object, 0x20 );


extern struct state_object *
state_object_create(
	const char *		name,
	int			auto_sequence,
	state_function_t *	callbacks,
	int			max_inputs,
	int			max_states
);


extern void
state_object_dispatchc(
	struct state_object *	self,
	int			input,
	int			arg0,
	int			arg1,
	int			arg2
);



/** VRAM accessors */
extern uint32_t
vram_get_number(
	uint32_t		number
);

/** Write the VRAM to a BMP file named "A:/test.bmp" */
extern void
dispcheck( void );

extern const char * vram_instance_str_ptr;

/** VRAM info structure (maybe?) */
struct vram_object
{
	const char *		name; // "Vram Instance" 0xFFCA79E5
	uint32_t		off_0x04;
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	struct semaphore *	sem; // off 0x14;
};

extern struct vram_object * 
vram_instance( void );

extern int
vram_get_lock(
	struct vram_object *	vram
);


/** VRAM info in the BSS.
 *
 * Pixels are in the format 
 */
struct vram_info
{
	uint16_t *		vram;
	uint32_t		width; // maybe
	uint32_t		pitch; // maybe
	uint32_t		height;
	uint32_t		vram_number;
};

extern struct vram_info vram_info[2];


/** Main menu tab functions */
extern int main_tab_dialog_id;
extern void main_tab_header_dialog( void );
extern void StopMnMainTabHeaderApp( void );
extern void StartMnMainRec1App( void );
extern void StartMnMainRec2App( void );
extern void StartMnMainPlay1App( void );
extern void StartMnMainPlay2App( void );
extern void StartMnMainSetup1App( void );
extern void StartMnMainSetup2App( void );
extern void StartMnMainSetup3App( void );
extern void StartMnMainCustomFuncApp( void );
extern void StartMnMainMyMenuApp( void );


/** Hidden menus */
extern void StartFactoryMenuApp( void );
extern void StartMnStudioSetupMenuApp( void );


/** stdio / stdlib / string */
extern char * strcpy( char *, const char * );
extern void * memcpy( void *, const void *, size_t );


#endif
