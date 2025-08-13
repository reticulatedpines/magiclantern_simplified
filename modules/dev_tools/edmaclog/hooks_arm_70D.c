#include "hooks.h"
#include "dryos.h"
#include "log.h"

// EDMAC logging, around device IDs.  Trying to work out
// how / if these correlate with channels, and what different
// device IDs might mean.

// For this D5 cam, we'll use the existing patch_hook_function(),
// which decides the func sig for the hooks.
// From patch.h:
//  - regs contain R0-R12 and LR
// Use a struct so we can name things?  Nah, too easy.


void hook_CreateResLockEntry_70D(uint32_t *regs, uint32_t *stack, uint32_t pc)
{
    uint32_t log_buf_size = 80;
    char log_buf[log_buf_size];
    snprintf(log_buf, log_buf_size, "CreateResLockEntry, LR: %x, time: %d\n",
             regs[13], get_ms_clock());
    send_log_data_str(log_buf);

    uint32_t *pResources = (uint32_t *)regs[0];
    uint32_t res_count = regs[1];

    for (uint32_t i = 0; i < res_count; i++)
    {
        snprintf(log_buf, log_buf_size, "    0x%x\n", *(pResources + i));
        send_log_data_str(log_buf);
    }
}

void hook_StartEDMAC_70D(uint32_t *regs, uint32_t *stack, uint32_t pc)
{
    uint32_t log_buf_size = 80;
    char log_buf[log_buf_size];
    snprintf(log_buf, log_buf_size, "StartEDMAC, LR: %x, chan: %x, time: %d\n",
             regs[13], regs[0], get_ms_clock());
    send_log_data_str(log_buf);
}

void hook_ConnectReadEDMAC_70D(uint32_t *regs, uint32_t *stack, uint32_t pc)
{
    uint32_t log_buf_size = 80;
    char log_buf[log_buf_size];
    snprintf(log_buf, log_buf_size, "ConnectReadEDMAC, LR: %x, chan: %x, dev: %x, time: %d\n",
             regs[13], regs[0], regs[1], get_ms_clock());
    send_log_data_str(log_buf);
}

void hook_ConnectWriteEDMAC_70D(uint32_t *regs, uint32_t *stack, uint32_t pc)
{
    uint32_t log_buf_size = 80;
    char log_buf[log_buf_size];
    snprintf(log_buf, log_buf_size, "ConnectWriteEDMAC, LR: %x, chan: %x, dev: %x, time: %d\n",
             regs[13], regs[0], regs[1], get_ms_clock());
    send_log_data_str(log_buf);
}

// D45 cams have one func, SetEDMAC, that configures both address
// and region dimensions.
// D78 have a separate SetAddr() and SetSize().
// (haven't checked 6 or X, probably the same).
// Here we log with strings that look the same as D78, in order
// to make comparison of logs simpler.
void hook_SetEDMAC_70D(uint32_t *regs, uint32_t *stack, uint32_t pc)
{
    // void SetEDmac(uint channel, void *address, edmac_info *edmac_info, int flags)
    uint32_t log_buf_size = 120;
    char log_buf[log_buf_size];
    uint32_t time = get_ms_clock();

    snprintf(log_buf, log_buf_size, "SetSize, LR: %x, chan: %x, time: %d\n",
             regs[13], regs[0], time);
    send_log_data_str(log_buf);
    struct edmac_info_D4567 *edmac_info = (struct edmac_info_D4567 *)regs[2];
    if (edmac_info != NULL)
    {
        snprintf(log_buf, log_buf_size, "SetSize, xa: %d, ya: %d, xb: %d, yb: %d, xn: %d, yn: %d\n",
                 edmac_info->xa, edmac_info->ya,
                 edmac_info->xb, edmac_info->yb,
                 edmac_info->xn, edmac_info->yn);
    }
    else
    {
        snprintf(log_buf, log_buf_size, "SetSize, edmac_info == NULL\n");
    }
    send_log_data_str(log_buf);

    snprintf(log_buf, log_buf_size, "SetAddr, LR: %x, chan: %x, addr: %x, time: %d\n",
             regs[13], regs[0], regs[1], time);
    send_log_data_str(log_buf);
}

