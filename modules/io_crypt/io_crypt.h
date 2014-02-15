
#ifndef __IO_CRYPT_H
#define __IO_CRYPT_H

#define CRYPT_SCRATCH_SIZE 0x00800000


typedef void crypt_priv_t;

typedef struct crypt_cipher_t
{
    /* priv: private data for the encryption algorithm */
    crypt_priv_t *priv;
    /* encrypt: encrypt given data at *src of given length and store at *dst, returns encrypted data length */
    uint32_t (*encrypt)(crypt_priv_t *ctx, uint8_t *dst, uint8_t *src, uint32_t length, uint32_t offset);
    /* encrypt: decrypt given data at *src of given length and store at *dst, returns decrypted data length */
    uint32_t (*decrypt)(crypt_priv_t *ctx, uint8_t *dst, uint8_t *src, uint32_t length, uint32_t offset);
    /* set_blocksize: set encryption algorithm parameter blocksize */
    void (*set_blocksize)(crypt_priv_t *ctx, uint32_t size);
    /* reset: reset internal data to the state right after initialization */
    void (*reset)(crypt_priv_t *ctx);
    /* deinit: free private data */
    void (*deinit)(crypt_priv_t *ctx);
} crypt_cipher_t;

typedef struct
{
    crypt_cipher_t crypt_ctx;
    uint64_t file_key;
    uint32_t header_size;
    uint32_t file_size;
    char filename[64];
} fd_map_t;

typedef struct
{
    uint32_t (*OpenFile)(void *iodev, char *filename, int32_t flags, char *filename_);
    uint32_t (*CloseFile)(uint32_t handle);
    uint32_t (*unk1)();
    uint32_t (*ReadFile)(uint32_t handle, uint8_t *buf, uint32_t length);
    uint32_t (*WriteFile)(uint32_t handle, uint8_t *buf, uint32_t length);
    uint32_t (*unk2)();
    uint32_t (*unk3)();
    uint32_t (*unk4)();
    uint32_t (*unk5)();
} iodev_handlers_t;


#endif
