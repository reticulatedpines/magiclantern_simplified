/*
"ADTG" is a special Analog Devices part that we assume
is a Timing Generator (and maybe some other stuff). See:
https://magiclantern.fandom.com/wiki/ADTG
Modifying timing can do things such as changing the
sensor sample area (a longer time while reading cols
leads to more being read, etc).

"CMOS" is somehow related, a string found in ADTG contexts
in ROMs.  Possibly the naming is preserved from when the cam
sensors were CMOS.  Certainly some parts of "CMOS" in the
forum discussions of ADTG relate to modifying handling of the sensor,
for example, controlling ISO when reading the sensor.

The ADTG part has its own internal control registers.  These
are the primary things we're interested in logging.  The Digic
layer sends messages to the ADTG part to control it.  We parse
and log these.
*/

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>

#include "adtglog2.h"
#include "log.h"
#include "patch_mmu.h"
#include "hooks_thumb.h"
#include "hooks_arm.h"

static uint32_t buf_item_size = 0;

// D45 cams need to resolve calls that they never make
// to MMU functions on D78 cams, and vice versa.
// This really isn't a good system.
extern WEAK_FUNC(ret_1) int patch_hook_function(uintptr_t addr,
                                                uint32_t orig_instr,
                                                patch_hook_function_cbr hook_function,
                                                const char *description);
extern WEAK_FUNC(ret_1) int convert_f_patch_to_patch(struct function_hook_patch *f_patch_in,
                                                     struct patch *patch_out,
                                                     uint8_t *hook_mem_out);


static void log_cmos_command_buffer_16(uint16_t *cmos_buf, uint32_t lr)
{
    uint32_t log_buf_size = 80;
    char log_buf[log_buf_size];

    size_t cmos_i = 0;
    size_t log_buf_i = 0;
    while (cmos_i < MAX_CMOS_BUF)
    {
        // TODO add NRZI decoding when appropriate
        int err;

        if (log_buf_i > log_buf_size - 28)
        { // when space in stack buffer gets low,
          // flush and reset
            send_log_data_str(log_buf);
            log_buf_i = 0;
        }
        err = snprintf(log_buf + log_buf_i,
                       log_buf_size - log_buf_i,
                       "%04x ", cmos_buf[cmos_i]);
        if (err < 0)
            break;
        log_buf_i += err;

        if ((cmos_i % 8) == 7)
        {
            err = snprintf(log_buf + log_buf_i,
                           log_buf_size - log_buf_i,
                           "\n\t      ");
            if (err < 0)
                break;
            log_buf_i += err;
        }

        if (cmos_buf[cmos_i] == (uint16_t)CMOS_END_MARKER)
            break;

        cmos_i++;
    }
    snprintf(log_buf + log_buf_i, log_buf_size - log_buf_i, "\n");
    send_log_data_str(log_buf);

    if (cmos_i >= MAX_CMOS_BUF)
    {
        snprintf(log_buf, log_buf_size, "WARNING, end of CMOS command buf not found\n");
        send_log_data_str(log_buf);
    }
}

static void log_cmos_command_buffer_32(uint32_t *cmos_buf, uint32_t lr)
{
    uint32_t log_buf_size = 80;
    char log_buf[log_buf_size];

    size_t cmos_i = 0;
    size_t log_buf_i = 0;
    while (cmos_i < MAX_CMOS_BUF)
    {
        // TODO add NZRI decoding when appropriate
        int err;

        if (log_buf_i > log_buf_size - 28)
        { // when space in stack buffer gets low,
          // flush and reset
            send_log_data_str(log_buf);
            log_buf_i = 0;
        }
        err = snprintf(log_buf + log_buf_i,
                       log_buf_size - log_buf_i,
                       "%08x ", cmos_buf[cmos_i]);
        if (err < 0)
            break;
        log_buf_i += err;

        // FIXME horrible hack, quickly test modifying
        // command buffer for 6D2 inside the logger, yuck
        //
        // Tested and works.  Now to find out where
        // these values come from.
//        if (cmos_buf[cmos_i] == 0x0d03a110)
//        {
//            cmos_buf[cmos_i] =  0x0d03a140; // hopefully ISO 200/1600
//        }

        if ((cmos_i % 8) == 7)
        {
            err = snprintf(log_buf + log_buf_i,
                           log_buf_size - log_buf_i,
                           "\n\t      ");
            if (err < 0)
                break;
            log_buf_i += err;
        }

        if (cmos_buf[cmos_i] == CMOS_END_MARKER)
            break;

        cmos_i++;
    }
    snprintf(log_buf + log_buf_i, log_buf_size - log_buf_i, "\n");
    send_log_data_str(log_buf);

    if (cmos_i >= MAX_CMOS_BUF)
    {
        snprintf(log_buf, log_buf_size, "WARNING, end of CMOS command buf not found\n");
        send_log_data_str(log_buf);
    }
}

void log_cmos_command_buffer(void *cmos_buf, uint32_t lr)
{
    uint32_t log_buf_size = 100;
    char log_buf[log_buf_size];

    snprintf(log_buf, log_buf_size, "CMOS_write, time: %d,  LR: %x, buf_addr: %x\n\tdata: ",
             get_ms_clock(), lr, cmos_buf);
    send_log_data_str(log_buf);

    if (buf_item_size == 16)
    {
        log_cmos_command_buffer_16((uint16_t *)cmos_buf, lr);
    }
    else if (buf_item_size == 32)
    {
        log_cmos_command_buffer_32((uint32_t *)cmos_buf, lr);
    }
    else
    {
        snprintf(log_buf, log_buf_size, "ERROR, unexpected buf_item_size: %d\n", buf_item_size);
        send_log_data_str(log_buf);
    }
}

static unsigned int init()
{
    static uint8_t *log_buf = NULL;
    log_buf = malloc(MIN_LOG_BUF_SIZE);
    if (log_buf == NULL)
        return -1;

    if (init_log(log_buf, MIN_LOG_BUF_SIZE, "adtg.log") < 0)
        return -2;

    if (is_camera("200D", "1.0.1"))
    {
        buf_item_size = 32;

        // install hooks
        struct function_hook_patch f_patches[] = {
            {
                .patch_addr = 0xe034256e, // CMOS_write
                .orig_content = {0x2d, 0xe9, 0xf8, 0x4f, 0x04, 0x46, 0x86, 0x4d},
                .target_function_addr = (uint32_t)hook_CMOS_write_200D,
                .description = "Log ADTG CMOS writes"
            },
        };

        struct patch patches[COUNT(f_patches)] = {};
        uint8_t code_hooks[8 * COUNT(f_patches)] = {};

        for (int i = 0; i < COUNT(f_patches); i++)
        {
            if (convert_f_patch_to_patch(&f_patches[i],
                                         &patches[i],
                                         &code_hooks[8 * i]))
            {
                return 1;
            }
        }
        apply_patches(patches, COUNT(f_patches));
    }
    else if (is_camera("6D2", "1.1.1"))
    {
        buf_item_size = 32;

        // install hooks
        struct function_hook_patch f_patches[] = {
            {
                .patch_addr = 0xe053cdc4, // CMOS_write
                .orig_content = {0x2d, 0xe9, 0xfc, 0x5f, 0x04, 0x46, 0x94, 0x4f},
                .target_function_addr = (uint32_t)hook_CMOS_write_6D2,
                .description = "Log ADTG CMOS writes"
            },
        };

        struct patch patches[COUNT(f_patches)] = {};
        uint8_t code_hooks[8 * COUNT(f_patches)] = {};

        for (int i = 0; i < COUNT(f_patches); i++)
        {
            if (convert_f_patch_to_patch(&f_patches[i],
                                         &patches[i],
                                         &code_hooks[8 * i]))
            {
                return 1;
            }
        }
        apply_patches(patches, COUNT(f_patches));
    }
    else if (is_camera("70D", "1.1.2"))
    {
        buf_item_size = 16;

        patch_hook_function(0x26b54, // CMOS_write
                            0xe92d41f0, // orig_instr
                            hook_CMOS_write_70D,
                            "Log ADTG CMOS writes");
    }

    return 0;
}

static unsigned int deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(init)
    MODULE_DEINIT(deinit)
MODULE_INFO_END()
