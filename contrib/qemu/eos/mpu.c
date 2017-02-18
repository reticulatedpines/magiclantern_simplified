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
#include "mpu_spells/500D.h"
#include "mpu_spells/550D.h"
#include "mpu_spells/600D.h"
#include "mpu_spells/700D.h"
#include "mpu_spells/EOSM.h"
#include "mpu_spells/100D.h"
#include "mpu_spells/450D.h"

static void mpu_send_next_spell(EOSState *s)
{
    if (s->mpu.sq_head != s->mpu.sq_tail)
    {
        /* get next spell from the queue */
        s->mpu.out_spell = s->mpu.send_queue[s->mpu.sq_head];
        s->mpu.sq_head = (s->mpu.sq_head+1) & (COUNT(s->mpu.send_queue)-1);
        printf("[MPU] Sending spell: ");

        for (int i = 0; i < s->mpu.out_spell[0]; i++)
        {
            printf("%02x ", s->mpu.out_spell[i]);
        }
        printf("\n");

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
        s->mpu.send_queue[s->mpu.sq_tail] = mpu_init_spells[spell_set].out_spells[out_spell];
        s->mpu.sq_tail = next_tail;
    }
    else
    {
        printf("[MPU] ERROR: send queue full\n");
    }
}

static void mpu_enqueue_spell_generic(EOSState *s, unsigned char * spell)
{
    int next_tail = (s->mpu.sq_tail+1) & (COUNT(s->mpu.send_queue)-1);
    if (next_tail != s->mpu.sq_head)
    {
        printf("[MPU] Queueing spell: ");
        for (int i = 0; i < spell[0]; i++)
        {
            printf("%02x ", spell[i]);
        }
        printf("\n");
        s->mpu.send_queue[s->mpu.sq_tail] = spell;
        s->mpu.sq_tail = next_tail;
    }
    else
    {
        printf("[MPU] ERROR: send queue full\n");
    }
}

static void mpu_start_sending(EOSState *s)
{
    if (!s->mpu.sending && s->mpu.sq_head != s->mpu.sq_tail)
    {
        s->mpu.sending = 1;
        
        /* request a MREQ interrupt */
        eos_trigger_int(s, s->model->mpu_request_interrupt, 0);
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
    
    /* some spells may repeat; attempt to follow the sequence
     * by checking from where it left off at previous message */
    static int spell_set = 0;
    for (int k = 0; k < mpu_init_spell_count; k++, spell_set = (spell_set+1) % mpu_init_spell_count)
    {
        if (memcmp(s->mpu.recv_buffer+1, mpu_init_spells[spell_set].in_spell+1, mpu_init_spells[spell_set].in_spell[1]) == 0)
        {
            printf(" (recognized spell #%d)\n", spell_set+1);
            
            int out_spell;
            for (out_spell = 0; mpu_init_spells[spell_set].out_spells[out_spell][0]; out_spell++)
            {
                mpu_enqueue_spell(s, spell_set, out_spell);
            }
            mpu_start_sending(s);
            return;
        }
    }
    
    printf(" (unknown spell)\n");
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
                    printf("[MPU] spell finished\n");

                    if (s->mpu.sq_head != s->mpu.sq_tail)
                    {
                        printf("[MPU] Requesting next spell\n");
                        eos_trigger_int(s, s->model->mpu_request_interrupt, 1);   /* MREQ */
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
                eos_trigger_int(s, s->model->mpu_request_interrupt, 0);   /* MREQ */
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

    io_log("SIO3", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
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
    }
    
    io_log("MREQ", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
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
    { 0xE0D0,   BGMT_UNPRESS_LEFT,                                                      },
    { 0xE0CB,   BGMT_UNPRESS_DOWN,                                                      },
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
    { 0x0019,   BGMT_MENU,              "P",            "PLAY",                         },
    { 0x0017,   BGMT_MENU,              "I",            "INFO/DISP",                    },
    { 0x0010,   BGMT_Q,                 "Q",            "guess",                        },
    { 0x0026,   BGMT_LV,                "L",            "LiveView",                     },
    { 0x0011,   BGMT_PICSTYLE,          "W",            "Pic.Style",                    },
};

/* returns MPU button codes (lo, hi) */
static int translate_scancode_2(int scancode, int first_code)
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

    /* lookup MPU key code */
    for (int i = 0; i < COUNT(key_map); i++)
    {
        if (key_map[i].scancode == code)
        {
            return button_codes[key_map[i].gui_code];
        }
    }
    
    /* not found */
    return -2;
}

static int translate_scancode(int scancode)
{
    static int first_code = 0;
    
    if (first_code)
    {
        /* special keys (arrows etc) */
        int key = translate_scancode_2(scancode, first_code);
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
    return translate_scancode_2(scancode, 0);
}

static int key_avail(int scancode)
{
    /* check whether a given key is available on current camera model */
    return translate_scancode_2(scancode & 0xFF, scancode >> 8) > 0;
}

static void show_keyboard_help(void)
{
    puts("");
    puts("Available keys:");

    int last_status = 0;
    
    for (int i = 0; i < COUNT(key_map); i++)
    {
        if (key_map[i].pc_key_name)
        {
            last_status = key_avail(key_map[i].scancode);
            if (last_status)
            {
                printf("- %-12s : %s\n", key_map[i].pc_key_name, key_map[i].cam_key_name);
            }
        }
        else if (last_status && !key_avail(key_map[i].scancode))
        {
            /* for grouped keys, make sure all codes are available */
            printf("key code missing: %x %x\n", key_map[i].scancode, key_map[i].gui_code);
            exit(1);
        }
    }
    
    puts("- F1           : show this help");
    puts("");
}

void mpu_send_keypress(EOSState *s, int keycode)
{
    /* good news: most MPU button codes appear to be the same across all cameras :) */
    int key = translate_scancode(keycode);
    if (key <= 0) return;
    
    if (key == 0x00F1F1F1)
    {
        show_keyboard_help();
        return;
    }

    printf("Key event: %x -> %04x\n", keycode, key);
    
    static unsigned char mpu_keypress_spell[6] = {
        0x06, 0x05, 0x06, 0x00, 0x00, 0x00
    };
    
    mpu_keypress_spell[3] = key >> 8;
    mpu_keypress_spell[4] = key & 0xFF;
    
    /* fixme: race condition */
    mpu_enqueue_spell_generic(s, mpu_keypress_spell);
    mpu_start_sending(s);
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
    MPU_SPELL_SET(700D)
    MPU_SPELL_SET(EOSM)
    MPU_SPELL_SET(100D)
    MPU_SPELL_SET(450D)
    MPU_SPELL_SET(500D)

    /* 1200D works with 60D MPU spells... and BOOTS THE GUI!!! */
    MPU_SPELL_SET_OTHER_CAM(1200D, 60D)

    /* same for 1100D */
    MPU_SPELL_SET_OTHER_CAM(1100D, 60D)

    if (!mpu_init_spell_count)
    {
        printf("FIXME: no MPU spells for %s.\n", s->model->name);
        /* how to get them: http://magiclantern.fm/forum/index.php?topic=2864.msg166938#msg166938 */
    }

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
    MPU_BUTTON_CODES_OTHER_CAM(1200D, 600D)
    MPU_BUTTON_CODES(450D)
    MPU_BUTTON_CODES(500D)
    MPU_BUTTON_CODES(550D)
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

    if (!button_codes)
    {
        printf("FIXME: no MPU button codes for %s.\n", s->model->name);
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
    
    show_keyboard_help();
}
