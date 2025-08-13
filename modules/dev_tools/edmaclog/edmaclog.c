// EDMAC logging, next gen.
//
// The old logger doesn't log things we want,
// doesn't account for changes with D678X,
// hard-codes addresses which have changed, etc.
// And it's not documented well.
// Just do it all again.

#include "module.h"
#include "dryos.h"
#include "compiler.h"
#include "menu.h"
#include "console.h"
#include "bmp.h"
#include "log.h"
#include "patch_mmu.h"

#include "hooks.h"

// D678X cams don't provide this symbol, it's for cache hack patching,
// which they don't have.  D45 cams will override this with real code at link time.
extern WEAK_FUNC(ret_0) int patch_hook_function(uintptr_t addr, uint32_t orig_instr,
                                                patch_hook_function_cbr hook_function,
                                                const char *description);

// D45 cams don't provide this symbol, it's for MMU cams
extern WEAK_FUNC(ret_0) int convert_f_patch_to_patch(struct function_hook_patch *f_patch_in,
                                                     struct patch *patch_out,
                                                     uint8_t *hook_mem_out);

static uint32_t is_log_enabled = 0;
static void log_control_task()
{
    if (is_log_enabled)
    {
        is_log_enabled = 0;
        send_log_data_str("Disabling log.\n");
        disable_logging();
    }
    else
    {
        is_log_enabled = 1;
        enable_logging();
        send_log_data_str("Enabling log.\n");
    }
}
 
static struct menu_entry edmac_menu[] =
{
    {
        .name   = "Toggle EDMAC logging",
        .select = run_in_separate_task,
        .priv   = log_control_task,
        .icon_type = IT_ACTION,
    }
};

static unsigned int edmac_init()
{
    static uint8_t *log_buf = NULL;
    log_buf = malloc(MIN_LOG_BUF_SIZE);
    if (log_buf == NULL)
        return -1;

    if (init_log(log_buf, MIN_LOG_BUF_SIZE, "edmac.log") < 0)
        return -2;

    if (is_camera("200D", "1.0.1"))
    {
        // install 200D hooks
        struct function_hook_patch f_patches[] = {
            {
                .patch_addr = 0xe04b3738, // CreateResLockEntry
                .orig_content = {0x2d, 0xe9, 0xf0, 0x47, 0x80, 0x46, 0xd8, 0x4d},
                .target_function_addr = (uint32_t)hook_CreateResLockEntry_200D,
                .description = "Log CreateResLockEntry"
            },
            {
                .patch_addr = 0x35486, // edmac_set_addr
                .orig_content = {0x76, 0x4a, 0x21, 0xf0, 0x40, 0x41, 0x52, 0xf8},
                .target_function_addr = (uint32_t)hook_set_addr_200D,
                .description = "Log SetAddr"
            },
            {
                .patch_addr = 0x35506, // edmac_set_size
                .orig_content = {0xf0, 0xb5, 0x05, 0x46, 0x55, 0x4e, 0x89, 0xb0},
                .target_function_addr = (uint32_t)hook_set_size_200D,
                .description = "Log SetSize"
            },
            {
                .patch_addr = 0x3649c, // ConnectWriteEDMAC
                .orig_content = {0x70, 0xb5, 0x35, 0x28, 0x0d, 0x46, 0x04, 0x46},
                .target_function_addr = (uint32_t)hook_ConnectWriteEDMAC_200D,
                .description = "Log ConnectWriteEDMAC"
            },
            {
                .patch_addr = 0x364f0, // ConnectReadEDMAC
                .orig_content = {0x70, 0xb5, 0x35, 0x28, 0x0d, 0x46, 0x04, 0x46},
                .target_function_addr = (uint32_t)hook_ConnectReadEDMAC_200D,
                .description = "Log ConnectReadEDMAC"
            },
            {
                .patch_addr = 0x35392, // StartEDMAC
                .orig_content = {0xc9, 0x4a, 0x00, 0x21, 0xd4, 0x3a, 0x42, 0xf8},
                .target_function_addr = (uint32_t)hook_StartEDMAC_200D,
                .description = "Log StartEDMAC"
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
                return -3;
            }
        }
        apply_patches(patches, COUNT(f_patches));
    }
    if (is_camera("6D2", "1.1.1"))
    {
        // install 6D2 hooks
        struct function_hook_patch f_patches[] = {
            {
                .patch_addr = 0xe04aa70c, // CreateResLockEntry
                .orig_content = {0x2d, 0xe9, 0xf0, 0x47, 0x80, 0x46, 0xd8, 0x4d},
                .target_function_addr = (uint32_t)hook_CreateResLockEntry_6D2,
                .description = "Log CreateResLockEntry"
            },
            {
                .patch_addr = 0x2e27e, // edmac_set_addr
                .orig_content = {0x76, 0x4a, 0x21, 0xf0, 0x40, 0x41, 0x52, 0xf8},
                .target_function_addr = (uint32_t)hook_set_addr_6D2,
                .description = "Log SetAddr"
            },
            {
                .patch_addr = 0x2e2fe, // edmac_set_size
                .orig_content = {0xf0, 0xb5, 0x05, 0x46, 0x55, 0x4e, 0x89, 0xb0},
                .target_function_addr = (uint32_t)hook_set_size_6D2,
                .description = "Log SetSize"
            },
            {
                .patch_addr = 0x2f294, // ConnectWriteEDMAC
                .orig_content = {0x70, 0xb5, 0x35, 0x28, 0x0d, 0x46, 0x04, 0x46},
                .target_function_addr = (uint32_t)hook_ConnectWriteEDMAC_6D2,
                .description = "Log ConnectWriteEDMAC"
            },
            {
                .patch_addr = 0x2f2e8, // ConnectReadEDMAC
                .orig_content = {0x70, 0xb5, 0x35, 0x28, 0x0d, 0x46, 0x04, 0x46},
                .target_function_addr = (uint32_t)hook_ConnectReadEDMAC_6D2,
                .description = "Log ConnectReadEDMAC"
            },
            {
                .patch_addr = 0x2e18a, // StartEDMAC
                .orig_content = {0xc9, 0x4a, 0x00, 0x21, 0xd4, 0x3a, 0x42, 0xf8},
                .target_function_addr = (uint32_t)hook_StartEDMAC_6D2,
                .description = "Log StartEDMAC"
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
                return -3;
            }
        }
        apply_patches(patches, COUNT(f_patches));
    }
    else if (is_camera("70D", "1.1.2"))
    {
            patch_hook_function(0xff2c049c, // CreateResLockEntry
                                0xe92d47f0, // orig_instr
                                hook_CreateResLockEntry_70D,
                                "Log CreateResLockEntry");
            patch_hook_function(0x3817c, // StartEDMAC
                                0xe92d47ff, // orig_instr
                                hook_StartEDMAC_70D,
                                "Log StartEDMAC");
            patch_hook_function(0x37ff4, // ConnectReadEDMAC
                                0xe92d40fe, // orig_instr
                                hook_ConnectReadEDMAC_70D,
                                "Log ConnectReadEDMAC");
            patch_hook_function(0x37f30, // ConnectWriteEDMAC
                                0xe92d403e, // orig_instr
                                hook_ConnectWriteEDMAC_70D,
                                "Log ConnectWriteDMAC");
            patch_hook_function(0x37dd0, // SetEDMAC
                                0xe92d40fe, // orig_instr
//            patch_hook_function(0x37a90, // _SetEDMAC, higher one doesn't like being hooked?
//                                0xe92d47f0, // orig_instr
                                hook_SetEDMAC_70D,
                                "Log SetEDMAC");
    }
    else if (is_camera("5D3", "1.2.3"))
    {
            patch_hook_function(0x12910, // StartEDMAC
                                0xe92d47ff, // orig_instr
                                hook_StartEDMAC_5D3,
                                "Log StartEDMAC");
            patch_hook_function(0x12768, // ConnectReadEDMAC
                                0xe92d40fe, // orig_instr
                                hook_ConnectReadEDMAC_5D3,
                                "Log ConnectReadEDMAC");
            patch_hook_function(0x126a4, // ConnectWriteEDMAC
                                0xe92d403e, // orig_instr
                                hook_ConnectWriteEDMAC_5D3,
                                "Log ConnectWriteDMAC");
            patch_hook_function(0x125f8, // SetEDMAC
                                0xe92d40fe, // orig_instr
                                hook_SetEDMAC_5D3,
                                "Log SetEDMAC");
    }
    else
    {
        bmp_printf(FONT_MED, 20, 20, "No hooks defined for this cam, cannot start logging");
        return CBR_RET_ERROR;
    }
    
    menu_add("Debug", edmac_menu, COUNT(edmac_menu));
    return 0;
}

static unsigned int edmac_deinit()
{
    menu_remove("Debug", edmac_menu, COUNT(edmac_menu));
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(edmac_init)
    MODULE_DEINIT(edmac_deinit)
MODULE_INFO_END()
