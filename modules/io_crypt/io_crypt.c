
#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

#include <io_crypt.h>

#include "crypt_lfsr64.h"
#include "crypt_rsa.h"

#include "../trace/trace.h"
#include "../ime_base/ime_base.h"

char *strncpy(char *dest, const char *src, size_t n);

static iodev_handlers_t *orig_iodev = NULL;
static iodev_handlers_t hook_iodev;

uint32_t iocrypt_trace_ctx = TRACE_ERROR;

static uint8_t *iocrypt_scratch = NULL;
static uint64_t iocrypt_key = 0;
static uint32_t iocrypt_disabled = 1;

static fd_map_t iocrypt_files[32];
static struct semaphore *iocrypt_password_sem = 0;
static char iocrypt_ime_text[128];

/* those are model specific */
static uint32_t iodev_table = 0;
static uint32_t iodev_ctx = 0;
static uint32_t iodev_ctx_size = 0;


static IME_UPDATE_FUNC(iocrypt_ime_update)
{
    return IME_OK;
}

static IME_DONE_FUNC(iocrypt_ime_done)
{
    gui_stop_menu();
    
    /* if key input dialog was cancelled */
    if(status != IME_OK)
    {
        iocrypt_disabled = 1;
        NotifyBox(2000, "Crypto disabled");
        beep();

        give_semaphore(iocrypt_password_sem);
        return IME_OK;
    }
    
    hash_password(text, &iocrypt_key);
    
    /* done, use that key */
    iocrypt_disabled = 0;
    give_semaphore(iocrypt_password_sem);
    
#if 0    
    /* do some unaligned writes to check alignment handling */
    uint8_t buffer[256];
    memset(buffer, 0x00, sizeof(buffer));
    
    FILE* f = FIO_CreateFileEx("B:/IO_CRYPT.CR2");
    for(int loop = 0; loop < 10 * 1024; loop++)
    {
        FIO_WriteFile(f, &buffer[0], 19);
        FIO_WriteFile(f, &buffer[1], 21);
        FIO_WriteFile(f, &buffer[2], 33);
        FIO_WriteFile(f, &buffer[3], 31);
        FIO_WriteFile(f, &buffer[7], 3);
        bmp_printf(FONT_LARGE, 0, 60, "Loop: %d", loop);
    }
    
    FIO_CloseFile(f);
    beep();
#endif
    
    return IME_OK;
}

static uint32_t iodev_GetPosition(uint32_t fd)
{
    uint32_t *ctx = (uint32_t *)(iodev_ctx + iodev_ctx_size * fd);
    
    return ctx[2];
}


/* these are the iodev hooks */
static uint32_t hook_iodev_CloseFile(uint32_t fd)
{
    trace_write(iocrypt_trace_ctx, "iodev_CloseFile(%d)", fd);
    
    if(fd < COUNT(iocrypt_files))
    {
        iocrypt_files[fd].crypt_ctx->deinit((void **)&iocrypt_files[fd].crypt_ctx);
        iocrypt_files[fd].crypt_ctx = NULL;
    }
    
    uint32_t ret = orig_iodev->CloseFile(fd);
    
    return ret;
}

static uint32_t hook_iodev_OpenFile(void *iodev, char *filename, int32_t flags, char *filename_)
{
    uint32_t fd = orig_iodev->OpenFile(iodev, filename, flags, filename_);
    
    trace_write(iocrypt_trace_ctx, "iodev_OpenFile('%s', %d) = %d", filename, flags, fd);
    
    if(fd < COUNT(iocrypt_files))
    {
        iocrypt_files[fd].crypt_ctx = NULL;
        
        /* copy filename */
        strncpy(iocrypt_files[fd].filename, filename, sizeof(iocrypt_files[fd].filename));
        
        /* shall we crypt the file? */
        if(!iocrypt_disabled)
        {
            if(!strcmp(&filename[strlen(filename) - 3], "CR2") || !strcmp(&filename[strlen(filename) - 3], "JPG"))
            {
                crypt_lfsr64_init(&(iocrypt_files[fd].crypt_ctx), iocrypt_key);
            }
        }

        /* log the encryption status */
        if(iocrypt_files[fd].crypt_ctx)
        {
            trace_write(iocrypt_trace_ctx, "   ->> ENCRYPTED '%s'", filename);
        }
        else
        {
            trace_write(iocrypt_trace_ctx, "   ->> plain '%s'", filename);
        }
    }
    
    return fd;
}

static uint32_t hook_iodev_ReadFile(uint32_t fd, uint8_t *buf, uint32_t length)
{
    uint32_t fd_pos = iodev_GetPosition(fd);
    uint32_t ret = orig_iodev->ReadFile(fd, buf, length);
    
    if(fd < COUNT(iocrypt_files))
    {
        trace_write(iocrypt_trace_ctx, "iodev_ReadFile(0x%08X, 0x%08X) -> fd = %d, fd_pos = 0x%08X, %s", buf, length, fd, fd_pos, iocrypt_files[fd].filename);
        
        if(iocrypt_files[fd].crypt_ctx)
        {
            trace_write(iocrypt_trace_ctx, "   ->> DECRYPT");
            iocrypt_files[fd].crypt_ctx->decrypt(iocrypt_files[fd].crypt_ctx, buf, buf, length, fd_pos);
            trace_write(iocrypt_trace_ctx, "   ->> DONE");
        }
    }
    
    return ret;
}

static uint32_t hook_iodev_WriteFile(uint32_t fd, uint8_t *buf, uint32_t length)
{
    uint32_t ret = 0;
    
    if(fd < COUNT(iocrypt_files))
    {
        uint8_t *work_ptr = buf;
        uint32_t misalign = ((uint32_t)buf) % 8;
        uint32_t fd_pos = iodev_GetPosition(fd);
        
        if(iocrypt_files[fd].crypt_ctx)
        {
            trace_write(iocrypt_trace_ctx, "iodev_WriteFile pre(0x%08X, 0x%08X) -> fd = %d, fd_pos = 0x%08X, misalign = %d, %s", buf, length, fd, fd_pos, misalign, iocrypt_files[fd].filename);
        
            /* if there is no buffer or the size is too big, do expensive in-place encryption */
            if(iocrypt_scratch && (length <= CRYPT_SCRATCH_SIZE))
            {
                /* update buffer to write from */
                work_ptr = &iocrypt_scratch[misalign];
                trace_write(iocrypt_trace_ctx, "   ->> double buffered");
            }
            
            trace_write(iocrypt_trace_ctx, "   ->> ENCRYPT");
            iocrypt_files[fd].crypt_ctx->encrypt(iocrypt_files[fd].crypt_ctx, work_ptr, buf, length, fd_pos);
            trace_write(iocrypt_trace_ctx, "   ->> DONE");
            
            ret = orig_iodev->WriteFile(fd, work_ptr, length);
            
            trace_write(iocrypt_trace_ctx, "iodev_WriteFile post(0x%08X, 0x%08X) -> fd = %d, fd_pos = 0x%08X, fd_pos (now) = 0x%08X, %s", buf, length, fd, fd_pos, iodev_GetPosition(fd), iocrypt_files[fd].filename);
            
            /* if we were not able to write it from scratch, undo in-place encryption */
            if(work_ptr == buf)
            {
                trace_write(iocrypt_trace_ctx, "   ->> CLEANUP");
                iocrypt_files[fd].crypt_ctx->decrypt(iocrypt_files[fd].crypt_ctx, buf, buf, length, fd_pos);
                trace_write(iocrypt_trace_ctx, "   ->> DONE");
            }
            
            return ret;
        }
    }
    
    /* handle by default method */
    ret = orig_iodev->WriteFile(fd, buf, length);
    
    return ret;
}

static void iocrypt_enter_pw()
{
    /* ensure there is only one dialog */
    take_semaphore(iocrypt_password_sem, 0);
    
    gui_open_menu();
    
    if(!ime_base_start("io_crypt: Enter password", iocrypt_ime_text, sizeof(iocrypt_ime_text) - 1, IME_UTF8, IME_CHARSET_ANY, &iocrypt_ime_update, &iocrypt_ime_done, 0, 0, 0, 0))
    {
        give_semaphore(iocrypt_password_sem);
        iocrypt_disabled = 1;
        NotifyBox(2000, "IME error, Crypto disabled");
    }
}

static MENU_SELECT_FUNC(iocrypt_enter_pw_select)
{
    iocrypt_enter_pw();
}

static MENU_SELECT_FUNC(iocrypt_test_rsa)
{
    crypt_cipher_t *crypt_ctx;
    crypt_rsa_init(&crypt_ctx, 0xDEADBEEF11223344);
}

static struct menu_entry iocrypt_menus[] =
{
    {
        .name = "Enter io_crypt password",
        .select = &iocrypt_enter_pw_select,
        .priv = NULL,
        .icon_type = IT_ACTION,
    },
    {
        .name = "Test RSA",
        .select = &iocrypt_test_rsa,
        .priv = NULL,
        .icon_type = IT_ACTION,
    }
};

static unsigned int iocrypt_init()
{
    if(streq(camera_model_short, "600D"))
    {
        /* not verified */
        iodev_table = 0x1E684;
        iodev_ctx = 0x7EB08;
        iodev_ctx_size = 0x18;
    }
    else if(streq(camera_model_short, "7D"))
    {
        /* not verified */
        iodev_table = 0x2D3B8;
        iodev_ctx = 0x85510;
        iodev_ctx_size = 0x18;
    }
    else if(streq(camera_model_short, "60D"))
    {
        /* not verified */
        iodev_table = 0x3E2BC;
        iodev_ctx = 0x5CB38;
        iodev_ctx_size = 0x18;
    }
    else if(streq(camera_model_short, "5D3"))
    {
        iodev_table = 0x44FA8;
        iodev_ctx = 0x67140;
        iodev_ctx_size = 0x20;
    }
    else
    {
        NotifyBox(2000, "io_crypt: Camera unsupported");
        return 0;
    }
    
    iocrypt_password_sem = create_named_semaphore("iocrypt_pw", 1);
    
    /* for debugging */
    if(1)
    {
        iocrypt_trace_ctx = trace_start("debug", "B:/IO_CRYPT.TXT");
        trace_set_flushrate(iocrypt_trace_ctx, 1000);
        trace_format(iocrypt_trace_ctx, TRACE_FMT_TIME_REL | TRACE_FMT_COMMENT, ' ');
        trace_write(iocrypt_trace_ctx, "io_crypt: Starting trace");
    }
    
    /* ask for the initial password */
    iocrypt_enter_pw();
    
    /* this memory is used for buffering encryption, so we dont have to undo the changes in memory */
    iocrypt_scratch = shoot_malloc(CRYPT_SCRATCH_SIZE);
    
    /* now patch the iodev handlers */
    orig_iodev = (iodev_handlers_t *)MEM(iodev_table);
    hook_iodev = *orig_iodev;
    hook_iodev.OpenFile = &hook_iodev_OpenFile;
    hook_iodev.ReadFile = &hook_iodev_ReadFile;
    hook_iodev.WriteFile = &hook_iodev_WriteFile;
    MEM(iodev_table) = (uint32_t)&hook_iodev;
    
    /* any file operation is routed through us now */
    menu_add("Debug", iocrypt_menus, COUNT(iocrypt_menus) );
    
    
    return 0;
}

static unsigned int iocrypt_deinit()
{
    MEM(iodev_table) = (uint32_t)orig_iodev;
    shoot_free(iocrypt_scratch);
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(iocrypt_init)
    MODULE_DEINIT(iocrypt_deinit)
MODULE_INFO_END()

