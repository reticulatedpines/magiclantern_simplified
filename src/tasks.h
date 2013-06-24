#ifndef _tasks_h_
#define _tasks_h_

/** \file
 * DryOS tasks and override functions.
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "dryos.h"

struct context
{
        uint32_t                cpsr;           // off_0x00;
        uint32_t                r[13];          // off_0x04;
        uint32_t                lr;             // off_0x38;
        uint32_t                pc;             // off_0x3C;
}; // 0x40 bytes

struct task
{
        uint32_t                off_0x00;       // always 0?
        uint32_t                off_0x04;       // stack maybe?
        uint32_t                run_prio;       // flags?
        void *                  entry;          // off 0x0c
        uint32_t                arg;            // off_0x10;
        uint32_t                waitObjId;
        uint32_t                off_0x18;
        uint32_t                stackStartAddr;
        uint32_t                stackSize;
        char *                  name;           // off_0x24;
        uint32_t                off_0x28;
        uint32_t                off_0x2c;
        uint32_t                self;
        uint32_t                off_0x34;
        uint32_t                off_0x38;
        uint32_t                off_0x3c;
        uint32_t                taskId;
        uint32_t                off_0x44;
        uint8_t                 off_0x48;
        uint8_t                 currentState;
        uint8_t                 off_0x4a;
        uint8_t                 yieldRequest;
        uint8_t                 off_0x4c;
        uint8_t                 sleepReason;
        uint8_t                 off_0x4e;
        uint8_t                 off_0x4f;
        struct context *        context;        // off 0x4C
};


struct task_attr_str {
  unsigned int entry;
  unsigned int args;
  unsigned int stack;
  unsigned int size;
  unsigned int used; // 0x10
  void* name;
  unsigned int off_18;
  unsigned int flags;
  unsigned char wait_id;
  unsigned char pri;
  unsigned char state;
  unsigned char fpu;
  unsigned int id;
}; // size = 0x28

/** Return the head of the running task list */
extern struct task *
get_current_task(void);

/** Official initial task.
 * \note Overridden by reboot shim.
 * \internal
 */
extern int
init_task( int a, int b, int c, int d );

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
        struct task *           task
);


/** Hook to override task dispatch */
void (*task_dispatch_hook)(
        struct context **       context
);


/** Override a DryOS task */
struct task_mapping
{
        thunk           orig;
        thunk           replacement;
};

#define TASK_OVERRIDE( orig_func, replace_func ) \
extern void orig_func( void ); \
__attribute__((section(".task_overrides"))) \
struct task_mapping task_mapping_##replace_func = { \
        .orig           = orig_func, \
        .replacement    = replace_func, \
}


/** Auto-create tasks */
struct task_create
{
        const char *            name;
        void                    (*entry)( void * );
        int                     priority;
        uint32_t                stack_size;
        void *                  arg;
};

#define TASK_CREATE( NAME, ENTRY, ARG, PRIORITY, STACK_SIZE ) \
struct task_create \
__attribute__((section(".tasks"))) \
task_create_##ENTRY = { \
        .name           = NAME, \
        .entry          = ENTRY, \
        .arg            = ARG, \
        .priority       = PRIORITY, \
        .stack_size     = STACK_SIZE, \
}

#define INIT_FUNC( NAME, ENTRY ) \
struct task_create \
__attribute__((section(".init_funcs"))) \
task_create_##ENTRY = { \
        .name           = NAME, \
        .entry          = ENTRY, \
}

#ifdef _TASKS_C
int ml_shutdown_requested = 0;
#else
extern int ml_shutdown_requested;
#endif

#define TASK_LOOP for (int k = 0; !ml_shutdown_requested ; k++)


char * get_task_name_from_id(int id);

#endif
