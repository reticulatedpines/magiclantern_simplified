
#ifndef __CRYPT_LFSR_H
#define __CRYPT_LFSR_H

typedef struct
{
    crypt_cipher_t cipher;
    uint64_t lfsr_state;
    uint64_t key;
    uint64_t password;
    uint32_t current_offset;
} lfsr64_ctx_t;


void crypt_lfsr64_init(void **crypt_ctx, uint64_t password);

#endif
