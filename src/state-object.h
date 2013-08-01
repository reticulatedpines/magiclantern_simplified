#ifndef _dryos_state_object_h_
#define _dryos_state_object_h_

/** \file
 * DryOS finite-state machine objects.
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

#include "compiler.h"
#include <platform/state-object.h>

#ifdef CONFIG_VXWORKS
//~ Calls CreateTaskClass from canon startup task.
//~ Ex: PropMgr, GenMgr, FileMgr
struct Manager      //~ size=0x30
{
    const char *                    name;                   //~ off_0x00    name of manager. ie: PropMgr
    struct TaskClass *              taskclass_ptr;          //~ off_0x04    pointer to taskclass struct
    const struct state_object *     stateobj_ptr;           //~ off_0x08    pointer to stateobject struct
    int                             debugmsg_class;         //~ off_0x0C    used with DebugMsg calls to determine arg0 (debug class)
    const struct unk_struct *       unk_struct_ptr;         //~ off_0x10    some unknown struct allocated, size varies
    int                             off_0x14;               //~ unknown     
    const struct semaphore *        mgr_semaphore;          //~ off_0x18    result of create_named_semaphore
    int                             off_0x1c;               //~ unknown     
    int                             off_0x20;               //~ unknown
    int                             off_0x24;               //~ unknown
    int                             off_0x28;               //~ unknown
    int                             off_0x2c;               //~ unknown
};

//~ CreateTaskClass called from canon startup task.
struct TaskClass    //~ size=0x18
{
    const char *                    identifier;             //~ off_0x00    "TaskClass"
    const char *                    name;                   //~ off_0x04    task class name. ie: PropMgr
    int                             off_0x08;               //~ unknown     initialized to 1 in CreateTaskClass
    const struct task *             task_struct_ptr;        //~ off_0x0c    ret_CreateTask (ptr to task struct) called from CreateTaskClass
    const struct msg_queue *        msg_queue_ptr_maybe;    //~ off_0x10    some kind of message queue pointer (very low level functions at work)
    void *                          eventdispatch_func_ptr; //~ off_0x14    event dispatch pointer. ie: propmgrEventDispatch
};


//~ ex: shoot capture module struct pointer stored at 0x4E74, setup in SCS_Initialize
struct Module   //~ size=0x24
{
    const char *                    name;                   //~ off_0x00    ie: "ShootCapture"
    int                             off_0x04;               //~ unknown
    struct StageClass *             stageclass_ptr;         //~ off_0x08    stageclass struct ptr
    const struct state_object *     stateobj_ptr;           //~ off_0x0C    SCSState struct ptr
    int                             debugmsg_class;         //~ off_0x10    arg0 to SCS_Initialize, used for arg0 to SCS debug messages (debug class)
    int                             off_0x14;               //~ unknown
    int                             off_0x18;               //~ unknown
    int                             off_0x1c;               //~ unknown
    int                             off_0x20;               //~ unknown  
};

//~ called during initialization of each state machine
struct StageClass   //~ size=0x1C
{
    const char *                    identifier;             //~ off_0x00    "StageClass"
    const char *                    name;                   //~ off_0x04    arg0 to CreateStageClass, name of stage class.
    int                             off_0x08;               //~ unknown     unk, set to 1 when initialized. could signal if stageclass is initialized
    const struct task *             task_struct_ptr;        //~ off_0x0c    ret_CreateTask (ptr to task struct) called from CreateStageClass
    const struct msg_queue *        msg_queue_ptr_maybe;    //~ off_0x10    some kind of message queue pointer (very low level functions at work)
    const struct JobQueue *         jobqueue_struct_ptr;    //~ off_0x14    pointer to struct created in CreateJobQueue
    void *                          eventdispatch_func_ptr; //~ off_0x18    event dispatch function pointer. ie: scsEventDispatch
};


//~ CreateJobQueue called by CreateStageClass
struct JobQueue     //~ size=0x14
{
    const char *                    identifier;             //~ off_0x00    "JobQueue"
    const struct unk_struct *       unk_struct_ptr;         //~ off_0x04    some unknown struct allocated, size is CreateJobQueue arg0*4
    int                             unk_arg0;               //~ unknown     arg0 to CreateJobQueue, unknown.
    int                             off_0x0C;               //~ unknown
    int                             off_0x10;               //~ unknown
};
#else

//~ Structures for DryOS, derived from research on VxWorks.

struct Manager
{
    const char *                    name;                   //~ off_0x00    name of manager. ie: Evf
    int                             off_0x04;               //~ off_0x04    unknown
    struct TaskClass *              taskclass_ptr;          //~ off_0x08    pointer to taskclass struct
    const struct state_object *     stateobj_ptr;           //~ off_0x0C    pointer to stateobject struct
};

struct TaskClass    //~ size=0x18
{
    const char *                    identifier;             //~ off_0x00    "TaskClass"
    const char *                    name;                   //~ off_0x04    task class name. ie: PropMgr
    int                             off_0x08;               //~ unknown     initialized to 1 in CreateTaskClass
    const struct task *             task_struct_ptr;        //~ off_0x0c    ret_CreateTask (ptr to task struct) called from CreateTaskClass
    const struct msg_queue *        msg_queue_ptr_maybe;    //~ off_0x10    some kind of message queue pointer (very low level functions at work)
    void *                          eventdispatch_func_ptr; //~ off_0x14    event dispatch pointer. ie: propmgrEventDispatch
};

#endif

/** State objects.
 *
 * Size 0x20
 */
typedef struct state_object * (*state_transition_function_t)(
        void * x,
        void * y,
        void * z,
        void * w
);

struct state_transition
{
        uint32_t                next_state;
        state_transition_function_t     state_transition_function;
};


struct state_object
{
        const char *            type;           // off 0x00, "StateObject" 
        const char *            name;           // off 0x04, arg 0
        uint32_t                auto_sequence;  // off 0x08, arg 1

        // off 0x0c, always 0xff99a228 ?
        void                    (*StateTransition_maybe)(
                struct state_object *   self,
                void *          x,
                uint32_t        input,
                void *          z,
                void *          t
        );

        // Points to an array of size [max_inputs][max_states]
        struct state_transition *       state_matrix;   // off 0x10
        uint32_t                max_inputs;     // off 0x14, arg 2
        uint32_t                max_states;     // off 0x18, arg 3
        uint32_t                current_state;          // off 0x1c, initially 0
};

SIZE_CHECK_STRUCT( state_object, 0x20 );


extern struct state_object *
CreateStateObject(
        const char *            name,
        int                     auto_sequence,
        struct state_transition *       state_matrix,
        int                     max_inputs,
        int                     max_states
);


extern void
state_object_dispatchc(
        struct state_object *   self,
        int                     input,
        int                     arg0,
        int                     arg1,
        int                     arg2
);

#define STATE_FUNC(stateobj,input,state) stateobj->state_matrix[(state) + (input) * stateobj->max_states].state_transition_function

#endif
