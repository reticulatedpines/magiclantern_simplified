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
        const char *            task_name;      // off_0x24; please use get_current_task_name() instead
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

/** The head of the running task list */
extern struct task * current_task;

/** Current interrupt ( << 2 on D4/5, exact value on D2/3/6) */
extern uint32_t current_interrupt;

/** Official initial task.
 * \note Overridden by reboot shim.
 * \internal
 */
extern int
init_task( int a, int b, int c, int d );

typedef int (*init_task_func)(int,int,int,int);

/** Official routine to create the init task.
 * \internal
 */
extern void
create_init_task();

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
extern void (*task_dispatch_hook)();


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

extern int ml_shutdown_requested;

#define TASK_LOOP for (int k = 0; !ml_shutdown_requested ; k++)


const char * get_task_name_from_id(int id);

static inline const char * get_current_task_name()
{
#if 0
    /* DryOS: right after current_task we have a flag
     * set to 1 when handling an interrupt */
    /* this doesn't work on DIGIC 7 */
    uint32_t interrupt_active = *(volatile uint32_t *)((uintptr_t)&current_task + 4);
#endif

    /* DryOS: right before interrupt_active we have a counter showing the interrupt nesting level */
    uint32_t interrupt_level = *(volatile uint32_t *)((uintptr_t)&current_interrupt - 4);

    if (!interrupt_level)
    {
        return current_task->task_name;
    }
    else
    {
        static char isr[] = "**INT-00h**";
#if defined(CONFIG_DIGIC_VI) || defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)
        int i = current_interrupt;
#else
        int i = current_interrupt >> 2;
#endif
        int i0 = (i & 0xF);
        int i1 = (i >> 4) & 0xF;
        int i2 = (i >> 8) & 0xF;
        isr[5] = i2 ? '0' + i2 : '-';
        isr[6] = i1 < 10 ? '0' + i1 : 'A' + i1 - 10;
        isr[7] = i0 < 10 ? '0' + i0 : 'A' + i0 - 10;
        return isr;
    }
}

/* to refactor with CBR */
void task_update_loads(); // called every second from clock_task

#endif
