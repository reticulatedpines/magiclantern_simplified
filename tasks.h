#ifndef _tasks_h_
#define _tasks_h_

/** \file
 * DryOS tasks and override functions.
 */

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


/** Return the head of the running task list */
extern struct task *
get_current_task(void);

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


/** Hook to override task dispatch */
void (*task_dispatch_hook)(
	struct context **	context
);


/** Override a DryOS task */
struct task_mapping
{
	thunk		orig;
	thunk		replacement;
};

#define TASK_OVERRIDE( orig_func, replace_func ) \
extern void orig_func( void ); \
__attribute__((section(".task_overrides"))) \
struct task_mapping task_mapping_##replace_func = { \
	.orig		= orig_func, \
	.replacement	= replace_func, \
}


#endif
