/* SPDX-License-Identifier: GPL-2.0-only
 * Experimental, read-only USB LiveView for the original EOS M (firmware 2.0.2).
 * Opcode 0x9997, subcommand 0 = protocol version, 1 = one UYVY422 frame.
 * Unlike PTP_CHDK, this cannot read or write arbitrary camera memory.
 * Keep disabled by default: hardware behaviour is only verified on EOS M.
 */
#include "dryos.h"
#include "ptp.h"
#include "shoot.h"
#include "propvalues.h"
#include "vram.h"

#ifndef CONFIG_EOSM
#error CONFIG_PTP_PREVIEW is currently supported only on EOS M
#endif

#define PTP_EOSM_PREVIEW 0x9997
#define FRAME_HEADER_SIZE 16
#define MAX_FRAME_BYTES (1024 * 600 * 2)

static void write_u16(uint8_t *p, uint16_t n)
{
    p[0] = n;
    p[1] = n >> 8;
}

static void write_u32(uint8_t *p, uint32_t n)
{
    p[0] = n;
    p[1] = n >> 8;
    p[2] = n >> 16;
    p[3] = n >> 24;
}

PTP_HANDLER(PTP_EOSM_PREVIEW, 0)
{
    struct ptp_msg msg = {
        .id = PTP_RC_OK,
        .session = session,
        .transaction = transaction,
        .param_count = 0,
    };

    if (param1 == 0)
    {
        msg.param_count = 2;
        msg.param[0] = 1; /* protocol version */
        msg.param[1] = 1; /* signed-chroma UYVY422 */
    }
    else if (param1 == 1)
    {
        if (!lv)
        {
            msg.id = PTP_RC_GeneralError;
        }
        else
        {
            struct vram_info *frame = get_yuv422_vram();
            if (!frame || !frame->vram || frame->width <= 0 ||
                frame->width > 1024 || (frame->width & 1) ||
                frame->height <= 0 || frame->height > 600 ||
                frame->pitch < frame->width * 2 || frame->pitch > 2048 ||
                frame->pitch * frame->height > MAX_FRAME_BYTES)
            {
                msg.id = PTP_RC_GeneralError;
            }
            else
            {
                uint32_t bytes = frame->pitch * frame->height;
                uint8_t *snapshot = malloc(FRAME_HEADER_SIZE + bytes);
                if (!snapshot)
                {
                    msg.id = PTP_RC_GeneralError;
                }
                else
                {
                    snapshot[0] = 'M'; snapshot[1] = 'L';
                    snapshot[2] = 'V'; snapshot[3] = 'F';
                    write_u16(snapshot + 4, frame->width);
                    write_u16(snapshot + 6, frame->height);
                    write_u32(snapshot + 8, frame->pitch);
                    write_u32(snapshot + 12, bytes);
                    /* Canon may swap the VRAM buffer during USB transfer.
                     * A prompt copy limits (but cannot eliminate) tearing. */
                    memcpy(snapshot + FRAME_HEADER_SIZE, frame->vram, bytes);
                    if (!send_ptp_data(context, (char *)snapshot,
                                       FRAME_HEADER_SIZE + bytes))
                        msg.id = PTP_RC_GeneralError;
                    free(snapshot);
                }
            }
        }
    }
    else
    {
        msg.id = PTP_RC_ParameterNotSupported;
    }

    context->send_resp(context->handle, &msg);
    return 0;
}
