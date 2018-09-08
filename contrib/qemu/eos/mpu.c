#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "eos.h"
#include "model_list.h"
#include "mpu.h"

/* todo: understand the meaning of these spells,
 * rather than replaying them blindly */

#define MPU_DPRINTF(fmt, ...) DPRINTF("[MPU] ", EOS_LOG_MPU, fmt, ## __VA_ARGS__)
#define MPU_EPRINTF(fmt, ...) EPRINTF("[MPU] ", EOS_LOG_MPU, fmt, ## __VA_ARGS__)
#define MPU_VPRINTF(fmt, ...) VPRINTF("[MPU] ", EOS_LOG_MPU, fmt, ## __VA_ARGS__)
#define MPU_DPRINTF0(fmt, ...) DPRINTF("",      EOS_LOG_MPU, fmt, ## __VA_ARGS__)
#define MPU_EPRINTF0(fmt, ...) EPRINTF("",      EOS_LOG_MPU, fmt, ## __VA_ARGS__)

// Forward declare static functions
static void mpu_send_next_spell(EOSState *s);
static void mpu_enqueue_spell(EOSState *s, int spell_set, int out_spell, uint16_t * copied_spell);
static void mpu_interpret_command(EOSState *s);

static struct mpu_init_spell * mpu_init_spells = 0;
static int mpu_init_spell_count = 0;

/**
 * We don't know the meaning of MPU messages yet, so we'll replay them from a log file.
 * Timing is important - if we would just send everything as a response to first message,
 * the tasks that handle that message may not be started yet.
 * 
 * We will attempt to guess a causal relationship between mpu_send and mpu_recv calls.
 * Although not perfect, that guess is a good starting point.
 */

#include "mpu_spells/5D2.h"
#include "mpu_spells/5D3.h"
#include "mpu_spells/6D.h"
#include "mpu_spells/50D.h"
#include "mpu_spells/60D.h"
#include "mpu_spells/70D.h"
#include "mpu_spells/450D.h"
#include "mpu_spells/500D.h"
#include "mpu_spells/550D.h"
#include "mpu_spells/600D.h"
#include "mpu_spells/700D.h"
#include "mpu_spells/100D.h"
#include "mpu_spells/EOSM.h"
#include "mpu_spells/EOSM2.h"
#include "mpu_spells/generic.h"
#include "mpu_spells/bruteforce.h"

#include "mpu_spells/known_spells.h"

static const char * mpu_spell_generic_description(uint16_t * spell)
{
    for (int i = 0; i < COUNT(known_spells); i++)
    {
        if (spell[0] && spell[1])
        {
            if (spell[2] == 6)
            {
                return "GUI_SWITCH";
            }

            if (spell[2] == known_spells[i].class &&
                spell[3] == known_spells[i].id)
            {
                return known_spells[i].description;
            }
        }
    }

    return 0;
}

static void mpu_send_next_spell(EOSState *s)
{
    if (s->mpu.sq_head != s->mpu.sq_tail)
    {
        /* get next spell from the queue */
        /* note: send_queue always contains copies of messages
         * allocated on mpu_enqueue_spell* and freed here */
        free(s->mpu.out_spell);
        s->mpu.out_spell = s->mpu.send_queue[s->mpu.sq_head];
        s->mpu.sq_head = (s->mpu.sq_head+1) & (COUNT(s->mpu.send_queue)-1);
        MPU_EPRINTF("Sending : ");

        for (int i = 0; i < s->mpu.out_spell[0]; i++)
        {
            MPU_EPRINTF0("%02x ", s->mpu.out_spell[i]);
        }

        const char * desc = mpu_spell_generic_description(s->mpu.out_spell);
        MPU_EPRINTF0(" (%s)\n", desc ? desc : KLRED"unnamed"KRESET);

        s->mpu.out_char = -2;

        /* request a SIO3 interrupt */
        /* use 100 here if brute-forcing MPU spells, to avoid overflowing Canon buffers */
        eos_trigger_int(s, s->model->mpu_sio3_interrupt, 0);
    }
    else
    {
        MPU_DPRINTF("Nothing more to send.\n");
        s->mpu.sending = 0;
    }
}

static uint16_t * copy_n_subst_spell(uint16_t * spell, uint16_t * template, uint16_t * received)
{
    int len = spell[0] * sizeof(spell[0]);
    uint16_t * copy = malloc(len);
    assert(copy);
    memcpy(copy, spell, len);
    for (int i = 0; i < spell[0]; i++)
    {
        if ((copy[i] >> 8) == 1)
        {
            /* some argument taken from received spell */
            assert(template);
            assert(received);
            assert(template[0] == received[0]);
            for (int j = 0; j < template[0]; j++)
            {
                if (copy[i] == template[j])
                {
                    copy[i] = received[j];
                }
            }
        }
    }
    return copy;
}

static void mpu_enqueue_spell(EOSState *s, int spell_set, int out_spell, uint16_t * copied_spell)
{
    int next_tail = (s->mpu.sq_tail+1) & (COUNT(s->mpu.send_queue)-1);
    if (next_tail != s->mpu.sq_head)
    {
        MPU_DPRINTF("Queueing spell #%d.%d\n", spell_set+1, out_spell+1);
        s->mpu.send_queue[s->mpu.sq_tail] = copied_spell;
        s->mpu.sq_tail = next_tail;
    }
    else
    {
        MPU_EPRINTF("ERROR: send queue full\n");
    }
}

static void mpu_enqueue_spell_generic(EOSState *s, uint16_t * spell)
{
    int next_tail = (s->mpu.sq_tail+1) & (COUNT(s->mpu.send_queue)-1);
    if (next_tail != s->mpu.sq_head)
    {
        MPU_DPRINTF("Queueing spell: ");
        for (int i = 0; i < spell[0]; i++)
        {
            MPU_DPRINTF0("%02x ", spell[i]);
        }
        MPU_DPRINTF0("\n");
        s->mpu.send_queue[s->mpu.sq_tail] = copy_n_subst_spell(spell, 0, 0);
        s->mpu.sq_tail = next_tail;
    }
    else
    {
        MPU_EPRINTF("ERROR: send queue full\n");
    }
}

static void mpu_start_sending(EOSState *s)
{
    if (!s->mpu.sending && s->mpu.sq_head != s->mpu.sq_tail)
    {
        s->mpu.sending = 1;
        
        /* request a MREQ interrupt */
        eos_trigger_int(s, s->model->mpu_mreq_interrupt, 0);
    }
}

static int match_spell(uint16_t * received, uint16_t * in_spell)
{
    int n = in_spell[0];
    for (int i = 0; i < n; i++)
    {
        uint8_t in_lo = in_spell[i] & 0xFF;
        uint8_t in_hi = in_spell[i] >> 8;
        switch (in_hi)
        {
            case 0:
            {
                if (in_lo != received[i])
                {
                    return 0;
                }
                break;
            }

            case 1:
            {
                /* arguments - they match any value */
                break;
            }

            default:
            {
                assert(0);
            }
        }
    }

    /* no mismatch */
    return 1;
}


static int clean_shutdown = 0;

static void clean_shutdown_check(void)
{
    if (!clean_shutdown)
    {
        MPU_EPRINTF(
            KLRED"WARNING: forced shutdown."KRESET"\n\n"
            "For clean shutdown, please use 'Machine -> Power Down'\n"
            "(or 'system_powerdown' in QEMU monitor.)\n"
        );
    }
}

static void request_shutdown(void)
{
    MPU_EPRINTF("Shutdown requested.\n");
    clean_shutdown = 1;
    qemu_system_shutdown_request();
}

static void mpu_interpret_command(EOSState *s)
{
    MPU_EPRINTF("Received: ");
    int i;
    for (i = 0; i < s->mpu.recv_index; i++)
    {
        MPU_EPRINTF0("%02x ", s->mpu.recv_buffer[i]);
    }
    
    /* some spells may repeat; attempt to follow the sequence
     * by checking from where it left off at previous message */
    static int spell_set = 0;
    for (int k = 0; k < mpu_init_spell_count; k++, spell_set = (spell_set+1) % mpu_init_spell_count)
    {
        if (match_spell(s->mpu.recv_buffer+1, mpu_init_spells[spell_set].in_spell+1))
        {
            const char * desc = (mpu_init_spells[spell_set].description)
                ? mpu_init_spells[spell_set].description
                : mpu_spell_generic_description(mpu_init_spells[spell_set].in_spell);

            MPU_EPRINTF0(" (%s - spell #%d)\n", desc ? desc : KLRED"unnamed"KRESET, spell_set+1);
            
            int out_spell;
            for (out_spell = 0; mpu_init_spells[spell_set].out_spells[out_spell][0]; out_spell++)
            {
                /* copy and replace (substitute) any arguments */
                uint16_t * reply = copy_n_subst_spell(
                    mpu_init_spells[spell_set].out_spells[out_spell],
                    mpu_init_spells[spell_set].in_spell,
                    s->mpu.recv_buffer
                );
                mpu_enqueue_spell(s, spell_set, out_spell, reply);
            }
            mpu_start_sending(s);

            if (mpu_init_spells[spell_set].out_spells[out_spell][1] == MPU_SHUTDOWN)
            {
                request_shutdown();
            }

            /* next time, start matching from next spell */
            spell_set = (spell_set+1) % mpu_init_spell_count;
            return;
        }
    }

    const char * desc = mpu_spell_generic_description(s->mpu.recv_buffer);

    MPU_EPRINTF0(" ("KLRED"unknown - %s"KRESET")\n", desc ? desc : "unnamed");
}

void mpu_handle_sio3_interrupt(EOSState *s)
{
    if (s->mpu.sending)
    {
        int num_chars = s->mpu.out_spell[0];
        
        if (num_chars)
        {
            /* next two chars */
            s->mpu.out_char += 2;
            
            if (s->mpu.out_char < num_chars)
            {
                MPU_VPRINTF("Sending spell: chars %d & %d out of %d\n", 
                    s->mpu.out_char+1, s->mpu.out_char+2,
                    num_chars
                );
                
                if (s->mpu.out_char + 2 < num_chars)
                {
                    eos_trigger_int(s, s->model->mpu_sio3_interrupt, 0);   /* SIO3 */
                }
                else
                {
                    MPU_DPRINTF("spell finished\n");

                    if (s->mpu.sq_head != s->mpu.sq_tail)
                    {
                        MPU_DPRINTF("Requesting next spell\n");
                        eos_trigger_int(s, s->model->mpu_mreq_interrupt, 1);   /* MREQ */
                    }
                    else
                    {
                        /* no more spells */
                        MPU_DPRINTF("spells finished\n");
                        
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
            MPU_DPRINTF("Request more data\n");
            eos_trigger_int(s, s->model->mpu_sio3_interrupt, 0);   /* SIO3 */
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
            MPU_DPRINTF("receiving next message\n");
        }
        else
        {
            /* if a message is started in SIO3, it should continue with SIO3's, without triggering another MREQ */
            /* it appears to be harmless,  but I'm not sure what happens with more than 1 message queued */
            MPU_DPRINTF("next message was started in SIO3\n");
        }
        eos_trigger_int(s, s->model->mpu_sio3_interrupt, 0);   /* SIO3 */
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
     * - 1300D uses 0x83DC00 = reqest to send, 0x93D800 idle
     * - 1300D: status reg is 0xC022F484, tested for 0x40000 instead of 2
     * - 80D: request 0xD20B0884: 0xC0003 = request to send, 0x4D00B2 idle
     * - 80D: status 0xD20B0084, tested for 0x10000
     */
    
    int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;
    int receive_finished = 0;

    if(type & MODE_WRITE)
    {
        assert(address == s->model->mpu_request_register);

        int prev_value = s->mpu.status;
        s->mpu.status = value;
        
        if (value & s->model->mpu_request_bitmask)
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
        else if (prev_value & s->model->mpu_request_bitmask)
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
                eos_trigger_int(s, s->model->mpu_mreq_interrupt, 0);   /* MREQ */
                /* next steps in eos_handle_mreq -> mpu_handle_mreq_interrupt */
            }
        }
    }
    else
    {
        assert(address == s->model->mpu_status_register);

        if (s->mpu.sending == 2)
        {
            /* last two chars sent, finished */
            s->mpu.sending = 0;
        }

        /* actual return value doesn't seem to matter much
         * Canon code only tests a flag that appears to signal some sort of error
         * returning anything other than that flag has no effect
         * the following is just a guess that hopefully matches the D4/5 hardware
         */

        ret = (s->mpu.sending) ? 1 : 0;

        if (s->model->mpu_request_register == s->model->mpu_status_register)
        {
            ret |= s->mpu.status & ~1;
        }

        msg = "status (sending=%d, receiving=%d)";
        msg_arg1 = s->mpu.sending;
        msg_arg2 = s->mpu.receiving;
    }

    if (qemu_loglevel_mask(EOS_LOG_MPU))
    {
        io_log("MPU", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    }

    if (receive_finished)
    {
        mpu_interpret_command(s);
    }
    
    return ret;
}

static int mpu_handle_get_data(EOSState *s, int *hi, int *lo)
{
    if (s->mpu.out_spell &&
        s->mpu.out_char >= 0 &&
        s->mpu.out_char < s->mpu.out_spell[0])
    {
        *hi = s->mpu.out_spell[s->mpu.out_char];
        *lo = s->mpu.out_spell[s->mpu.out_char+1];
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
                    msg_arg2 = (intptr_t) "";
                    
                    if (s->mpu.recv_index == 0 && value == 0)
                    {
                        /* fixme: sometimes it gets out of sync
                         * ignoring null values at index 0 appears to fix it... */
                        msg_arg2 = (intptr_t) "(empty header!)";
                        //assert(0);
                    }
                    else if (s->mpu.recv_index + 2 < COUNT(s->mpu.recv_buffer))
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
        case 0x1D:  /* 40D uses 8-bit reads */
        
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
                        /* 40D uses 8-bit reads (fixme: cleaner way to handle this) */
                        ret = ((hi << 8) | lo) >> ((address & 1) ? 8 : 0);
                        msg = "Data from MPU";
                    } else {
                        msg = "From MPU -> out of range (char %d of %d)";
                        msg_arg1 = s->mpu.out_char;
                        msg_arg2 = s->mpu.out_spell[0];
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

    if (qemu_loglevel_mask(EOS_LOG_MPU))
    {
        io_log("SIO3", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    }
    return ret;
}

unsigned int eos_handle_mreq( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;
    
    if (address == s->model->mpu_control_register)
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

        if (qemu_loglevel_mask(EOS_LOG_MPU))
        {
            io_log("MREQ", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
        }
        return ret;
    }

    /* not handled here; unknown */
    io_log("???", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

#include "mpu_spells/button_codes.h"

static int* button_codes = 0;

/* http://www.marjorie.de/ps2/scancode-set1.htm */
/* to create a group of keys, simply name only the first key from the group */
static struct {
    int scancode;
    int gui_code;
    const char * pc_key_name;
    const char * cam_key_name;
} key_map[] = {
    { 0xE048,   BGMT_PRESS_UP,          "Arrow keys",   "Navigation",                   },
    { 0xE04B,   BGMT_PRESS_LEFT,                                                        },
    { 0xE050,   BGMT_PRESS_DOWN,                                                        },
    { 0xE04D,   BGMT_PRESS_RIGHT,                                                       },
    { 0xE0C8,   BGMT_UNPRESS_UP,                                                        },
    { 0xE0CB,   BGMT_UNPRESS_LEFT,                                                      },
    { 0xE0D0,   BGMT_UNPRESS_DOWN,                                                      },
    { 0xE0CD,   BGMT_UNPRESS_RIGHT,                                                     },
    
    { 0x004F,   BGMT_PRESS_DOWN_LEFT,   "Numpad keys",  "Joystick (8 directions)",      },
    { 0x0050,   BGMT_PRESS_DOWN,                                                        },
    { 0x0051,   BGMT_PRESS_DOWN_RIGHT,                                                  },
    { 0x004B,   BGMT_PRESS_LEFT,                                                        },
    { 0x004D,   BGMT_PRESS_RIGHT,                                                       },
    { 0x0047,   BGMT_PRESS_UP_LEFT,                                                     },
    { 0x0048,   BGMT_PRESS_UP,                                                          },
    { 0x0049,   BGMT_PRESS_UP_RIGHT,                                                    },
    { 0x00CF,   BGMT_UNPRESS_UDLR,                                                      },
    { 0x00D0,   BGMT_UNPRESS_UDLR,                                                      },
    { 0x00D1,   BGMT_UNPRESS_UDLR,                                                      },
    { 0x00CB,   BGMT_UNPRESS_UDLR,                                                      },
    { 0x00CD,   BGMT_UNPRESS_UDLR,                                                      },
    { 0x00C7,   BGMT_UNPRESS_UDLR,                                                      },
    { 0x00C8,   BGMT_UNPRESS_UDLR,                                                      },
    { 0x00C9,   BGMT_UNPRESS_UDLR,                                                      },
    { 0x004C,   BGMT_JOY_CENTER,        "Numpad 5",     "Joystick center",              },
    { 0x00CC,   BGMT_UNPRESS_UDLR,                                                      },
    
    { 0xE049,   BGMT_WHEEL_UP,          "PgUp, PgDn",   "Sub dial (rear scrollwheel)"   },
    { 0xE051,   BGMT_WHEEL_DOWN,                                                        },

    { 0x001A,   BGMT_WHEEL_LEFT,        "[ and ]",      "Main dial (top scrollwheel)",  },
    { 0x001B,   BGMT_WHEEL_RIGHT,                                                       },

    { 0x0039,   BGMT_PRESS_SET,         "SPACE",        "SET",                          },
    { 0x00B9,   BGMT_UNPRESS_SET,                                                       },

    { 0xE053,   BGMT_TRASH,             "DELETE",       "guess"                         },

    { 0x0032,   BGMT_MENU,              "M",            "MENU",                         },
    { 0x0019,   BGMT_PLAY,              "P",            "PLAY",                         },
    { 0x0017,   BGMT_INFO,              "I",            "INFO/DISP",                    },
    { 0x0097,   BGMT_UNPRESS_INFO,                                                      },
    { 0x0010,   BGMT_Q,                 "Q",            "guess",                        },
    { 0x0090,   BGMT_UNPRESS_Q,                                                         },
    { 0x0026,   BGMT_LV,                "L",            "LiveView",                     },
    { 0x0021,   BGMT_FUNC,              "F",            "FUNC",                         },
    { 0x0024,   BGMT_JUMP,              "J",            "JUMP",                         },
    { 0x0020,   BGMT_PRESS_DIRECT_PRINT,"D",            "Direct Print",                 },
    { 0x00A0,   BGMT_UNPRESS_DIRECT_PRINT,                                              },
    { 0x0011,   BGMT_PICSTYLE,          "W",            "Pic.Style",                    },
    { 0x001E,   BGMT_PRESS_AV,          "A",            "Av",                           },
    { 0x009E,   BGMT_UNPRESS_AV,                                                        },
    { 0x002C,   BGMT_PRESS_MAGNIFY_BUTTON,   "Z",       "Zoom in",                      },
    { 0x00AC,   BGMT_UNPRESS_MAGNIFY_BUTTON,                                            },
    { 0x002C,   BGMT_PRESS_ZOOM_IN,          "Z/X",     "Zoom in/out",                  },
    { 0x00AC,   BGMT_UNPRESS_ZOOM_IN,                                                   },
    { 0x002D,   BGMT_PRESS_ZOOM_OUT,                                                    },
    { 0x00AD,   BGMT_UNPRESS_ZOOM_OUT,                                                  },
    { 0x002A,   BGMT_PRESS_HALFSHUTTER, "Shift",        "Half-shutter"                  },
    { 0x0036,   BGMT_PRESS_HALFSHUTTER,                                                 },
    { 0x00AA,   BGMT_UNPRESS_HALFSHUTTER,                                               },
    { 0x00B6,   BGMT_UNPRESS_HALFSHUTTER,                                               },

    { 0x000B,   MPU_NEXT_SHOOTING_MODE, "0/9",          "Mode dial"                     },
    { 0x000A,   MPU_PREV_SHOOTING_MODE,                                                 },
    { 0x002F,   MPU_ENTER_MOVIE_MODE,   "V",            "Movie mode"                    },

    /* the following unpress events are just tricks for sending two events
     * with a small - apparently non-critical - delay between them */
    { 0x0030,   GMT_GUICMD_OPEN_BATT_COVER, "B",        "Open battery door",            },
    { 0x00B0,   MPU_SEND_ABORT_REQUEST,     /* sent shortly after opening batt. door */ },
    { 0x002E,   GMT_GUICMD_OPEN_SLOT_COVER, "C",        "Open card door",               },
    { 0x00AE,   MPU_SEND_SHUTDOWN_REQUEST,  /* sent shortly after opening card door */  },
    { 0x0044,   GMT_GUICMD_START_AS_CHECK,  "F10",      "Power down switch",            },
    { 0x00C4,   MPU_SEND_SHUTDOWN_REQUEST,  /* sent shortly after START_AS_CHECK */     },
};

/* returns MPU button codes (lo, hi) */
/* don't allow auto-repeat for most keys (exception: scrollwheels) */
static int translate_scancode_2(int scancode, int first_code, int allow_auto_repeat)
{
    if (!button_codes)
    {
        return -1;
    }
    
    int code = (first_code << 8) | scancode;

    if (code == 0x003B)
    {
        /* special: F1 -> help */
        return 0x00F1F1F1;
    }

    int ret = -2;                   /* not found */

    /* lookup MPU key code */
    for (int i = 0; i < COUNT(key_map); i++)
    {
        if (key_map[i].scancode == code)
        {
            switch (key_map[i].gui_code)
            {
                case MPU_EVENT_DISABLED:
                {
                    ret = 0;
                    break;
                }

                case BGMT_PRESS_HALFSHUTTER:
                case BGMT_UNPRESS_HALFSHUTTER:
                case BGMT_PRESS_FULLSHUTTER:
                case BGMT_UNPRESS_FULLSHUTTER:
                case MPU_SEND_SHUTDOWN_REQUEST:
                case MPU_SEND_ABORT_REQUEST:
                case MPU_NEXT_SHOOTING_MODE:
                case MPU_PREV_SHOOTING_MODE:
                case MPU_ENTER_MOVIE_MODE:
                {
                    /* special: return the raw gui code */
                    ret = 0x0E0E0000 | key_map[i].gui_code;
                    break;
                }

                case BGMT_PRESS_AV:
                case BGMT_UNPRESS_AV:
                {
                    /* special: return the raw gui code, with checking whether this button is supported */
                    if (button_codes[key_map[i].gui_code])
                    {
                        ret = 0x0E0E0000 | key_map[i].gui_code;
                    }
                    break;
                }

                case BGMT_WHEEL_UP:
                case BGMT_WHEEL_DOWN:
                case BGMT_WHEEL_LEFT:
                case BGMT_WHEEL_RIGHT:
                {
                    /* enable auto-repeat for these keys */
                    allow_auto_repeat = 1;
                    /* fall-through */
                }

                default:
                {
                    /* return model-specific button code (bindReceiveSwitch) */
                    /* don't allow auto-repeat */
                    ret = button_codes[key_map[i].gui_code];
                    break;
                }
            }

            if (ret > 0)
            {
                /* valid code found? stop here */
                break;
            }
        }
    }

    static int last_code = 0;
    if (code == last_code && !allow_auto_repeat)
    {
        /* don't auto-repeat */
        return -3;
    }
    last_code = code;

    return ret;
}

static int translate_scancode(int scancode)
{
    static int first_code = 0;
    
    if (first_code)
    {
        /* special keys (arrows etc) */
        int key = translate_scancode_2(scancode, first_code, 0);
        first_code = 0;
        return key;
    }
    
    if (scancode == 0xE0)
    {
        /* wait for second keycode */
        first_code = scancode;
        return -1;
    }
    
    /* regular keys */
    return translate_scancode_2(scancode, 0, 0);
}

static int key_avail(int scancode, int gui_code)
{
    /* check whether a given key is available on current camera model */
    /* disable autorepeat checking */
    int raw = translate_scancode_2(scancode & 0xFF, scancode >> 8, 1);
    if (raw <= 0) return 0;
    return ((raw & 0xFFFF0000) == 0x0E0E0000) || (raw == button_codes[gui_code]);
}

static void show_keyboard_help(void)
{
    MPU_EPRINTF0("\n");
    MPU_EPRINTF("Available keys:\n");

    int last_status = 0;
    
    for (int i = 0; i < COUNT(key_map); i++)
    {
        if (key_map[i].pc_key_name)
        {
            last_status = key_avail(key_map[i].scancode, key_map[i].gui_code);
            if (last_status)
            {
                int unpress_available = 0;
                for (int j = i+1; j < COUNT(key_map); j++)
                {
                    if ((key_map[i].scancode & 0x80) == 0 &&
                        (key_map[i].scancode | 0x80) == key_map[j].scancode &&
                        (key_avail(key_map[j].scancode, key_map[j].gui_code)))
                    {
                        unpress_available = 1;
                    }
                }
                const char * press_only = 
                    (unpress_available || strstr(key_map[i].cam_key_name, "wheel"))
                        ? "" : "(press only)";
                MPU_EPRINTF0("- %-12s : %s %s\n", key_map[i].pc_key_name, key_map[i].cam_key_name, press_only);
            }
        }
        else if (last_status)
        {
            /* for grouped keys, make sure all codes are available */
            if (key_map[i].gui_code == BGMT_UNPRESS_SET ||
                key_map[i].gui_code == BGMT_UNPRESS_INFO ||
                key_map[i].gui_code == BGMT_UNPRESS_Q)
            {
                /* exception: UNPRESS_SET on VxWorks models */
                /* 5D3 has UNPRESS_INFO - others? */
                /* only 100D sends UNPRESS_SET when releasing Q */
            }
            else if (!key_avail(key_map[i].scancode, key_map[i].gui_code))
            {
                MPU_EPRINTF("key code missing: %x %x\n", key_map[i].scancode, key_map[i].gui_code);
                exit(1);
            }
        }
    }
    
    MPU_EPRINTF0("- F1           : show this help\n");
    MPU_EPRINTF0("\n");
}

void mpu_send_keypress(EOSState *s, int keycode)
{
    /* good news: most MPU button codes appear to be the same across all cameras :) */
    int key = translate_scancode(keycode);
    if (key <= 0)
    {
        MPU_DPRINTF0("Key not recognized: %x\n", keycode);
        return;
    }
    
    if (key == 0x00F1F1F1)
    {
        show_keyboard_help();
        return;
    }

    if ((key & 0xFFFF0000) == 0x0E0E0000)
    {
        MPU_EPRINTF0("Key event: %x -> %08x\n", keycode, key);

        #define MPU_SEND_SPELLS(spells) \
            for (int i = 0; i < COUNT(spells); i++) { \
                mpu_enqueue_spell_generic(s, spells[i]); \
            } \
            mpu_start_sending(s); \

        switch (key & 0xFFFF)
        {
            case BGMT_PRESS_HALFSHUTTER:
            {
                /* fixme: if in some other GUI mode, switch back to 0 first */
                /* it will no longer be a simple sequence, but one with confirmation */
                uint16_t mpu_halfshutter_spells[][6] = {
                    { 0x06, 0x05, 0x06, 0x26, 0x01, 0x00 },
                    { 0x06, 0x04, 0x05, 0x00, 0x00, 0x00 },
                };
                MPU_SEND_SPELLS(mpu_halfshutter_spells);
                break;
            }

            case BGMT_UNPRESS_HALFSHUTTER:
            {
                uint16_t mpu_halfshutter_spells[][6] = {
                    { 0x06, 0x04, 0x05, 0x0B, 0x00, 0x00 },
                };
                MPU_SEND_SPELLS(mpu_halfshutter_spells);
                break;
            }

            case BGMT_PRESS_AV:
            {
                /* fixme: M mode only, changes shutter info, maybe other side effects */
                uint16_t mpu_av_spells[][0x12] = {
                    { 0x06, 0x05, 0x06, 0x1C, 0x01, 0x00 },
                    { 0x12, 0x11, 0x0a, 0x08, 0x06, 0x00, 0x01, 0x01, 0x98, 0x10, 0x00, 0x68, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00 },
                };
                MPU_SEND_SPELLS(mpu_av_spells);
                break;
            }

            case BGMT_UNPRESS_AV:
            {
                uint16_t mpu_av_spells[][0x12] = {
                    { 0x06, 0x05, 0x06, 0x1C, 0x00, 0x00 },
                    { 0x12, 0x11, 0x0a, 0x08, 0x06, 0x00, 0x01, 0x03, 0x98, 0x10, 0x00, 0x68, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 },
                };
                MPU_SEND_SPELLS(mpu_av_spells);
                break;
            }

            case MPU_SEND_SHUTDOWN_REQUEST:
            {
                uint16_t shutdown_request[][6] = {
                    { 0x06, 0x05, 0x02, 0x0b, 0x00, 0x00 },
                };
                MPU_SEND_SPELLS(shutdown_request);
                break;
            }

            case MPU_SEND_ABORT_REQUEST:
            {
                uint16_t abort_request[][6] = {
                    { 0x06, 0x04, 0x02, 0x0c, 0x00, 0x00 },
                };
                MPU_SEND_SPELLS(abort_request);
                break;
            }

            case MPU_NEXT_SHOOTING_MODE:
            case MPU_PREV_SHOOTING_MODE:
            {
                int delta = (key & 0xFFFF) == MPU_NEXT_SHOOTING_MODE ? 1 : -1;
                /* this request covers many other properties, some model-specific, some require storing state variables */
                /* look up the mode change request for this model and patch it */

                for (int k = 0; k < mpu_init_spell_count && mpu_init_spells[0].out_spells[k][0]; k++)
                {
                    uint16_t * spell = mpu_init_spells[0].out_spells[k];

                    if (spell[2] == 0x02 && (spell[3] == 0x00 || spell[3] == 0x0e))
                    {
                        int old_mode = spell[4];

                        /* any valid mode beyond 31? */
                        int new_mode = (old_mode + delta) & 0x1F;

                        MPU_EPRINTF("using reply #1.%d for mode switch (%d -> %d).\n", k+1, old_mode, new_mode);

                        /* not 100% sure it's right */
                        spell[4] = spell[5] = new_mode;
                        mpu_enqueue_spell_generic(s, spell);
                        mpu_start_sending(s);

                        break;
                    }
                }
                break;
            }

            case MPU_ENTER_MOVIE_MODE:
            {
                if (s->model->dedicated_movie_mode)
                {
                    /* fixme: duplicate code */
                    for (int k = 0; k < mpu_init_spell_count && mpu_init_spells[0].out_spells[k][0]; k++)
                    {
                        uint16_t * spell = mpu_init_spells[0].out_spells[k];

                        if (spell[2] == 0x02 && (spell[3] == 0x00 || spell[3] == 0x0e))
                        {
                            /* toggle between M and Movie */
                            int old_mode = spell[4];
                            int new_mode = (old_mode == 0x14) ? 0x3 : 0x14;

                            MPU_EPRINTF("using reply #1.%d for mode switch (%d -> %d).\n", k+1, old_mode, new_mode);
                            spell[4] = spell[5] = new_mode;
                            mpu_enqueue_spell_generic(s, spell);
                            mpu_start_sending(s);

                            break;
                        }
                    }
                }
                else
                {
                    static int mv = 0;  /* fixme: get current state from properties */
                    if (mv)
                    {
                        /* exit movie mode (back to photo mode) */
                        uint16_t movie_mode_request[][8] = {
                            { 0x06, 0x05, 0x03, 0x37, 0x01, 0x00 },         /* PROP_MIRROR_DOWN_IN_MOVIE_MODE */
                            { 0x06, 0x05, 0x01, 0x48, 0x01, 0x00 },         /* PROP_LIVE_VIEW_MOVIE_SELECT */
                            { 0x06, 0x05, 0x01, 0x4f, 0x00, 0x00 },         /* PROP_FIXED_MOVIE */
                            { 0x06, 0x05, 0x01, 0x4b, 0x01, 0x00 },         /* PROP_LIVE_VIEW_VIEWTYPE_SELECT */
                            { 0x06, 0x05, 0x03, 0x37, 0x00, 0x00 },         /* PROP_MIRROR_DOWN_IN_MOVIE_MODE */
                          //{ 0x08, 0x06, 0x04, 0x0c, 0x03, 0x00, 0x01 }    /* PROP_SHOOTING_TYPE */
                        };
                        MPU_SEND_SPELLS(movie_mode_request);
                    }
                    else
                    {
                        /* enter movie mode */
                        uint16_t movie_mode_request[][8] = {
                            { 0x06, 0x05, 0x03, 0x37, 0x01, 0x00 },         /* PROP_MIRROR_DOWN_IN_MOVIE_MODE */
                            { 0x06, 0x05, 0x01, 0x48, 0x02, 0x00 },         /* PROP_LIVE_VIEW_MOVIE_SELECT */
                            { 0x06, 0x05, 0x01, 0x4f, 0x01, 0x00 },         /* PROP_FIXED_MOVIE */
                            { 0x06, 0x05, 0x01, 0x4b, 0x02, 0x00 },         /* PROP_LIVE_VIEW_VIEWTYPE_SELECT */
                            { 0x06, 0x05, 0x03, 0x37, 0x00, 0x00 },         /* PROP_MIRROR_DOWN_IN_MOVIE_MODE */
                          //{ 0x08, 0x06, 0x04, 0x0c, 0x03, 0x00, 0x01 }    /* PROP_SHOOTING_TYPE */
                        };
                        MPU_SEND_SPELLS(movie_mode_request);
                    }
                    mv = !mv;
                }
                break;
            }

            default:
            {
                assert(0);
            }
        }
        return;
    }

    MPU_EPRINTF0("Key event: %x -> %04x\n", keycode, key);

    uint16_t mpu_keypress_spell[6] = {
        0x06, 0x05, 0x06, key >> 8, key & 0xFF, 0x00
    };
    
    /* todo: check whether a race condition is still possible */
    /* (is this function called from the same thread as I/O handlers or not?) */
    mpu_enqueue_spell_generic(s, mpu_keypress_spell);
    mpu_start_sending(s);
}

static void mpu_send_powerdown(Notifier * notifier, void * null)
{
    EOSState *s = (EOSState *)((void *)notifier
        - offsetof(MPUState, powerdown_notifier)
        - offsetof(EOSState, mpu));

    /* same as F10 */
    mpu_send_keypress(s, 0x0044);
    mpu_send_keypress(s, 0x00C4);
}

static void mpu_check_duplicate_spells(EOSState *s)
{
    for (int i = 0; i < mpu_init_spell_count; i++)
    {
        if (mpu_init_spells[i].out_spells[0][0])
        {
            /* non-empty spell */
            /* let's check for duplicates, as they may indicate some spells that depend on camera state */
            /* in current implementation, they may produce undefined behavior (depending on which spell is matched first) */
            int found = 0;
            int already_handled = 0;

            for (int j = 0; j < i; j++)
            {
                if (mpu_init_spells[j].out_spells[0][0] &&
                    (match_spell(mpu_init_spells[i].in_spell, mpu_init_spells[j].in_spell) ||
                     match_spell(mpu_init_spells[j].in_spell, mpu_init_spells[i].in_spell)))
                {
                    already_handled = 1;
                    break;
                }
            }
            if (already_handled)
            {
                continue;
            }
            for (int j = 0; j < mpu_init_spell_count; j++)
            {
                if (i != j &&
                    (match_spell(mpu_init_spells[i].in_spell, mpu_init_spells[j].in_spell) ||
                     match_spell(mpu_init_spells[j].in_spell, mpu_init_spells[i].in_spell)))
                {
                    if (!found)
                    {
                        MPU_EPRINTF("warning: non-empty spell #%d", i+1);
                        if (mpu_init_spells[i].description) {
                            MPU_EPRINTF0(" (%s)", mpu_init_spells[i].description);
                        } else {
                            MPU_EPRINTF0(" (%02x %02x %02x %02x)",
                                mpu_init_spells[i].in_spell[0], mpu_init_spells[i].in_spell[1],
                                mpu_init_spells[i].in_spell[2], mpu_init_spells[i].in_spell[3]
                            );
                        }
                        MPU_EPRINTF0(" has duplicate(s): ");
                        found = 1;
                    }
                    MPU_EPRINTF0("#%d ", j+1);
                }
            }
            if (found) MPU_EPRINTF0("\n");
        }
    }
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

    MPU_SPELL_SET(5D2)
    MPU_SPELL_SET(5D3)
    MPU_SPELL_SET(6D)
    MPU_SPELL_SET(50D)
    MPU_SPELL_SET(60D)
    MPU_SPELL_SET(70D)
    MPU_SPELL_SET(450D)
    MPU_SPELL_SET(500D)
    MPU_SPELL_SET(550D)
    MPU_SPELL_SET(600D)
    MPU_SPELL_SET(700D)
    MPU_SPELL_SET(100D)
    MPU_SPELL_SET(EOSM)
    MPU_SPELL_SET(EOSM2)

    /* 1200D works with 60D MPU spells... and BOOTS THE GUI!!! */
    /* same for 1100D */
    MPU_SPELL_SET_OTHER_CAM(1000D, 450D)
    MPU_SPELL_SET_OTHER_CAM(1100D, 60D)
    MPU_SPELL_SET_OTHER_CAM(1200D, 60D)
    MPU_SPELL_SET_OTHER_CAM(1300D, 600D)
    MPU_SPELL_SET_OTHER_CAM(40D, 50D)

    MPU_SPELL_SET_OTHER_CAM(650D, 700D)

    if (!mpu_init_spell_count)
    {
        MPU_EPRINTF("FIXME: using generic MPU spells for %s.\n", s->model->name);
        mpu_init_spells = mpu_init_spells_generic;
        mpu_init_spell_count = COUNT(mpu_init_spells_generic);
        /* how to get them: http://magiclantern.fm/forum/index.php?topic=2864.msg166938#msg166938 */
    }

    if (0)
    {
        MPU_EPRINTF("WARNING: using bruteforce MPU spells for %s.\n", s->model->name);
        mpu_init_spells = mpu_init_spells_bruteforce;
        mpu_init_spell_count = COUNT(mpu_init_spells_bruteforce);
    }

    mpu_check_duplicate_spells(s);

#define MPU_BUTTON_CODES(cam) \
    if (strcmp(s->model->name, #cam) == 0) { \
        button_codes = button_codes_##cam; \
    }

#define MPU_BUTTON_CODES_OTHER_CAM(cam1,cam2) \
    if (strcmp(s->model->name, #cam1) == 0) { \
        button_codes = button_codes_##cam2; \
    }

    MPU_BUTTON_CODES(100D)
    MPU_BUTTON_CODES(1100D)
    MPU_BUTTON_CODES(1200D)
    MPU_BUTTON_CODES_OTHER_CAM(1300D, 1200D)
    MPU_BUTTON_CODES(450D)
    MPU_BUTTON_CODES_OTHER_CAM(1000D, 450D)
    MPU_BUTTON_CODES(40D)
    MPU_BUTTON_CODES(500D)
    MPU_BUTTON_CODES(550D)
    MPU_BUTTON_CODES(50D)
    MPU_BUTTON_CODES(5D2)
    MPU_BUTTON_CODES(5D3)
    MPU_BUTTON_CODES(600D)
    MPU_BUTTON_CODES(60D)
    MPU_BUTTON_CODES(6D)
    MPU_BUTTON_CODES(650D)
    MPU_BUTTON_CODES(700D)
    MPU_BUTTON_CODES(70D)
    MPU_BUTTON_CODES(7D)
    MPU_BUTTON_CODES(EOSM)
    MPU_BUTTON_CODES(EOSM2)

    if (!button_codes)
    {
        MPU_EPRINTF("FIXME: no MPU button codes for %s.\n", s->model->name);
        /* run qemu-2.x.x/hw/eos/mpu_spells/make_button_codes.sh to get them */
        return;
    }
    
    if (button_codes[BGMT_UNPRESS_UDLR])
    {
        assert(button_codes[BGMT_UNPRESS_UP]    == 0);
        assert(button_codes[BGMT_UNPRESS_DOWN]  == 0);
        assert(button_codes[BGMT_UNPRESS_LEFT]  == 0);
        assert(button_codes[BGMT_UNPRESS_RIGHT] == 0);
        button_codes[BGMT_UNPRESS_UP]    = 
        button_codes[BGMT_UNPRESS_DOWN]  = 
        button_codes[BGMT_UNPRESS_LEFT]  = 
        button_codes[BGMT_UNPRESS_RIGHT] = button_codes[BGMT_UNPRESS_UDLR];
    }

    for (int i = 0; i < COUNT(key_map); i++)
    {
        if (key_map[i].gui_code == MPU_ENTER_MOVIE_MODE && s->model->dedicated_movie_mode == -1)
        {
            /* no movie mode on this model */
            key_map[i].gui_code = MPU_EVENT_DISABLED;
        }
    }

    show_keyboard_help();

    s->mpu.powerdown_notifier.notify = mpu_send_powerdown;
    qemu_register_powerdown_notifier(&s->mpu.powerdown_notifier);

    atexit(clean_shutdown_check);
}
