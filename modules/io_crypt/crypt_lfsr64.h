#ifndef __CRYPT_LFSR_H
#define __CRYPT_LFSR_H


typedef struct
{
    uint64_t lfsr_state;
    uint8_t key_uint8[8];
    uint64_t key_uint64[8];
    uint64_t password;
    uint32_t current_block;
    uint32_t blocksize;
} lfsr64_ctx_t;

void crypt_lfsr64_init(crypt_cipher_t *crypt_ctx, uint64_t password);

#endif
