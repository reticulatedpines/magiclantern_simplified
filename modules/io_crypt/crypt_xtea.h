#ifndef __CRYPT_XTEA_H
#define __CRYPT_XTEA_H


typedef struct
{
    uint32_t password[4];
    uint64_t nonce;
} xtea_ctx_t;

typedef struct
{
    uint64_t nonce;
} xtea_data_t;

void crypt_xtea_init(crypt_cipher_t *crypt_ctx, uint32_t password[4], uint64_t nonce);

#endif

