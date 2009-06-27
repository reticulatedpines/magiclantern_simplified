#ifndef _debug_h_
#define _debug_h_
/** \file
 * Debug macros and functions.
 */

/** Debug messages and debug manager */
extern void
DebugMsg(
	int			subsys,
	int			level,
	const char *		fmt,
	...
);


extern const char * dm_names[];

// To find these, look at the dm_names table at 0x292C
// Subtract the pointer from 0x292c and divide by 4
#define DM_SETPROP	0
#define DM_PRP		1
#define DM_PROPAD	2
#define DM_INTCOM	3
#define DM_WINSYS	4
#define DM_CTRLSRV	5
#define DM_LVCAF	6
#define DM_LVAF		7
#define DM_LVCAE	8
#define DM_LVAE		9
#define DM_LVFD		10
#define DM_LVMD		11
#define DM_LVCLR	12
#define DM_LVWB		13
#define DM_DP		14
#define DM_MAC		15
#define DM_CRP		16
#define DM_UPDC		17
#define DM_SYS		18
#define DM_RD		19
#define DM_AUDIO	20
#define DM_ENGP		0x16
#define DM_MOVR		48
#define DM_MAGIC	50 // Replaces PTPCOM with MAGIC
#define DM_DISP		0x82

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

#endif
