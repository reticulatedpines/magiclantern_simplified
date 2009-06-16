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


#define DM_SYS		18
#define DM_AUDIO	20

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
