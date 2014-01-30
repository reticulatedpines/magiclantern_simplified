#ifndef __CRYPT_LFSR_H
#define __CRYPT_LFSR_H

/*  when write speed is about 70MiB/s unencrypted:
        blocksize 0x00002000: 2MiB/s
        blocksize 0x00010000: 5MiB/s
        blocksize 0x00020000: 20MiB/s
    the larger the block size is, the easier it is to reconstruct the current block key
        */
#define CRYPT_LFSR64_BLOCKSIZE    0x00020000

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
