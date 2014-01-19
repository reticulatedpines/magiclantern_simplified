#ifndef __CRYPT_LFSR_H
#define __CRYPT_LFSR_H

#define CRYPT_LFSR64_BLOCKSIZE    0x00002000

typedef struct
{
    crypt_cipher_t cipher;
    uint64_t lfsr_state;
    uint8_t key_uint8[8];
    uint64_t key_uint64[8];
    uint64_t password;
    uint32_t current_block;
} lfsr64_ctx_t;

void crypt_lfsr64_init(void **crypt_ctx, uint64_t password);

#endif
