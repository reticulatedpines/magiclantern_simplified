/* EOS M 2.0.2: make Canon EOS Utility 2 offer "Remote shooting" / live view.
 *
 * 1) Canon registers the remote shooting / live view PTP opcodes only after the
 *    event procedure FA_SetRemoteSpecialMode(1) has run.
 * 2) EOS Utility 2 picks its per-model class from the model ID that the camera reports
 *    in PTP property 0xD116 (= Canon property PROP_CAM_MODEL, ID at buf + 32).
 *    The EOS M class (0x80000331) hides Remote shooting; the 650D one (0x80000301) shows it.
 *
 * RAM only, nothing is flashed. Unload by removing the module and rebooting.
 * Written with LLM assistance (see LLM_POLICY.md): the code is a call and a one-word write.
 * Debug log: ML/LOGS/EU2TETHR.LOG */
#include <module.h>
#include <dryos.h>
#include <property.h>
#include <fio-ml.h>

#define MODEL_ID_650D 0x80000301
#define LOG_FILE "ML/LOGS/EU2TETHR.LOG"

static void logmsg(const char *fmt, uint32_t a, uint32_t b)
{
    char line[96];
    snprintf(line, sizeof(line), fmt, a, b);
    FILE *f = FIO_CreateFileOrAppend(LOG_FILE);
    if (f)
    {
        FIO_WriteFile(f, line, strlen(line));
        FIO_CloseFile(f);
    }
}

static unsigned int eu2tethr_init()
{
    logmsg("init start %x %x\n", 0, 0);
    call("FA_SetRemoteSpecialMode", 1);
    logmsg("init done, FA_SetRemoteSpecialMode(1) called %x %x\n", 0, 0);

    /* The property is set once at boot, before this module loads, so a handler never fires.
     * Canon's prop_get_value (EOS M 2.0.2 ROM 0xFF1202E8, not exported as a stub) returns the
     * address of the live property data: patch the model ID there. */
    uint8_t *data = 0;
    size_t plen = 0;
    int (*prop_get_value_rom)(unsigned, void **, size_t *) = (void *) 0xFF1202E8;
    int err = prop_get_value_rom(PROP_CAM_MODEL, (void **) &data, &plen);
    logmsg("prop_get_value err=%d len=%d\n", err, plen);
    if (!err && data && plen >= 36)
    {
        logmsg("storage at %x, old id=%x\n", (uint32_t) data, *(uint32_t *)(data + 32));
        *(volatile uint32_t *)(data + 32) = MODEL_ID_650D;
        logmsg("new id=%x, name word=%x\n", *(uint32_t *)(data + 32), *(uint32_t *)data);

        /* the PTP layer queued its 0xD116 event at boot with the old value: ask the property
         * manager for a change with the patched data, so a fresh event (and value) is generated */
        prop_request_change(PROP_CAM_MODEL, data, plen);
        logmsg("prop_request_change done %x %x\n", 0, 0);
    }
    return 0;
}

static unsigned int eu2tethr_deinit()
{
    return 0;
}

PROP_HANDLER(PROP_CAM_MODEL)
{
    uint32_t old = len >= 36 ? *(uint32_t *)((uint8_t *)buf + 32) : 0;
    if (len >= 36)
    {
        *(volatile uint32_t *)((uint8_t *)buf + 32) = MODEL_ID_650D;
    }
    logmsg("PROP_CAM_MODEL handler len=%d old id=%x\n", len, old);
}

MODULE_INFO_START()
    MODULE_INIT(eu2tethr_init)
    MODULE_DEINIT(eu2tethr_deinit)
MODULE_INFO_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_CAM_MODEL)
MODULE_PROPHANDLERS_END()
