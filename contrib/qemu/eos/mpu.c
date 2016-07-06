#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "eos.h"
#include "model_list.h"
#include "mpu.h"

/* todo: understand the meaning of these spells,
 * rather than replaying them blindly */

// Forward declare static functions
static void mpu_send_next_spell(EOSState *s);
static void mpu_enqueue_spell(EOSState *s, int spell_set, int out_spell);
static void mpu_interpret_command(EOSState *s);

static struct mpu_init_spell * mpu_init_spells = 0;
static int mpu_init_spell_count = 0;

#define MPU_CURRENT_OUT_SPELL mpu_init_spells[s->mpu.spell_set].out_spells[s->mpu.out_spell]
/**
 * We don't know the meaning of MPU messages yet, so we'll replay them from a log file.
 * Timing is important - if we would just send everything as a response to first message,
 * the tasks that handle that message may not be started yet.
 * 
 * We will attempt to guess a causal relationship between mpu_send and mpu_recv calls.
 * Although not perfect, that guess is a good starting point.
 */

#include "mpu_spells/60D.h"
#include "mpu_spells/5D2.h"
#include "mpu_spells/5D3.h"
#include "mpu_spells/70D.h"
#include "mpu_spells/550D.h"
#include "mpu_spells/600D.h"

static void mpu_send_next_spell(EOSState *s)
{
    if (s->mpu.sq_head != s->mpu.sq_tail)
    {
        /* get next spell from the queue */
        s->mpu.spell_set = s->mpu.send_queue[s->mpu.sq_head].spell_set;
        s->mpu.out_spell = s->mpu.send_queue[s->mpu.sq_head].out_spell;
        s->mpu.sq_head = (s->mpu.sq_head+1) & (COUNT(s->mpu.send_queue)-1);
        printf("[MPU] Sending spell #%d.%d ( ", s->mpu.spell_set+1, s->mpu.out_spell+1);

        int i;
        for (i = 0; i < MPU_CURRENT_OUT_SPELL[0]; i++)
        {
            printf("%02x ", MPU_CURRENT_OUT_SPELL[i]);
        }
        printf(")\n");

        s->mpu.out_char = -2;

        /* request a SIO3 interrupt */
        eos_trigger_int(s, 0x36, 0);
    }
    else
    {
        printf("[MPU] Nothing more to send.\n");
        s->mpu.sending = 0;
    }
}

static void mpu_enqueue_spell(EOSState *s, int spell_set, int out_spell)
{
    int next_tail = (s->mpu.sq_tail+1) & (COUNT(s->mpu.send_queue)-1);
    if (next_tail != s->mpu.sq_head)
    {
        printf("[MPU] Queueing spell #%d.%d\n", spell_set+1, out_spell+1);
        s->mpu.send_queue[s->mpu.sq_tail].spell_set = spell_set;
        s->mpu.send_queue[s->mpu.sq_tail].out_spell = out_spell;
        s->mpu.sq_tail = next_tail;
    }
    else
    {
        printf("[MPU] ERROR: send queue full\n");
    }
}


static void mpu_interpret_command(EOSState *s)
{
    printf("[MPU] Received: ");
    int i;
    for (i = 0; i < s->mpu.recv_index; i++)
    {
        printf("%02x ", s->mpu.recv_buffer[i]);
    }
    
    int spell_set;
    for (spell_set = 0; spell_set < mpu_init_spell_count; spell_set++)
    {
        if (memcmp(s->mpu.recv_buffer+1, mpu_init_spells[spell_set].in_spell+1, mpu_init_spells[spell_set].in_spell[1]) == 0)
        {
            printf(" (recognized spell #%d)\n", spell_set+1);
            
            int out_spell;
            for (out_spell = 0; mpu_init_spells[spell_set].out_spells[out_spell][0]; out_spell++)
            {
                mpu_enqueue_spell(s, spell_set, out_spell);
            }
            
            if (!s->mpu.sending && s->mpu.sq_head != s->mpu.sq_tail)
            {
                s->mpu.sending = 1;
                
                /* request a MREQ interrupt */
                eos_trigger_int(s, 0x50, 0);
            }
            return;
        }
    }
    
    printf(" (unknown spell)\n");
}

void mpu_handle_sio3_interrupt(EOSState *s)
{
    if (s->mpu.sending)
    {
        int num_chars = MPU_CURRENT_OUT_SPELL[0];
        
        if (num_chars)
        {
            /* next two chars */
            s->mpu.out_char += 2;
            
            if (s->mpu.out_char < num_chars)
            {
                /*
                printf(
                    "[MPU] Sending spell #%d.%d, chars %d & %d out of %d\n", 
                    s->mpu.spell_set+1, s->mpu.out_spell+1,
                    s->mpu.out_char+1, s->mpu.out_char+2,
                    num_chars
                );
                */
                
                if (s->mpu.out_char + 2 < num_chars)
                {
                    eos_trigger_int(s, 0x36, 0);   /* SIO3 */
                }
                else
                {
                    printf("[MPU] spell #%d.%d finished\n", s->mpu.spell_set+1, s->mpu.out_spell+1);

                    if (s->mpu.sq_head != s->mpu.sq_tail)
                    {
                        printf("[MPU] Requesting next spell\n");
                        eos_trigger_int(s, 0x50, 0);   /* MREQ */
                    }
                    else
                    {
                        /* no more spells */
                        printf("[MPU] spells finished\n");
                        
                        /* we have two more chars to send */
                        s->mpu.sending = 2;
                    }
                }
            }
        }
    }

    if (s->mpu.receiving)
    {
        if (s->mpu.recv_index < s->mpu.recv_buffer[0])
        {
            /* more data to receive */
            printf("[MPU] Request more data\n");
            eos_trigger_int(s, 0x36, 0);   /* SIO3 */
        }
    }
}

void mpu_handle_mreq_interrupt(EOSState *s)
{
    if (s->mpu.sending)
    {
        mpu_send_next_spell(s);
    }
    
    if (s->mpu.receiving)
    {
        if (s->mpu.recv_index == 0)
        {
            printf("[MPU] receiving next message\n");
        }
        else
        {
            /* if a message is started in SIO3, it should continue with SIO3's, without triggering another MREQ */
            /* it appears to be harmless,  but I'm not sure what happens with more than 1 message queued */
            printf("[MPU] next message was started in SIO3\n");
        }
        eos_trigger_int(s, 0x36, 0);   /* SIO3 */
    }
}

unsigned int eos_handle_mpu(unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    /* C022009C - MPU request/status
     * - set to 0x46 at startup
     * - bic 0x2 in mpu_send
     * - orr 0x2 at the end of a message sent to the MPU (SIO3_ISR, get_data_to_send)
     * - tst 0x2 in SIO3 and MREQ ISRs
     * - should return 0x44 when sending data to MPU
     * - and 0x47 when receiving data from MPU
     */
    
    int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;
    int receive_finished = 0;

    if(type & MODE_WRITE)
    {
        int prev_value = s->mpu.status;
        s->mpu.status = value;
        
        if (value & 2)
        {
            if (s->mpu.receiving)
            {
                if (s->mpu.recv_index && s->mpu.recv_index == s->mpu.recv_buffer[0])
                {
                    msg = "Receive finished";
                    s->mpu.receiving = 0;
                    receive_finished = 1;
                }
                else
                {
                    msg = "Unknown request while receiving";
                }
            }
            else if (s->mpu.sending)
            {
                msg = "Unknown request while sending";
            }
            else /* idle */
            {
                if (value == 0x46)
                {
                    msg = "init";
                }
            }
        }
        else if (prev_value & 2)
        {
            /* receive request: transition of bit (1<<1) from high to low */
            msg = "Receive request %s";
            msg_arg1 = (intptr_t) "";
            
            if (s->mpu.receiving)
            {
                msg_arg1 = (intptr_t) "(I'm busy receiving stuff!)";
            }
            else if (s->mpu.sending)
            {
                msg_arg1 = (intptr_t) "(I'm busy sending stuff, but I'll try!)";
                s->mpu.receiving = 1;
                s->mpu.recv_index = 0;
            }
            else
            {
                s->mpu.receiving = 1;
                s->mpu.recv_index = 0;
                eos_trigger_int(s, 0x50, 0);   /* MREQ */
                /* next steps in eos_handle_mreq -> mpu_handle_mreq_interrupt */
            }
        }
    }
    else
    {
        if (s->mpu.sending == 2)
        {
            /* last two chars sent, finished */
            s->mpu.sending = 0;
        }
        
        ret = (s->mpu.sending && !s->mpu.receiving) ? 0x3 :  /* I have data to send */
              (!s->mpu.sending && s->mpu.receiving) ? 0x0 :  /* I'm ready to receive data */
              (s->mpu.sending && s->mpu.receiving)  ? 0x1 :  /* I'm ready to send and receive data */
                                                      0x2 ;  /* I believe this is some error code */
        ret |= (s->mpu.status & 0xFFFFFFFC);                 /* The other bits are unknown;
                                                                they are set to 0x44 by writing to the register */

        msg = "status (sending=%d, receiving=%d)";
        msg_arg1 = s->mpu.sending;
        msg_arg2 = s->mpu.receiving;
    }

    io_log("MPU", s, address, type, value, ret, msg, msg_arg1, msg_arg2);

    if (receive_finished)
    {
        mpu_interpret_command(s);
    }
    
    return ret;
}

static int mpu_handle_get_data(EOSState *s, int *hi, int *lo)
{
    if (s->mpu.spell_set < mpu_init_spell_count &&
        s->mpu.out_spell >= 0 &&
        s->mpu.out_char >= 0 && s->mpu.out_char < MPU_CURRENT_OUT_SPELL[0])
    {
        *hi = MPU_CURRENT_OUT_SPELL[s->mpu.out_char];
        *lo = MPU_CURRENT_OUT_SPELL[s->mpu.out_char+1];
        return 1;
    }
    return 0;
}

unsigned int eos_handle_sio3( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;

    switch(address & 0xFF)
    {
        case 0x04:
            /* C0820304 
             * 
             * write:
             *      - confirm data sent to MPU, by writing 1; used together with C0820318
             *      - when sending data from the MPU, each char pair is confirmed
             *        by sending a 0 back on C0820318, followed by a 1 here
             *        but the last char pair is not confirmed
             * 
             * read:
             *      - tst 0x1 in SIO3 when sending to MPU, stop if NE
             *      - guess: 0 if idle, 1 if busy
             */
            
            if(type & MODE_WRITE)
            {
                if (value == 1)
                {
                    msg = "To MPU <- ack data";
                }
                else
                {
                    msg = "To MPU <- ???";
                }
            }
            else
            {
                msg = "request to send?";
            }
            break;

        case 0x10:  /* C0820310: set to 0 at the beginning of SIO3_ISR, not used anywhere else */
            if(type & MODE_WRITE)
            {
                if (value == 0)
                {
                    msg = "ISR started";
                    mpu_handle_sio3_interrupt(s);
                }
            }
            break;

        case 0x18:  /* C0820318 - data sent to MPU */
            if(type & MODE_WRITE)
            {
                if (s->mpu.receiving)
                {
                    msg = "Data to MPU, at index %d %s";
                    msg_arg1 = s->mpu.recv_index;
                    if (s->mpu.recv_index + 2 < COUNT(s->mpu.recv_buffer))
                    {
                        s->mpu.recv_buffer[s->mpu.recv_index++] = (value >> 8) & 0xFF;
                        s->mpu.recv_buffer[s->mpu.recv_index++] = (value >> 0) & 0xFF;
                    }
                    else
                    {
                        msg_arg2 = (intptr_t) "(overflow!)";
                    }
                }
                else if (s->mpu.sending && value == 0)
                {
                    msg = "Dummy data to MPU";
                }
                else
                {
                    msg = "Data to MPU (wait a minute, I'm not listening!)";
                }
            }
            break;

        case 0x1C:  /* C082031C - data coming from MPU */
        
            if(type & MODE_WRITE)
            {
                msg = "Data from MPU (why writing?!)";
            }
            else
            {
                if (s->mpu.sending)
                {
                    int hi = 0, lo = 0;
                    if (mpu_handle_get_data(s, &hi, &lo)) {
                        ret = (hi << 8) | lo;
                        msg = "Data from MPU";
                    } else {
                        msg = "From MPU -> out of range (cmd %d, char %d)";
                        msg_arg1 = s->mpu.out_spell;
                        msg_arg2 = s->mpu.out_char;
                        ret = 0;
                    }
                }
                else
                {
                    msg = "No data from MPU";
                    ret = 0;
                }
            }
            break;
    }

    io_log("SIO3", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

unsigned int eos_handle_mreq( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;
    
    if ((address & 0xFF) == 0x2C)
    {
        /* C020302C */
        /* set to 0x1C at the beginning of MREQ ISR, and to 0x0C at startup */
        if(type & MODE_WRITE)
        {
            msg = "CTL register %s";
            if (value == 0x0C)
            {
                msg_arg1 = (intptr_t) "init";
            }
            else if (value == 0x1C)
            {
                msg_arg1 = (intptr_t) "(ISR started)";
                mpu_handle_mreq_interrupt(s);
            }
        }
        else
        {
            msg = "CTL register -> idk, sending 0xC";
            ret = 0xC;
        }
    }
    
    io_log("MREQ", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

void mpu_spells_init(EOSState *s)
{
#define MPU_SPELL_SET(cam) \
    if (strcmp(s->model->name, #cam) == 0) { \
        mpu_init_spells = mpu_init_spells_##cam; \
        mpu_init_spell_count = COUNT(mpu_init_spells_##cam); \
    }

#define MPU_SPELL_SET_OTHER_CAM(cam1,cam2) \
    if (strcmp(s->model->name, #cam1) == 0) { \
        mpu_init_spells = mpu_init_spells_##cam2; \
        mpu_init_spell_count = COUNT(mpu_init_spells_##cam2); \
    }
    
    MPU_SPELL_SET(60D)
    MPU_SPELL_SET(5D2)
    MPU_SPELL_SET(70D)
    MPU_SPELL_SET(5D3)
    MPU_SPELL_SET(550D)
    MPU_SPELL_SET(600D)

    /* 1200D works with 60D MPU spells... and BOOTS THE GUI!!! */
    MPU_SPELL_SET_OTHER_CAM(1200D, 60D)
    
    /* same for 1100D */
    MPU_SPELL_SET_OTHER_CAM(1100D, 60D)

    //~ MPU_SPELL_SET_OTHER_CAM(600D, 60D)
    
    if (!mpu_init_spell_count)
    {
        printf("FIXME: no MPU spells for %s.\n", s->model->name);
        /* how to get them: http://magiclantern.fm/forum/index.php?topic=2864.msg166938#msg166938 */
    }
}
