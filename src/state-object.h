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

#define STATE_FUNC(stateobj,input,state) stateobj->state_matrix[(state) + (input) * stateobj->max_states].state_transition_function

/* waits for N LiveView frames (using state object vsync) */
int wait_lv_frames(int num_frames);

#endif
