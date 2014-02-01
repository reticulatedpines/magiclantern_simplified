#ifdef MODULE

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#define TRACE_DISABLED
#include "../trace/trace.h"

#else

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define trace_write(x,...) do { 0; } while (0)
//#define trace_write(x,...) do { printf(__VA_ARGS__); printf("\n"); } while (0)

#endif

#include "io_crypt.h"
#include "crypt_lfsr64.h"

/*  when write speed is about 70MiB/s unencrypted:
        blocksize 0x00002000: 2MiB/s
        blocksize 0x00010000: 5MiB/s
        blocksize 0x00020000: 20MiB/s
    the larger the block size is, the easier it is to reconstruct the current block key
        */
static uint32_t crypt_lfsr64_blocksize = 0x00020000;

extern uint32_t iocrypt_trace_ctx;

/* clock the current LFSR by given number of clocks */
static void crypt_lfsr64_clock(lfsr64_ctx_t *ctx, uint32_t clocks)
{
    uint64_t lfsr = ctx->lfsr_state;
    
    for(uint32_t clock = 0; clock < clocks; clock++)
    {
        /* maximum length LFSR according to http://www.xilinx.com/support/documentation/application_notes/xapp052.pdf */
        int bit = ((lfsr >> 63) ^ (lfsr >> 62) ^ (lfsr >> 60) ^ (lfsr >> 59)) & 1;
        lfsr = (lfsr << 1) | bit;
    }
    
    ctx->lfsr_state = lfsr;
}

/* xor the buffer with 8/64 bit alignment */
static void crypt_lfsr64_xor_uint8(void *dst_in, void *src_in, lfsr64_ctx_t *ctx, uint32_t offset)
{
    uint8_t *dst = (uint8_t *)dst_in;
    uint8_t *src = (uint8_t *)src_in;
    
    *dst = *src ^ ctx->key_uint8[offset % 8];
}

static void crypt_lfsr64_xor_uint64(void *dst_in, void *src_in, lfsr64_ctx_t *ctx, uint32_t offset)
{
    uint64_t *dst = (uint64_t *)dst_in;
    uint64_t *src = (uint64_t *)src_in;
    
    *dst = *src ^ ctx->key_uint64[offset % 8];
}

static void update_key(lfsr64_ctx_t *ctx, uint32_t offset, uint32_t force)
{
    /* update the current encryption key whever reaching the next block */
    uint32_t block = offset / crypt_lfsr64_blocksize;
    
    if(!force)
    {
        if(ctx->current_block == block)
        {
            return;
        }
    }
    
    ctx->current_block = block;
    
    /* first initialize LFSR with base key and block */
    ctx->lfsr_state = ctx->password ^ block;
    crypt_lfsr64_clock(ctx, 111);
    
    /* then update it with the file offset again and shift by an amount based on file offset */
    ctx->lfsr_state ^= block;
    crypt_lfsr64_clock(ctx, (11 * block) % 16);
    
    /* mask it again */
    ctx->lfsr_state ^= ctx->password;
    
    trace_write(iocrypt_trace_ctx, "update_key: offset 0x%08X, password: 0x%08X%08X, key: 0x%08X%08X", offset, (uint32_t)(ctx->password>>32), (uint32_t)ctx->password, (uint32_t)(ctx->lfsr_state>>32), (uint32_t)ctx->lfsr_state);
    
    /* build an array with key elements for every offset in uint8_t mode */
    uint64_t lfsr = ctx->lfsr_state;
    for(int pos = 0; pos < 8; pos++)
    {
        ctx->key_uint8[pos] = (uint8_t)(lfsr & 0xFF);
        lfsr >>= 8;
    }
    
    /* build an array with key elements for every offset in uint64_t mode */
    for(int pos = 0; pos < 8; pos++)
    {
        uint32_t elem_addr = (uint32_t)&ctx->key_uint64[pos];
        
        memcpy((void*)elem_addr, &ctx->key_uint8[pos], 8 - pos);
        memcpy((void*)elem_addr + (8-pos), ctx->key_uint8, pos);
    }
}

#define IS_UNALIGNED(x) ((((uint32_t)dst) % (x)) || (((uint32_t)src) % (x)))
static uint32_t crypt_lfsr64_encrypt(crypt_cipher_t *cipher_ctx, uint8_t *dst, uint8_t *src, uint32_t length, uint32_t offset)
{
    lfsr64_ctx_t *ctx = cipher_ctx->priv;
    
    /* initial key creation */
    update_key(ctx, offset, 0); 
    
    /* try to get the addresses aligned */
    //trace_write(iocrypt_trace_ctx, "crypt_lfsr64_xor_uint8 offset 0x%08X, length 0x%08X", offset, length);
    while((IS_UNALIGNED(8) || (offset % 8)) && (length > 0))
    {
        uint32_t block_remain = offset % crypt_lfsr64_blocksize;
        
        if(block_remain == 0)
        {
            update_key(ctx, offset, 0);
        }
        
        crypt_lfsr64_xor_uint8(dst, src, ctx, offset);
        dst += 1;
        src += 1;
        offset += 1;
        length -= 1;
    }
    
    //trace_write(iocrypt_trace_ctx, "crypt_lfsr64_xor_uint64 offset 0x%08X, length 0x%08X", offset, length);
    if(!IS_UNALIGNED(8))
    {
        /* processing loop for 64 bit writes and even file offsets */
        while(length >= 8 && ((offset % 8) == 0))
        {
            if((offset % crypt_lfsr64_blocksize) == 0)
            {
                update_key(ctx, offset, 0);
            }
            
            crypt_lfsr64_xor_uint64(dst, src, ctx, 0);
            dst += 8;
            src += 8;
            offset += 8;
            length -= 8;
        }
        
        /* processing loop for 64 bit writes and odd file offsets */
        while(length >= 8)
        {
            uint32_t block_remain = offset % crypt_lfsr64_blocksize;
            
            if(block_remain == 0)
            {
                update_key(ctx, offset, 0);
            }
            else if(block_remain < 8)
            {
                break;
            }
            
            crypt_lfsr64_xor_uint64(dst, src, ctx, offset);
            dst += 8;
            src += 8;
            offset += 8;
            length -= 8;
        }
    }
    
    /* do the rest */
    //trace_write(iocrypt_trace_ctx, "crypt_lfsr64_xor_uint8 offset 0x%08X, length 0x%08X", offset, length);
    while(length > 0)
    {
        uint32_t block_remain = offset % crypt_lfsr64_blocksize;
        
        if(block_remain == 0)
        {
            update_key(ctx, offset, 0);
        }
        
        crypt_lfsr64_xor_uint8(dst, src, ctx, offset);
        trace_write(iocrypt_trace_ctx, "crypt_lfsr64_xor_uint8 offset 0x%08X, 0x%02X <- 0x%02X", offset, *dst, *src);
        dst += 1;
        src += 1;
        offset += 1;
        length -= 1;
    }
}
#undef IS_UNALIGNED

/* using a symmetric cipher, both encryption and decryption are the same */
static uint32_t crypt_lfsr64_decrypt(crypt_cipher_t *ctx, uint8_t *dst, uint8_t *src, uint32_t length, uint32_t offset)
{
    return crypt_lfsr64_encrypt(ctx, dst, src, length, offset);
}

static void crypt_lfsr64_set_blocksize(crypt_cipher_t *crypt_ctx, uint32_t size)
{
    crypt_lfsr64_blocksize = size;
}

static void crypt_lfsr64_deinit(crypt_cipher_t *crypt_ctx)
{
    if(crypt_ctx && crypt_ctx->priv)
    {
        free(crypt_ctx->priv);
        crypt_ctx->priv = NULL;
    }
}

/* allocate and initialize an LFSR64 cipher ctx and save to pointer */
void crypt_lfsr64_init(crypt_cipher_t *crypt_ctx, uint64_t password)
{
    lfsr64_ctx_t *ctx = malloc(sizeof(lfsr64_ctx_t));
    
    if(!ctx)
    {
        trace_write(iocrypt_trace_ctx, "crypt_lfsr64_init: failed to malloc");
        return;
    }
    
    /* setup cipher ctx */
    crypt_ctx->encrypt = &crypt_lfsr64_encrypt;
    crypt_ctx->decrypt = &crypt_lfsr64_decrypt;
    crypt_ctx->deinit = &crypt_lfsr64_deinit;
    crypt_ctx->set_blocksize = &crypt_lfsr64_set_blocksize;
    
    crypt_ctx->priv = ctx;
    ctx->password = password;
    ctx->current_block = 0xFFFFFFFF;
    ctx->lfsr_state = 0;
    
    /* setup initial cipher key */
    update_key(ctx, 0, 1); 
    
    trace_write(iocrypt_trace_ctx, "crypt_lfsr64_init: initialized");
}


