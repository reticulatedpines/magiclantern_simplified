/** \file
 * Magic Lantern GUI main task.
 *
 * Overrides the DryOS gui_main_task() to be able to re-map events.
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

#include <dryos.h>
#include <bmp.h>
#include <ml_rpc.h>
#include <config.h>
#include <consts.h>
#include "cache_hacks.h"

uint32_t ml_rpc_transferred = 0;
uint32_t ml_rpc_verbosity = 0;
uint32_t ml_rpc_available_cached = 0;

ml_rpc_request_t ml_rpc_buffer;

void ml_rpc_verbose(uint32_t state)
{
    ml_rpc_verbosity = state;
}

/* send a command with parameters to other digic. wait n*50ms for a response */
uint32_t ml_rpc_send(uint32_t command, uint32_t parm1, uint32_t parm2, uint32_t parm3, uint32_t wait)
{
    /* reset flag that signals transfer and contains result */
    ml_rpc_transferred = 0;
    
    /* build buffer to send */
    ml_rpc_buffer.message = command;
    ml_rpc_buffer.parm1 = parm1;
    ml_rpc_buffer.parm2 = parm2;
    ml_rpc_buffer.parm3 = parm3;
    
#if defined(CONFIG_7D_MASTER)
    RequestRPC(ML_RPC_ID_SLAVE, &ml_rpc_buffer, sizeof(ml_rpc_request_t), 0);    
    ml_rpc_transferred = ML_RPC_OK;
#else
    RequestRPC(ML_RPC_ID_MASTER, &ml_rpc_buffer, sizeof(ml_rpc_request_t), 0);
    
    /* now wait until we get some response */
    while(wait && (ml_rpc_transferred == 0))
    {
        msleep(50);
        wait--;
    }
#endif
    
    return ml_rpc_transferred;
}

uint32_t ml_rpc_available()
{
    if(ml_rpc_available_cached)
    {
        return ml_rpc_available_cached == 1;
    }
    
    /* wait 200ms for a successful reply */
    if(ml_rpc_send(ML_RPC_PING, *(volatile uint32_t *)0xC0242014, 0, 0, 4) == ML_RPC_PING_REPLY)
    {
        ml_rpc_available_cached = 1;
    }
    else
    {
        ml_rpc_available_cached = 2;
    }
    
    return ml_rpc_available_cached == 1;
}

/* why some commands within a struct through one RPC command and not separate RPC handlers?
   in the early steps with RPC its better to use only one communication channel that is known to 
   work with one ID that is known to be free than using many RPC IDs that may cause collisions.
   as soon we are sure that this has no side effects, we can define multiple RPC handlers - if needed.
 */
uint32_t ml_rpc_handler (uint8_t *buffer, uint32_t length)
{
    ml_rpc_request_t *req = (ml_rpc_request_t*) buffer;
    
    if(length != sizeof(ml_rpc_request_t))
    {
        ml_rpc_send(ML_RPC_ERROR, buffer, length, 0, 0);
    }
    else
    {
        ml_rpc_transferred = req->message;
        
#if defined(CONFIG_7D_MASTER)
        /* master side knows these commands nad answers without any wait for successful transmit */
        switch(req->message)
        {
            case ML_RPC_PING:
                ml_rpc_send(ML_RPC_PING_REPLY, req->parm1, req->parm3, req->parm3, 0);
                break;
                
            case ML_RPC_CACHE_HACK:
                cache_fake(req->parm1, req->parm2, req->parm3);
                ml_rpc_send(ML_RPC_OK, req->message, 0, 0, 0);
                break;
                
            case ML_RPC_CACHE_HACK_DEL:
                cache_fake(req->parm1, MEM(req->parm1), req->parm2);
                ml_rpc_send(ML_RPC_OK, req->message, 0, 0, 0);
                break;
                
            default:
                break;
        }
#else
        /* these are slave commands */
        switch(req->message)
        {
            case ML_RPC_OK:
            case ML_RPC_ERROR:
                break;
                
            case ML_RPC_PING:
                ml_rpc_send(ML_RPC_PING_REPLY, req->parm1, req->parm3, req->parm3, 0);
                break;
                
            case ML_RPC_PING_REPLY:
                if(ml_rpc_verbosity)
                {
                    bmp_printf(FONT_MED, 0, 60, "PING REPLY %d us       ", (*(volatile uint32_t *)0xC0242014 - req->parm1));
                }
                break;
                
            default:
                bmp_printf(FONT_MED, 0, 60, "RPC 0x%08X unknown", req->message);
                break;
        }
#endif
    }
    free_dma_memory(buffer);
    return 0;
}

void ml_rpc_init()
{
#if defined(CONFIG_7D_MASTER)
    RegisterRPCHandler(ML_RPC_ID_MASTER, &ml_rpc_handler);
#else
    RegisterRPCHandler(ML_RPC_ID_SLAVE, &ml_rpc_handler);
#endif
}

INIT_FUNC(__FILE__, ml_rpc_init);


