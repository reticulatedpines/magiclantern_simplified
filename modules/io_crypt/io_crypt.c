
/*
 * Copyright (C) 2013 Magic Lantern Team
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
 
/*
    About how to find the model specific memory addresses
    (example 7D v2.0.3, slave)

    finding iodev_table
    1. starting point FIO_ReadFile at 0xFF1FC434
    2. if file handle id is < 100, it enters 0xFF3279F4
    3. there the first call is 0xFF08CFA8
    4. this function references to 0x2D3B0 (pointers to tables of functions) and calls a function from there using BX
    5. as we (always) use the second entry in that table, our iodev_table = 0x2D3B8 (it points to 0xFF58B350)

    the function tables have these functions:
      0xFF58B350:
      DCD iodev_OpenFile
      DCD iodev_CloseFile
      DCD iodev_unsupported
      DCD iodev_ReadFile
      DCD iodev_WriteFile
      DCD iodev_unsupported
      DCD iodev_unsupported
      DCD iodev_unsupported_2
      DCD iodev_unsupported

    finding iodev_ctx (to be optimized away..)
    1. go to iodev_OpenFile, which is at 0xFF458BBC (referenced in table found above)
    2. enter the 3rd function 0xFF3E0D50. its the one with return value being checked (SUBS R5, R0, #0)
    3. the first function (0xFF3E0958) being entered is allocating an fd.
    4. this function references to 0x85510, so iodev_ctx = 0x85510

    finding iodev_ctx_size (to be optimized away..)
    1. the same function as you used above (0xFF3E0958) goes through all entries
    2. it adds 0x18 bytes per iteration, so iodev_ctx_size = 0x18

*/
 
#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <string.h>
#include <io_crypt.h>

#include "crypt_lfsr64.h"
#include "crypt_rsa.h"

#include "../trace/trace.h"
#include "../ime_base/ime_base.h"

uint32_t iocrypt_trace_ctx = TRACE_ERROR;

/* magics used to detect or set file type */
static const uint8_t cr2_magic[] = "\x49\x49\x2A\x00";
static const uint8_t jpg_magic[] = "\xff\xd8\xff\xe1";
static const uint8_t rsa_magic[] = "\xff\xd8\xff\xd8";
static const uint8_t lfsr_magic[] = "\xff\xd8\xff\x8d";

/* FIO function hooks */
static iodev_handlers_t *orig_iodev = NULL;
static iodev_handlers_t hook_iodev;

/* those are model specific */
static uint32_t iodev_table = 0;
static uint32_t iodev_ctx = 0;
static uint32_t iodev_ctx_size = 0;

/* every file descriptor gets its own entry in this table */
static fd_map_t iocrypt_files[32];

/* scratch memory is used to put encrypted data into before it gets written */
static uint8_t *iocrypt_scratch = NULL;

/* current encryption key, derieved from password being entered */
static uint64_t iocrypt_key = 0;

/* status for RSA key generation */
static uint32_t iocrypt_rsa_key_active = 0;

/* RSA has a static context */
static crypt_cipher_t iocrypt_rsa_ctx;

/* variables needed to set up password */
static struct semaphore *iocrypt_password_sem = 0;
static char iocrypt_ime_text[128];

/* crypt thread data */
struct msg_queue *iocrypt_msgs = NULL;
static uint32_t iocrypt_shutdown = 0;

/* en-/decrypt on the fly with a symmetric cipher and given password */
#define CRYPT_MODE_SYMMETRIC            0

/* encrypt on the fly using a random key which gets stored using an asymmetric cipher. decryption on PC only. */
#define CRYPT_MODE_ASYMMETRIC           1
#define CRYPT_MODE_ASYMMETRIC_PARANOID  2

/* same ciphers, but files are stored unencrypted and get encrypted when camera is idle */
#define CRYPT_MODE_BACKGROUND_SYM       3
#define CRYPT_MODE_BACKGROUND_ASYM      4


/* dont redirect any file to fake images */
#define CRYPT_FAKE_OFF                  0
/* if encrypytion key is unknown, show fake image */
#define CRYPT_FAKE_UNREADABLE           1
/* if the file is encrypted, no matter if key is correct or not, show fake image */
#define CRYPT_FAKE_ENCRYPTED            2
/* all images are replaced with fake ones */
#define CRYPT_FAKE_ALL                  3


static CONFIG_INT("io_crypt.enabled", iocrypt_enabled, 0);
static CONFIG_INT("io_crypt.mode", iocrypt_mode, 0);
static CONFIG_INT("io_crypt.fake", iocrypt_fake, 0);
static CONFIG_INT("io_crypt.block_size", iocrypt_block_size, 4);
static CONFIG_INT("io_crypt.ask_pass", iocrypt_ask_pass, 0);
static CONFIG_INT("io_crypt.rsa_key_size", iocrypt_rsa_key_size, 2);


static IME_UPDATE_FUNC(iocrypt_ime_update)
{
    return IME_OK;
}

static IME_DONE_FUNC(iocrypt_ime_done)
{
    //gui_stop_menu();
    
    /* if key input dialog was cancelled */
    if(status != IME_OK)
    {
        iocrypt_key = 0;
        NotifyBox(2000, "Crypto disabled");
        beep();

        give_semaphore(iocrypt_password_sem);
        return IME_OK;
    }
    
    hash_password(text, &iocrypt_key);
    
    /* done, use that key */
    give_semaphore(iocrypt_password_sem);
    
    return IME_OK;
}

/* ToDo: these directly read/write the current FIO position by altering FIO private data. is FIO_SeekFile safe to use here? */
static uint32_t iodev_GetPosition(uint32_t fd)
{
    uint32_t *ctx = (uint32_t *)(iodev_ctx + iodev_ctx_size * fd);
    
    return ctx[2];
}

static void iodev_SetPosition(uint32_t fd, uint32_t pos)
{
    uint32_t *ctx = (uint32_t *)(iodev_ctx + iodev_ctx_size * fd);
    ctx[2] = pos;
}


/* these are the iodev hooks */
static uint32_t hook_iodev_CloseFile(uint32_t fd)
{
    trace_write(iocrypt_trace_ctx, "iodev_CloseFile(%d)", fd);
    
    if(fd < COUNT(iocrypt_files))
    {
        if(iocrypt_files[fd].crypt_ctx.priv)
        {
            iocrypt_files[fd].crypt_ctx.deinit(iocrypt_files[fd].crypt_ctx.priv);
            iocrypt_files[fd].crypt_ctx.priv = NULL;
        }
    }
    
    uint32_t ret = orig_iodev->CloseFile(fd);
    
    return ret;
}

static uint32_t iocrypt_asym_init(int fd)
{
    uint64_t file_key = 0;
    uint32_t lfsr_blocksize = (8192 << iocrypt_block_size);
    uint32_t blocksize = crypt_rsa_blocksize(iocrypt_rsa_ctx.priv);
    
    trace_write(iocrypt_trace_ctx, "iocrypt_save_asym_hdr: block size %d bytes, lfsr_blocksize %d bytes", blocksize, lfsr_blocksize);
    if(!blocksize)
    {
        return 0;
    }
    char *key = malloc(blocksize + 32);
    
    /* create a random per-file crypt key */
    rand_fill(key, blocksize / 4);
    memcpy(&file_key, key, sizeof(uint64_t));

    /* encrypt the randomly generated per-file header with RSA public key */
    volatile iocrypt_job_t job;
    
    job.type = CRYPT_JOB_ENCRYPT;
    job.semaphore = iocrypt_files[fd].semaphore;
    job.ctx = &iocrypt_rsa_ctx;
    job.dst = key;
    job.buf = key;
    job.length = blocksize;
    job.fd_pos = 0;
    
    trace_write(iocrypt_trace_ctx, "iocrypt_asym_init: encrypt");
    msg_queue_post(iocrypt_msgs, &job);

    /* wait until worker finished */
    take_semaphore(job.semaphore, 0);
    trace_write(iocrypt_trace_ctx, "iocrypt_asym_init: encrypt done (%d)", job.ret);
    
    uint32_t encrypted_size = job.ret;
    
    /* write header, block size and encrypted key */
    orig_iodev->WriteFile(fd, (uint8_t*)rsa_magic, 4);
    orig_iodev->WriteFile(fd, (uint8_t*)&encrypted_size, sizeof(uint32_t));
    orig_iodev->WriteFile(fd, (uint8_t*)&lfsr_blocksize, sizeof(uint32_t));
    orig_iodev->WriteFile(fd, (uint8_t*)key, encrypted_size);
    free(key);
    
    uint32_t used_header = 4 + sizeof(uint32_t) + sizeof(uint32_t) + encrypted_size;
    uint32_t aligned_header = (used_header + 0x1FF) & ~0x1FF;
    uint32_t remain = aligned_header - used_header;
    
    /* fill remaining space with random data */
    char *filler = malloc(remain);
    rand_fill(filler, remain / 4);
    orig_iodev->WriteFile(fd, (uint8_t*)filler, remain);
    free(filler);
    
    iocrypt_files[fd].header_size = aligned_header;
    
    /* rewind file */
    iodev_SetPosition(fd, 0);
    
    /* init encryption */
    iocrypt_files[fd].file_key = file_key;
    crypt_lfsr64_init(&iocrypt_files[fd].crypt_ctx, iocrypt_files[fd].file_key);
    iocrypt_files[fd].crypt_ctx.set_blocksize(iocrypt_files[fd].crypt_ctx.priv, lfsr_blocksize);

    return 1;
}

static uint32_t iocrypt_sym_init(int fd)
{
    uint32_t lfsr_blocksize = (8192 << iocrypt_block_size);
    
    trace_write(iocrypt_trace_ctx, "iocrypt_sym_init: lfsr_blocksize %d bytes", lfsr_blocksize);
    
    /* write header, block size and encrypted key */
    orig_iodev->WriteFile(fd, (uint8_t*)lfsr_magic, 4);
    orig_iodev->WriteFile(fd, (uint8_t*)&lfsr_blocksize, sizeof(uint32_t));
    
    iocrypt_files[fd].header_size = 0x200;
    uint32_t remain = 0x200 - 8;
    
    /* fill remaining space with random data */
    char *filler = malloc(remain);
    rand_fill(filler, remain / 4);
    orig_iodev->WriteFile(fd, (uint8_t*)filler, remain);
    free(filler);
    
    /* rewind file */
    iodev_SetPosition(fd, 0);
    
    /* init encryption */
    iocrypt_files[fd].file_key = iocrypt_key;
    crypt_lfsr64_init(&iocrypt_files[fd].crypt_ctx, iocrypt_files[fd].file_key);
    iocrypt_files[fd].crypt_ctx.set_blocksize(iocrypt_files[fd].crypt_ctx.priv, lfsr_blocksize);

    return 1;
}

static uint32_t hook_iodev_OpenFile(void *iodev, char *filename, int32_t flags, char *filename_)
{
    int plain = 0;
    int decryptable = 0;
    
    uint32_t fd = orig_iodev->OpenFile(iodev, filename, flags, filename_);
    
    trace_write(iocrypt_trace_ctx, "iodev_OpenFile('%s', %d) = %d", filename, flags, fd);
    
    if(fd < COUNT(iocrypt_files) && iocrypt_enabled)
    {
        iocrypt_files[fd].crypt_ctx.priv = NULL;
        iocrypt_files[fd].header_size = 0;
    
        FIO_GetFileSize(filename, &iocrypt_files[fd].file_size);
        
        /* copy filename */
        strncpy(iocrypt_files[fd].filename, filename, sizeof(iocrypt_files[fd].filename));
        
        char *ext = &filename[strlen(filename) - 3];
        
        /* analyze file */
        if(!strcmp(ext, "CR2") || !strcmp(ext, "JPG"))
        {
            /* when opening for read, first check if we really have to decrypt it */
            if((flags & 3) == O_RDONLY)
            {
                uint8_t buf[4];
                
                /* read and rewind file */
                orig_iodev->ReadFile(fd, buf, 4);
                iodev_SetPosition(fd, 0);
                
                /* check for JPEG or CR2 header */
                if(!memcmp(buf, jpg_magic, 4))
                {
                    trace_write(iocrypt_trace_ctx, "   ->> File '%s' seems to be unencrypted JPEG", filename);
                    plain = 1;
                }
                else if(!memcmp(buf, cr2_magic, 4))
                {
                    trace_write(iocrypt_trace_ctx, "   ->> File '%s' seems to be unencrypted CR2", filename);
                    plain = 1;
                }
                else if(!memcmp(buf, lfsr_magic, 4))
                {
                    trace_write(iocrypt_trace_ctx, "   ->> File '%s' seems to be LFSR encrypted, try to decrypt on the fly", filename);
                    plain = 0;
                    
                    uint32_t lfsr_blocksize = 0;
                    iodev_SetPosition(fd, 4);
                    orig_iodev->ReadFile(fd, (uint8_t *)&lfsr_blocksize, 4);
                    iodev_SetPosition(fd, 0x200);
                    
                    crypt_lfsr64_init(&iocrypt_files[fd].crypt_ctx, iocrypt_key);
                    iocrypt_files[fd].crypt_ctx.set_blocksize(iocrypt_files[fd].crypt_ctx.priv, lfsr_blocksize);
                    
                    if(iocrypt_files[fd].crypt_ctx.priv)
                    {
                        /* read again to check if it is decryptable now */
                        orig_iodev->ReadFile(fd, buf, 4);
                        iodev_SetPosition(fd, 0);
                        
                        iocrypt_files[fd].crypt_ctx.decrypt(iocrypt_files[fd].crypt_ctx.priv, buf, buf, 4, 0);
            
                        if(!memcmp(buf, jpg_magic, 4))
                        {
                            trace_write(iocrypt_trace_ctx, "   ->> File '%s' seems to be decryptable JPEG", filename);
                            decryptable = 1;
                        }
                        else if(!memcmp(buf, cr2_magic, 4))
                        {
                            trace_write(iocrypt_trace_ctx, "   ->> File '%s' seems to be decryptable CR2", filename);
                            decryptable = 1;
                        }
                        else
                        {
                            trace_write(iocrypt_trace_ctx, "   ->> File '%s' seems to be not decryptable", filename);
                        }
                        
                        /* shall we crypt the file? if not, release context */
                        if(!decryptable)
                        {
                            iocrypt_files[fd].crypt_ctx.deinit(iocrypt_files[fd].crypt_ctx.priv);
                            iocrypt_files[fd].crypt_ctx.priv = NULL;
                        }
                        else
                        {
                            iocrypt_files[fd].header_size = 0x200;
                        }
                    }
                }
                else if(!memcmp(buf, rsa_magic, 4))
                {
                    trace_write(iocrypt_trace_ctx, "   ->> File '%s' seems to be RSA encrypted", filename);
                }
                else
                {
                    trace_write(iocrypt_trace_ctx, "   ->> File '%s' is unknown", filename);
                }
            }
            else
            {
                /* shall we crypt the file? */
                switch(iocrypt_mode)
                {
                    case CRYPT_MODE_SYMMETRIC:
                        if(iocrypt_key)
                        {
                            trace_write(iocrypt_trace_ctx, "   ->> File '%s' will get encrypted with symmetric cipher", filename);
                            if(!iocrypt_sym_init(fd))
                            {
                                trace_write(iocrypt_trace_ctx, "   FAILED TO SET UP ENCRYPTION");
                                beep();
                                NotifyBox(2000, "Some error happened, not encrypting!");
                                break;
                            }
                        }
                        else
                        {
                            beep();
                            NotifyBox(2000, "No key entered, not encrypting!");
                        }
                        break;
                        
                    case CRYPT_MODE_ASYMMETRIC:
                    case CRYPT_MODE_ASYMMETRIC_PARANOID:
                        trace_write(iocrypt_trace_ctx, "   ->> File '%s' will get encrypted with asymmetric cipher", filename);
                        if(!iocrypt_asym_init(fd))
                        {
                            trace_write(iocrypt_trace_ctx, "   FAILED TO SET UP ENCRYPTION");
                            beep();
                            NotifyBox(2000, "RSA setup failed!");
                            break;
                        }
                        break;
                        
                    default:
                        break;
                }
            }
        }

        /* log the encryption status */
        if(iocrypt_files[fd].crypt_ctx.priv)
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
    if(!iocrypt_enabled)
    {
        return orig_iodev->ReadFile(fd, buf, length);
    }
    
    uint32_t fd_pos = iodev_GetPosition(fd);
    
    /* when there is some encryption active, handle file offset */
    if(iocrypt_files[fd].crypt_ctx.priv)
    {
        iodev_SetPosition(fd, iodev_GetPosition(fd) + iocrypt_files[fd].header_size);
    }
    
    uint32_t ret = orig_iodev->ReadFile(fd, buf, length);
    
    /* when there is some encryption active, handle file offset */
    if(iocrypt_files[fd].crypt_ctx.priv)
    {
        /* if reading was beyond file end, provide zeroes */
        if(ret < length)
        {
            memset(&buf[ret], 0x00, length - ret);
            ret = MIN(length, iocrypt_files[fd].file_size - fd_pos);
            iodev_SetPosition(fd, fd_pos + ret);
        }
       
        iodev_SetPosition(fd, iodev_GetPosition(fd) - iocrypt_files[fd].header_size);
    }
    
    if(fd < COUNT(iocrypt_files))
    {
        trace_write(iocrypt_trace_ctx, "iodev_ReadFile(0x%08X, 0x%08X) -> %s, fd = %d, pos_before = 0x%08X, ret %d, pos_after %d", buf, length, iocrypt_files[fd].filename, fd, fd_pos, iocrypt_files[fd].filename, ret, iodev_GetPosition(fd));
        
        if(iocrypt_files[fd].crypt_ctx.priv)
        {
            /* let the data being encrypted asynchronously */
            volatile iocrypt_job_t job;
            
            job.type = CRYPT_JOB_DECRYPT;
            job.fd = fd;
            job.semaphore = iocrypt_files[fd].semaphore;
            job.ctx = &iocrypt_files[fd].crypt_ctx;
            job.dst = buf;
            job.buf = buf;
            job.length = length;
            job.fd_pos = fd_pos;
            
            trace_write(iocrypt_trace_ctx, "iodev_ReadFile: decrypt");
            msg_queue_post(iocrypt_msgs, &job);
            
            /* wait until worker finished */
            take_semaphore(job.semaphore, 0);
            trace_write(iocrypt_trace_ctx, "iodev_ReadFile: decrypt done");
        }
    }
    
    return ret;
}

static uint32_t hook_iodev_WriteFile(uint32_t fd, uint8_t *buf, uint32_t length)
{
    uint32_t ret = 0;
    
    if(fd < COUNT(iocrypt_files) && iocrypt_enabled)
    {
        uint32_t misalign = ((uint32_t)buf) % 8;
        uint32_t fd_pos = iodev_GetPosition(fd);
        
        if(iocrypt_files[fd].crypt_ctx.priv)
        {
            trace_write(iocrypt_trace_ctx, "iodev_WriteFile pre(0x%08X, 0x%08X) -> %s, fd = %d, fd_pos = 0x%08X, misalign = %d", buf, length, iocrypt_files[fd].filename, fd, fd_pos, misalign);

            /* when there is some encryption active, handle file offset */
            iodev_SetPosition(fd, iodev_GetPosition(fd) + iocrypt_files[fd].header_size);
            
            /* let the data being encrypted asynchronously */
            volatile iocrypt_job_t job;
            
            job.type = CRYPT_JOB_ENCRYPT_WRITE;
            job.fd = fd;
            job.semaphore = iocrypt_files[fd].semaphore;
            job.ctx = &iocrypt_files[fd].crypt_ctx;
            job.buf = buf;
            job.length = length;
            job.fd_pos = fd_pos;
            
            trace_write(iocrypt_trace_ctx, "iodev_WriteFile: encrypt");
            msg_queue_post(iocrypt_msgs, &job);
            
            /* wait until worker finished */
            take_semaphore(job.semaphore, 0);
            trace_write(iocrypt_trace_ctx, "iodev_WriteFile: encrypt done");
            
            /* when there is some encryption active, handle file offset */
            iodev_SetPosition(fd, iodev_GetPosition(fd) - iocrypt_files[fd].header_size);
        
            trace_write(iocrypt_trace_ctx, "iodev_WriteFile post(0x%08X, 0x%08X) -> fd = %d, fd_pos = 0x%08X, fd_pos (now) = 0x%08X", buf, length, fd, fd_pos, iodev_GetPosition(fd));
            
            return job.ret;
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
        iocrypt_key = 0;
        NotifyBox(2000, "IME error, Crypto disabled");
    }
}

static void iocrypt_speed_test_write(char *file, uint32_t blocksize, uint32_t loops)
{
    char filename[32];
    uint8_t *buffer = malloc(blocksize);
    
    if(!buffer)
    {
        return;
    }
    memset(buffer, 0x5A, blocksize);
    
    snprintf(filename, sizeof(filename), "%s/%s", get_dcim_dir(), file);
    FILE* f = FIO_CreateFileEx(filename);
    if(f == INVALID_PTR)
    {
        free(buffer);
        return;
    }
    
    for(uint32_t loop = 0; loop < loops; loop++)
    {
        FIO_WriteFile(f, buffer, blocksize);
    }
    
    FIO_CloseFile(f);
    free(buffer);
}

static void iocrypt_speed_test_read(char *file, uint32_t blocksize)
{
    char filename[32];
    uint8_t *buffer = malloc(blocksize);
    
    if(!buffer)
    {
        return;
    }
    
    snprintf(filename, sizeof(filename), "%s/%s", get_dcim_dir(), file);
    
    FILE* f = FIO_Open(filename, O_RDONLY | O_SYNC);
    if(f == INVALID_PTR)
    {
        free(buffer);
        return;
    }
    
    while(FIO_ReadFile(f, buffer, blocksize) == (int)blocksize)
    {
    }
    
    FIO_CloseFile(f);
    free(buffer);
}

static void iocrypt_speed_test()
{
    uint32_t unset = 0;
    uint32_t loops = 10;
    uint32_t blocks = 20;
    uint32_t blocksize = 1*1024*1024;
    gui_stop_menu();
    msleep(500);
    

    hash_password("Speed test password", &iocrypt_key);

    bmp_printf(FONT_MED, 10, 30, "Starting benchmark");
    for(uint32_t loop = 0; loop < loops; loop++)
    {
        uint32_t start = 0;
        uint32_t delta = 0;
        uint32_t speed = 0;
        
        start = get_ms_clock_value();
        iocrypt_speed_test_write("IO_CRYPT.CR2", blocksize, blocks);
        delta = get_ms_clock_value() - start;
        speed = (blocksize / 1024) * blocks * 1000 * 10 / delta;
        trace_write(iocrypt_trace_ctx, "iocrypt_speed_test: [crypted] write %d ms, %d.%02d MB/s", delta, speed/10, speed % 10);
        bmp_printf(FONT_MED, 10, 60 + (4 * loop + 0) * font_med.height, "[crypted] write %d.%02d MB/s     ", speed/10, speed % 10);
        
        start = get_ms_clock_value();
        iocrypt_speed_test_read("IO_CRYPT.CR2", blocksize);
        delta = get_ms_clock_value() - start;
        speed = (blocksize / 1024) * blocks * 1000 * 10 / delta;
        trace_write(iocrypt_trace_ctx, "iocrypt_speed_test: [crypted] read %d ms, %d.%02d MB/s", delta, speed/10, speed % 10);
        bmp_printf(FONT_MED, 10, 60 + (4 * loop + 1) * font_med.height, "[crypted] read %d.%02d MB/s      ", speed/10, speed % 10);

        start = get_ms_clock_value();
        iocrypt_speed_test_write("IO_CRYPT.DAT", blocksize, blocks);
        delta = get_ms_clock_value() - start;
        speed = (blocksize / 1024) * blocks * 1000 * 10 / delta;
        trace_write(iocrypt_trace_ctx, "iocrypt_speed_test:   [plain] write %d ms, %d.%02d MB/s", delta, speed/10, speed % 10);
        bmp_printf(FONT_MED, 10, 60 + (4 * loop + 2) * font_med.height, "[plain] write %d.%02d MB/s      ", speed/10, speed % 10);
        start = get_ms_clock_value();
        iocrypt_speed_test_read("IO_CRYPT.DAT", blocksize);
        delta = get_ms_clock_value() - start;
        speed = (blocksize / 1024) * blocks * 1000 * 10 / delta;
        trace_write(iocrypt_trace_ctx, "iocrypt_speed_test:   [plain] read %d ms, %d.%02d MB/s", delta, speed/10, speed % 10);
        bmp_printf(FONT_MED, 10, 60 + (4 * loop + 3) * font_med.height, "[plain] read %d.%02d MB/s     ", speed/10, speed % 10);
        
        FIO_RemoveFile("IO_CRYPT.CR2");
        FIO_RemoveFile("IO_CRYPT.DAT");
    }
    
    if(unset)
    {
        iocrypt_key = 0;
    }
    
    bmp_printf(FONT_LARGE, 0, 60, "Test done");
    beep();
}

void iocrypt_rsa_key_gen()
{
    NotifyBox(600000, "Generating RSA key.\nThis takes a while!");
    
    iocrypt_rsa_key_active = 1;
    crypt_rsa_generate_keys(iocrypt_rsa_ctx.priv);
    iocrypt_rsa_key_active = 0;
    
    NotifyBoxHide();
    beep();
    NotifyBox(2000, "RSA key generated!");
}

static MENU_SELECT_FUNC(iocrypt_enter_pw_select)
{
    iocrypt_enter_pw();
}

static MENU_SELECT_FUNC(iocrypt_rsa_key_select)
{
    crypt_rsa_set_keysize(512 << iocrypt_rsa_key_size);
    
    task_create("crypt_rsa_generate_keys", 0x1e, 0x1000, iocrypt_rsa_key_gen, NULL);
}

static MENU_SELECT_FUNC(iocrypt_test_rsa)
{
    task_create("rsa_test", 0x1e, 0x1000, crypt_rsa_test, (void*)0);
}

static MENU_SELECT_FUNC(iocrypt_test_speed)
{
    task_create("speed_test", 0x1e, 0x1000, iocrypt_speed_test, (void*)0);
}

static MENU_UPDATE_FUNC(iocrypt_update)
{
    if(!iocrypt_enabled)
    {
        MENU_SET_VALUE("OFF");
        return;
    }
    
    switch(iocrypt_mode)
    {
        case CRYPT_MODE_SYMMETRIC:
            if(!iocrypt_key)
            {
                MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Please set a password first. Files are not encrypted yet.");
            }
            MENU_SET_VALUE("ON, Password");
            break;
            
        case CRYPT_MODE_ASYMMETRIC:
        case CRYPT_MODE_ASYMMETRIC_PARANOID:
            if(!crypt_rsa_get_pub(iocrypt_rsa_ctx.priv))
            {
                MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Please create keys first. Files are not encrypted yet.");
            }
            else if(crypt_rsa_get_priv(iocrypt_rsa_ctx.priv))
            {
                MENU_SET_WARNING(MENU_WARN_ADVICE, "Move IO_CRYPT.KEY to a safe place and DELETE from card.");
            }
            
            MENU_SET_VALUE("ON, RSA");
            break;
    }
    
    if(iocrypt_rsa_key_active)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Still generating the RSA keys.");
    }
}


static MENU_UPDATE_FUNC(iocrypt_rsa_key_update)
{
    if(iocrypt_rsa_key_active)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Still generating the RSA keys.");
        MENU_SET_VALUE("Generating, %d %%", crypt_rsa_get_keyprogress(iocrypt_rsa_ctx.priv));
    }
}

static struct menu_entry iocrypt_menus[] =
{
    {
        .name = "Encryption",
        .update = &iocrypt_update,
        .priv = &iocrypt_enabled,
        .max = 1,
        .submenu_width = 710,
        .children = (struct menu_entry[]) {
            {
                .name = "Encryption mode",
                .priv = &iocrypt_mode,
                .max = 1, /* the others are not implemented yet */
                .icon_type = IT_DICE,
                .choices = (const char *[]) {"Password", "RSA", "RSA (paranoid)", "Background PW", "Background RSA"},
                .help = "Select the encryption mode. The higher the level, the less comfort you have.",
            },
            /*
            {
                .name = "Show fake images",
                .priv = &iocrypt_fake,
                .max = 3,
                .choices = (const char *[]) {"OFF", "Wrong Password", "All Encrypted", "All Images"},
                .help = "Which images should get replaced by fake images.",
            },
            */
            {
                .name = "Set password",
                .select = &iocrypt_enter_pw_select,
                .priv = NULL,
                .icon_type = IT_ACTION,
            },
            {
                .name = "Blocksize",
                .priv = &iocrypt_block_size,
                .max = 6,
                .icon_type = IT_DICE,
                .choices = (const char *[]) {"8k", "16k", "32k", "64k", "128k", "256k", "512k"},
                .help = "Blocks get encrypted with the same 64 bit key. The smaller the more secure but slower.",
            },
            {
                .name = "Ask for password on startup",
                .priv = &iocrypt_ask_pass,
                .max = 1,
                .help = "When enabled it will ask for the encryption password right after camera powerup.",
            },
            {
                .name = "Create RSA Key",
                .select = &iocrypt_rsa_key_select,
                .update = &iocrypt_rsa_key_update,
                .priv = NULL,
                .icon_type = IT_ACTION,
                .help = "Do this ONCE at HOME and then store /priv.key on your PC safely.",
            },
            {
                .name = "RSA Keysize",
                .priv = &iocrypt_rsa_key_size,
                .max = 3,
                .icon_type = IT_DICE,
                .choices = (const char *[]) {"512", "1024", "2048", "4096"},
                .help = "Key size when creating a RSA key pair. The smaller, the less security you have.",
            },
            {
                .name = "Test: Speed",
                .select = &iocrypt_test_speed,
                .priv = NULL,
                .icon_type = IT_ACTION,
            },
            {
                .name = "Test: RSA",
                .select = &iocrypt_test_rsa,
                .priv = NULL,
                .icon_type = IT_ACTION,
            },
            MENU_EOL,
        },
    },
};

static void iocrypt_task()
{
    while(!iocrypt_shutdown && !ml_shutdown_requested)
    {
        int timeout = 500;
        iocrypt_job_t *job = NULL;

        /* fetch a new encryption job */
        if(msg_queue_receive(iocrypt_msgs, &job, timeout))
        {
            continue;
        }
        
        switch(job->type)
        {
            /* combined encrypt and write job which is atomic */
            case CRYPT_JOB_ENCRYPT_WRITE:
            {
                trace_write(iocrypt_trace_ctx, "   ->> ENCRYPT");
                job->ctx->encrypt(job->ctx->priv, iocrypt_scratch, job->buf, job->length, job->fd_pos);
                trace_write(iocrypt_trace_ctx, "   ->> WRITE");
                job->ret = orig_iodev->WriteFile(job->fd, iocrypt_scratch, job->length);
                trace_write(iocrypt_trace_ctx, "   ->> DONE");
                
                give_semaphore(job->semaphore);
                break;
            }
            
            /* simple decryption */
            case CRYPT_JOB_ENCRYPT:
            {
                trace_write(iocrypt_trace_ctx, "   ->> ENCRYPT");
                job->ret = job->ctx->encrypt(job->ctx->priv, job->dst, job->buf, job->length, job->fd_pos);
                trace_write(iocrypt_trace_ctx, "   ->> DONE (%d)", job->ret);
                
                give_semaphore(job->semaphore);
                break;
            }
            
            /* simple decryption */
            case CRYPT_JOB_DECRYPT:
            {
                trace_write(iocrypt_trace_ctx, "   ->> DECRYPT");
                job->ret = job->ctx->decrypt(job->ctx->priv, job->dst, job->buf, job->length, job->fd_pos);
                trace_write(iocrypt_trace_ctx, "   ->> DONE");
                
                give_semaphore(job->semaphore);
                break;
            }
        }
    }
    
    iocrypt_shutdown = 0;
}

static unsigned int iocrypt_init()
{
    /* for debugging */
    if(1)
    {
        char filename[32] = "IO_CRYPT.TXT";
        
        iocrypt_trace_ctx = trace_start("debug", filename);
        trace_set_flushrate(iocrypt_trace_ctx, 1000);
        trace_format(iocrypt_trace_ctx, TRACE_FMT_TIME_REL | TRACE_FMT_COMMENT, ' ');
        trace_write(iocrypt_trace_ctx, "io_crypt: Starting trace");
    }

    /* clear file map */
    for(int pos = 0; pos < COUNT(iocrypt_files); pos++)
    {
        iocrypt_files[pos].crypt_ctx.priv = NULL;
        iocrypt_files[pos].semaphore = create_named_semaphore("iocrypt_pw", 0);
    }
    
    if(is_camera("600D", "1.0.2"))
    {
        trace_write(iocrypt_trace_ctx, "io_crypt: Detected 600D");
        iodev_table = 0x1E684;
        iodev_ctx = 0x7EB08;
        iodev_ctx_size = 0x18;
    }
    else if(is_camera("7D", "2.0.3"))
    {
        trace_write(iocrypt_trace_ctx, "io_crypt: Detected 7D");
        iodev_table = 0x2D3B8;
        iodev_ctx = 0x85510;
        iodev_ctx_size = 0x18;
    }
    else if(is_camera("60D", "1.1.1"))
    {
        trace_write(iocrypt_trace_ctx, "io_crypt: Detected 60D");
        iodev_table = 0x3E2BC;
        iodev_ctx = 0x5CB38;
        iodev_ctx_size = 0x18;
    }
    else if(is_camera("5D3", "1.1.3"))
    {
        trace_write(iocrypt_trace_ctx, "io_crypt: Detected 5D3");
        iodev_table = 0x44FA8;
        iodev_ctx = 0x67140;
        iodev_ctx_size = 0x20;
    }
    /*
    else if(is_camera("650D", "1.0.4"))
    {
        trace_write(iocrypt_trace_ctx, "io_crypt: Detected 650D");
        iodev_table = 0x54060;
        iodev_ctx = 0x7C278;
        iodev_ctx_size = 0x20;
    }
    else if(is_camera("50D", "1.0.9"))
    {
        trace_write(iocrypt_trace_ctx, "io_crypt: Detected 50D");
        iodev_table = 0x1F208;
        iodev_ctx = 0x49A64;
        iodev_ctx_size = 0x18;
    }
    */
    else
    {
        NotifyBox(2000, "io_crypt: Camera unsupported");
        return CBR_RET_ERROR;
    }
    
    iocrypt_password_sem = create_named_semaphore("iocrypt_pw", 1);
    
    /* ask for the initial password */
    if(iocrypt_ask_pass && iocrypt_enabled && (iocrypt_mode == CRYPT_MODE_SYMMETRIC || iocrypt_mode == CRYPT_MODE_BACKGROUND_SYM))
    {
        trace_write(iocrypt_trace_ctx, "io_crypt: Asking for password");
        iocrypt_enter_pw();
    }
    
    /* this memory is used for buffering encryption, so we dont have to undo the changes in memory */
    iocrypt_scratch = shoot_malloc(CRYPT_SCRATCH_SIZE);
    
    /* now patch the iodev handlers */
    orig_iodev = (iodev_handlers_t *)MEM(iodev_table);
    hook_iodev = *orig_iodev;
    hook_iodev.OpenFile = &hook_iodev_OpenFile;
    hook_iodev.ReadFile = &hook_iodev_ReadFile;
    hook_iodev.WriteFile = &hook_iodev_WriteFile;
    hook_iodev.CloseFile = &hook_iodev_CloseFile;
    MEM(iodev_table) = (uint32_t)&hook_iodev;
    
    /* create a message queue for processing crypt tasks asyncrhonously */
    iocrypt_msgs = (struct msg_queue *) msg_queue_create("iocrypt_msgs", 100);
    
    crypt_rsa_init(&iocrypt_rsa_ctx);
    
    task_create("iocrypt_task", 0x1A, 0x1000, iocrypt_task, (void*)0);
    
    /* any file operation is routed through us now */
    menu_add("Shoot", iocrypt_menus, COUNT(iocrypt_menus) );
    
    return 0;
}

static unsigned int iocrypt_deinit()
{
    iocrypt_shutdown = 1;
    
    while(iocrypt_shutdown && !ml_shutdown_requested)
    {
        msleep(20);
    }
    
    MEM(iodev_table) = (uint32_t)orig_iodev;
    if(iocrypt_scratch)
    {
        shoot_free(iocrypt_scratch);
    }
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(iocrypt_init)
    MODULE_DEINIT(iocrypt_deinit)
MODULE_INFO_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(iocrypt_enabled)
    MODULE_CONFIG(iocrypt_mode)
    MODULE_CONFIG(iocrypt_block_size)
    MODULE_CONFIG(iocrypt_ask_pass)
    MODULE_CONFIG(iocrypt_rsa_key_size)
MODULE_CONFIGS_END()
