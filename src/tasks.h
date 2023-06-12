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

#ifdef CONFIG_DIGIC_678X
int get_task_info_by_id(int, int, void*);
extern int _get_task_info_by_id(int, void*);
#else
extern int get_task_info_by_id(int, int, void*);
#endif

#ifdef CONFIG_DIGIC_678X
// SJE - I believe this has changed because ARMv6 introduced RFE,
// which Digic7 is using to restore context when task switching.
// See 200D, 1.0.1, e0274f3a onwards.
//
// The context for a task seems to always be stored on the stack.
//
// Because you have to do RFE last, as it changes PC, and because
// RFE restores CPSR and PC, it's natural to put everything else
// before them on the stack, so you can pop then RFE.
    struct context
    {
        uint32_t r[13]; // off_0x00;
        uint32_t lr;    // off_0x34;
        uint32_t pc;    // off_0x38;
        uint32_t cpsr;  // off_0x3c;
    }; // 0x40 bytes
#else
    struct context
    {
        uint32_t cpsr;  // off_0x00;
        uint32_t r[13]; // off_0x04;
        uint32_t lr;    // off_0x38;
        uint32_t pc;    // off_0x3C;
    }; // 0x40 bytes
#endif

#ifdef CONFIG_NEW_TASK_STRUCTS
// ==== WARNING ====
// This code looks sane if you're used to thinking about building cams.
// But modules don't set this.  That means they always use the old structs,
// which could cause bugs if offsets of fields differ.  This is the way
// they've always behaved, however.
//
// Specifically, a few use get_current_task_name(), which is inlined from
// tasks.h.  That ends up inlining the offset of task.name.
// The next step to fixing this is stopping it being inlined,
// so that module build treats it as an external symbol and resolves it
// at runtime on the cam, not at build.
#if defined(CONFIG_TASK_STRUCT_V1)
// The earliest identified task struct, somewhere around the Digic 4 era.
// 50D uses this.
// Later D4 cams, e.g. 1300D, don't.
//
// Field names may not be correct, these are copied back from later
// versions.
struct task
{
//      type            name            offset, size
    struct task    *prev_task;      // 0x00, 4
    struct task    *next_task;      // 0x04, 4
    uint32_t        run_prio;       // 0x08, 4   // lower value is higher priority
    void           *entry;          // 0x0c, 4
    uint32_t        arg;            // 0x10, 4
    uint32_t        waitObjId;      // 0x14, 4
    uint32_t            unknown_03; // 0x18, 4
    uint32_t        stackStartAddr; // 0x1c, 4
    uint32_t        stackSize;      // 0x20, 4
    char           *name;           // 0x24, 4
    uint32_t            unknown_04; // 0x28, 4
    uint32_t            unknown_05; // 0x2c, 4
    struct task    *self;           // 0x30, 4
    uint32_t            unknown_06; // 0x34, 4
    uint32_t            unknown_07; // 0x38, 4
    uint32_t            unknown_08; // 0x3c, 4
    uint32_t        taskId;         // 0x40, 4
    uint8_t             unknown_0a; // 0x44, 1
    int8_t          currentState;   // 0x45, 1
    uint8_t             unknown_0b; // 0x46, 1
    uint8_t         yieldRequest;   // 0x47, 1
    uint8_t             unknown_0c; // 0x48, 1
    uint8_t         sleepReason;    // 0x49, 1
    uint8_t             unknown_0d; // 0x4a, 1
    uint8_t             unknown_0e; // 0x4b, 1
    struct context  *context;       // 0x4c, 4
};
SIZE_CHECK_STRUCT(task, 0x50);
#elif defined(CONFIG_TASK_STRUCT_V2)
// This version arrives somewhere around Digic 5.
// 7D2 has this.
struct task
{
//      type            name            offset, size
    struct task    *prev_task;      // 0x00, 4   // SJE not sure what these two fields are used for,
    struct task    *next_task;      // 0x04, 4   // but they're doubly-linked lists of tasks
    uint32_t        run_prio;       // 0x08, 4   // lower value is higher priority
    void           *entry;          // 0x0c, 4
    uint32_t        arg;            // 0x10, 4
    uint32_t        waitObjId;      // 0x14, 4
    uint32_t            unknown_03; // 0x18, 4
    uint32_t        stackStartAddr; // 0x1c, 4
    uint32_t        stackSize;      // 0x20, 4
    char           *name;           // 0x24, 4
    uint32_t            unknown_04; // 0x28, 4
    uint32_t            unknown_05; // 0x2c, 4
    struct task    *self;           // 0x30, 4
    uint32_t            unknown_06; // 0x34, 4
    uint32_t            unknown_07; // 0x38, 4
    uint32_t            unknown_08; // 0x3c, 4
    uint32_t        taskId;         // 0x40, 4 // size 4, but low 16-bits are used as task index.
                                               // Comparison against taskId is done with full 32 in *some*
                                               // APIs though, at least on D678, so the upper bits
                                               // mean something different.  They seem to be monotonic,
                                               // so I suspect they're used to avoid collisions
    uint32_t            unknown_09; // 0x44, 4
    uint8_t             unknown_0a; // 0x48, 1
    int8_t          currentState;   // 0x49, 1
    uint8_t             unknown_0b; // 0x4a, 1
    uint8_t         yieldRequest;   // 0x4b, 1
    uint8_t             unknown_0c; // 0x4c, 1
    uint8_t         sleepReason;    // 0x4d, 1
    uint8_t             unknown_0d; // 0x4e, 1
    uint8_t             unknown_0e; // 0x4f, 1
    struct context  *context;       // 0x50, 4
};
SIZE_CHECK_STRUCT(task, 0x54);
#elif defined(CONFIG_TASK_STRUCT_V2_SMP)
// This version has dual-core fields, seen starting
// with D7.
struct task
{
//      type            name            offset, size
    struct task    *prev_task;      // 0x00, 4   // SJE not sure what these two fields are used for,
    struct task    *next_task;      // 0x04, 4   // but they're doubly-linked lists of tasks
    uint32_t        run_prio;       // 0x08, 4   // lower value is higher priority
    void           *entry;          // 0x0c, 4
    uint32_t        arg;            // 0x10, 4
    uint32_t        waitObjId;      // 0x14, 4
    uint32_t            unknown_03; // 0x18, 4
    uint32_t        stackStartAddr; // 0x1c, 4
    uint32_t        stackSize;      // 0x20, 4
    char           *name;           // 0x24, 4
    uint32_t            unknown_04; // 0x28, 4
    uint32_t            unknown_05; // 0x2c, 4
    struct task    *self;           // 0x30, 4
    uint32_t            unknown_06; // 0x34, 4
    uint32_t            unknown_07; // 0x38, 4
    uint32_t            unknown_08; // 0x3c, 4
    uint32_t        taskId;         // 0x40, 4 // size 4, but low 16-bits are used as task index.
                                               // Comparison against taskId is done with full 32 in *some*
                                               // APIs though, at least on D678, so the upper bits
                                               // mean something different.
    uint32_t            unknown_09; // 0x44, 4
    uint8_t             unknown_0a; // 0x48, 1
    int8_t          currentState;   // 0x49, 1
    uint8_t             unknown_0b; // 0x4a, 1
    uint8_t         yieldRequest;   // 0x4b, 1
    uint8_t             unknown_0c; // 0x4c, 1
    uint8_t         sleepReason;    // 0x4d, 1
    uint8_t             unknown_0d; // 0x4e, 1
    uint8_t             unknown_0e; // 0x4f, 1
    uint8_t         cpu_requested;  // 0x50, 1 // Which CPU can take the task.  0xff means any.
    uint8_t         cpu_assigned;   // 0x51, 1 // Which CPU has taken the task,
                                               // 0xff means not yet taken.
                                               // See df0028a2, 200D 1.0.1, which
                                               // I believe is "int get_task_for_cpu(int cpu_id)"
    uint8_t             unknown_11; // 0x52, 1
    uint8_t             unknown_12; // 0x53, 1
    struct context  *context;       // 0x54, 4
    uint32_t            unknown_13; // 0x58, 4
};
SIZE_CHECK_STRUCT(task, 0x5c);
#endif

#if defined(CONFIG_TASK_ATTR_STRUCT_V1)
// Early ML code defines this struct as size 0x28, with an "id" field
// as the last member.  I haven't seen any roms where this is true,
// so I don't know where (or if!) it's used.
//
// I think this derives from work Indy did on 550d, I've examined that cam,
// and it uses V2 as far as I can tell.  Perhaps size 0x28 was a mistake.
// Perhaps I'm missing some access to the last field.
#elif defined(CONFIG_TASK_ATTR_STRUCT_V2)
// e.g. 70D
struct task_attr_str {
    uint32_t entry;     // 0x00, 4
    uint32_t args;      // 0x04, 4
    uint32_t stack;     // 0x08, 4
    uint32_t size;      // 0x0c, 4
    uint32_t used;      // 0x10, 4
    void *name;         // 0x14, 4
    uint32_t task_id;   // 0x18, 4 // NB later variants don't have this field
    uint32_t flags;     // 0x1c, 4
    uint8_t wait_id;    // 0x20, 1
    uint8_t pri;        // 0x21, 1
    uint8_t state;      // 0x22, 1
    uint8_t fpu;        // 0x23, 1
};
SIZE_CHECK_STRUCT(task_attr_str, 0x24);
#elif defined(CONFIG_TASK_ATTR_STRUCT_V3)
// Used on early D6, e.g. 7D2
struct task_attr_str {
    uint32_t state;         // 0x00, 4
    uint32_t pri;           // 0x04, 4
    uint32_t unknown_0b;    // 0x08, 4
    uint32_t entry;         // 0x0c, 4
    uint32_t args;          // 0x10, 4
    uint32_t wait_id;       // 0x14, 4
    uint32_t flags;         // 0x18, 4
    uint32_t stack;         // 0x1c, 4
    uint32_t size;          // 0x20, 4
    uint32_t used;          // 0x24, 4
    uint32_t unknown_01;    // 0x28, 4
    struct context *context;// 0x2c, 4
    char *name;             // 0x30, 4
};
SIZE_CHECK_STRUCT(task_attr_str, 0x34);
#elif defined(CONFIG_TASK_ATTR_STRUCT_V4)
// Seen on later D6 cams, no dual-core fields.
// e.g. 80D 1.0.3
struct task_attr_str {
    uint32_t state;         // 0x00, 4
    uint32_t pri;           // 0x04, 4
    uint32_t unknown_0b;    // 0x08, 4
    uint32_t entry;         // 0x0c, 4
    uint32_t args;          // 0x10, 4
    uint32_t wait_id;       // 0x14, 4
    uint32_t flags;         // 0x18, 4
    uint32_t stack;         // 0x1c, 4
    uint32_t size;          // 0x20, 4
    uint32_t used;          // 0x24, 4
    uint32_t unknown_01;    // 0x28, 4
    uint32_t unknown_02;    // 0x2c, 4
    struct context *context;// 0x30, 4
    char *name;             // 0x34, 4
};
SIZE_CHECK_STRUCT(task_attr_str, 0x38);
#elif defined(CONFIG_TASK_ATTR_STRUCT_V5)
// Seen on D7 cams, has dual-core fields.
struct task_attr_str {
    uint32_t state;         // 0x00, 4
    uint32_t pri;           // 0x04, 4
    uint32_t unknown_0b;    // 0x08, 4
    uint32_t entry;         // 0x0c, 4
    uint32_t args;          // 0x10, 4
    uint32_t wait_id;       // 0x14, 4
    uint32_t flags;         // 0x18, 4
    uint32_t stack;         // 0x1c, 4
    uint32_t size;          // 0x20, 4
    uint32_t used;          // 0x24, 4
    uint32_t cpu_requested; // 0x28, 4
    uint32_t cpu_assigned;  // 0x2c, 4
    struct context *context;// 0x30, 4
    uint32_t unknown_13;    // 0x34, 4
    char *name;             // 0x38, 4
};
SIZE_CHECK_STRUCT(task_attr_str, 0x3c);
#else
#error "You must determine which task and task_attr version to use and define it in internals.h"
#endif

#else // not CONFIG_NEW_TASK_STRUCTS
struct task
{
//      type            name            offset, size
        struct task    *prev_task;      // 0x00, 4   // SJE not sure what these two fields are used for,
        struct task    *next_task;      // 0x04, 4   // but they're doubly-linked lists of tasks
        uint32_t        run_prio;       // 0x08, 4   // lower value is higher priority
        void           *entry;          // 0x0c, 4
        uint32_t        arg;            // 0x10, 4
        uint32_t        waitObjId;      // 0x14, 4
        uint32_t            unknown_03; // 0x18, 4
        uint32_t        stackStartAddr; // 0x1c, 4
        uint32_t        stackSize;      // 0x20, 4
        char           *name;           // 0x24, 4
        uint32_t            unknown_04; // 0x28, 4
        uint32_t            unknown_05; // 0x2c, 4
        struct task    *self;           // 0x30, 4
        uint32_t            unknown_06; // 0x34, 4
        uint32_t            unknown_07; // 0x38, 4
        uint32_t            unknown_08; // 0x3c, 4
        uint32_t        taskId;         // 0x40, 4 // size 4, but low 16-bits are used as task index.
                                                   // Comparison against taskId is done with full 32 in *some*
                                                   // APIs though, at least on D678, so the upper bits
                                                   // mean something different.
#if defined(CONFIG_DIGIC_678X) || defined(CONFIG_70D) // Confirmed on 750D (D6), 200D (D7), M50, R, RP (D8)
        uint32_t            unknown_09; // 0x44, 4
#endif
        uint8_t             unknown_0a; // 0x44 / 0x48, 1
        int8_t          currentState;   // 0x45 / 0x49, 1
        uint8_t             unknown_0b; // 0x46 / 0x4a, 1
        uint8_t         yieldRequest;   // 0x47 / 0x4b, 1
        uint8_t             unknown_0c; // 0x48 / 0x4c, 1
        uint8_t         sleepReason;    // 0x49 / 0x4d, 1
        uint8_t             unknown_0d; // 0x4a / 0x4e, 1
        uint8_t             unknown_0e; // 0x4b / 0x4f, 1
#ifdef CONFIG_DIGIC_78X // DNE on D6, exists on D7, D8
        uint8_t         cpu_requested; // 0x50, 1 // SJE working theory: which CPU can
                                                  // take the task.  0xff means any.
        uint8_t         cpu_assigned; // 0x51, 1  // Which CPU has taken the task,
                                                  // 0xff means not yet taken.
                                                  // See df0028a2, 200D 1.0.1, which
                                                  // I believe is "int get_task_for_cpu(int cpu_id)"
        uint8_t             unknown_11; // 0x52, 1
        uint8_t             unknown_12; // 0x53, 1
        struct context  *context;       // 0x54, 4
        uint32_t            unknown_13; // 0x58, 4
#else // D6 ends with *context
        struct context  *context;       // 0x4c, 4
#endif
                                        // 0x50 / 0x5c // sizeof struct
};


#ifdef CONFIG_DIGIC_678X
// NB, these fields get copied from a struct task,
// and the effective types seems to change.  I guess this
// is just down to alignment of the structs (the asm loads a byte,
// but stores a word).
//
// SJE FIXME - I have updated this struct purely from reversing,
// it is currently only lightly tested.  I also haven't audited current ML
// usage of this struct.  It has new fields now, which old code
// might not populate, and DryOS might require, etc.
//
// kitor: Digic 6 is single core. Uses new structure, but without CPU fields.
struct task_attr_str {
  unsigned int state;
  unsigned int pri;
  unsigned int unknown_0b;

  unsigned int entry;
  unsigned int args;
  unsigned int wait_id;
  unsigned int flags;
  unsigned int stack;
  unsigned int size;
  unsigned int used;
#ifdef CONFIG_DIGIC_78X
  unsigned int cpu_requested;
  unsigned int cpu_assigned;
#endif
#ifdef CONFIG_80D
  unsigned int unk1;
  unsigned int unk2;
#endif
  unsigned int context;
#ifndef CONFIG_80D
  unsigned int unknown_13;
#endif
  char *name;
}; // size = 0x34 (D6) 0x3c (D78)
#else
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
#endif
#endif // CONFIG_NEW_TASK_STRUCTS

extern struct task *first_task;

/** The head of the running task list */
extern struct task *current_task;

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
int get_current_task_id();

const char *get_current_task_name();

/* to refactor with CBR */
void task_update_loads(); // called every second from clock_task

#endif
