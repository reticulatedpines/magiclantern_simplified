
#ifdef MODULE

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

#include "../trace/trace.h"

#else

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define msleep(x) usleep((x)*1000)

#if defined(TRACE_DISABLED)
#define trace_write(x,...) do { (void)0; } while (0)
#else
#define trace_write(x,...) do { printf(__VA_ARGS__); printf("\n"); } while (0)
#endif

#endif

#include "io_crypt.h"
#include "crypt_xtea.h"

extern uint32_t iocrypt_trace_ctx;
 
/* XTEA block cipher, encryption function*/
/* take 64 bits of data in v[0] and v[1] and 128 bits of key[0] - key[3] */
void xtea_crypt_block(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4])
{
    uint32_t i = 0;
    uint32_t v0 = v[0];
    uint32_t v1 = v[1];
    uint32_t sum = 0;
    uint32_t delta = 0x9E3779B9;
    
    for (i=0; i < num_rounds; i++)
    {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
    }
    v[0]=v0;
    v[1]=v1;
}
 
/* XTEA block cipher, decryption function (not used here, ctr-mode only needs encrypt */
void xtea_decrypt_block(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4])
{
    uint32_t i;
    uint32_t v0 = v[0];
    uint32_t v1 = v[1];
    uint32_t delta = 0x9E3779B9;
    uint32_t sum = delta * num_rounds;
    
    for (i=0; i < num_rounds; i++)
    {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
        sum -= delta;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    }
    v[0]=v0; 
    v[1]=v1;
}

/* CTR-mode */
void ctr_xtea_block(uint32_t buf[2], uint32_t const key[4], uint64_t nonce, uint32_t counter)
{
    uint32_t ctr_block[2] = {nonce >> 32, (uint32_t) (counter ^ nonce)};
    xtea_crypt_block(64, ctr_block, key);
    buf[0] ^= ctr_block[0];
    buf[1] ^= ctr_block[1];
}

/* CTR-mode for partial (i.e. smaller that 8-byte) blocks */
void ctr_xtea_block_partial(uint8_t* buf, uint32_t const key[4], uint64_t nonce, uint32_t counter, uint32_t start, uint32_t len)
{
    uint32_t ctr_block[2] = {nonce >> 32, (uint32_t) (counter ^ nonce)};
    
    xtea_crypt_block(64, ctr_block, key);
    
    for(uint32_t i = 0; i < len; i++)
    {
        if(i + start < 4)
        {
            buf[i] ^= ((uint8_t*) &ctr_block[0])[start+i];
        }
        else
        {
            buf[i] ^= ((uint8_t*) &ctr_block[1])[start+i-4];
        }
    }
}

static uint32_t crypt_xtea_encrypt(crypt_priv_t *priv, uint8_t *dst_in, uint8_t *src_in, uint32_t in_length, uint32_t offset)
{
    xtea_ctx_t* ctx = (xtea_ctx_t*) priv;
    uint32_t block_offset = (offset >> 3);
    uint32_t remain = offset % 8;
    uint32_t remain_len = in_length;
    uint32_t length = in_length;

    // in-place encryption. If buffers are not equal, copy encrypted block to dest
    if(dst_in != src_in)
    {
        memcpy(dst_in, src_in, length);
    }

    // if not 8-byte aligned, decrypt partial to next 8-byte boundary
    if(remain)
    {
        if(remain_len > 8 - remain)
        {
            remain_len = 8 - remain;
        }
        ctr_xtea_block_partial(dst_in, ctx->password, ctx->nonce, block_offset, remain, remain_len);
        length -= remain_len;
        dst_in += remain_len;
        block_offset++;
    }

    // decrypt whole 8-byte-blocks as long as possible
    while(length >= 8)
    {
        ctr_xtea_block((uint32_t*) dst_in, ctx->password, ctx->nonce, block_offset);
        dst_in += 8;
        length -= 8;
        block_offset++;
    }

    // decrypt rest
    if(length)
    {
        ctr_xtea_block_partial(dst_in, ctx->password, ctx->nonce, block_offset, 0, length);
    }

    return in_length;

}

/* CTR-mode is symmetric -> encrypt = decrypt */
static uint32_t crypt_xtea_decrypt(crypt_priv_t *priv, uint8_t *dst_in, uint8_t *src_in, uint32_t in_length, uint32_t offset)
{
    return crypt_xtea_encrypt(priv, dst_in, src_in, in_length, offset);
}

/* free ctx */
static void crypt_xtea_deinit(crypt_priv_t *priv)
{
    xtea_ctx_t *ctx = (xtea_ctx_t *)priv;
    
    if(ctx)
    {
        free(ctx);
    }
}

/* not needed */
static void crypt_xtea_set_blocksize(crypt_priv_t *priv, uint32_t size)
{
}

/* not needed */
static void crypt_xtea_reset(crypt_priv_t *priv)
{
}

/* allocate ctx */
void crypt_xtea_init(crypt_cipher_t *crypt_ctx, uint32_t password[4], uint64_t nonce)
{
    xtea_ctx_t* ctx = malloc(sizeof(xtea_ctx_t));
    
    if(!ctx)
    {
        trace_write(iocrypt_trace_ctx, "crypt_xtea_init: failed to malloc");
        return;
    }
    
    /* setup cipher ctx */
    crypt_ctx->encrypt = &crypt_xtea_encrypt;
    crypt_ctx->decrypt = &crypt_xtea_decrypt;
    crypt_ctx->deinit = &crypt_xtea_deinit;
    crypt_ctx->reset = &crypt_xtea_reset;
    crypt_ctx->set_blocksize = &crypt_xtea_set_blocksize;
    crypt_ctx->priv = ctx;
    
    memcpy(ctx->password, password, 16);
    ctx->nonce = nonce;
}
