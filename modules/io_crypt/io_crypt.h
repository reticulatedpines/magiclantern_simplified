
#ifndef __IO_CRYPT_H
#define __IO_CRYPT_H

#define CRYPT_SCRATCH_SIZE 0x00800000
typedef struct crypt_cipher_t
{
    void *priv;
    uint32_t (*encrypt)(void *ctx, uint8_t *dst, uint8_t *src, uint32_t length, uint32_t offset);
    uint32_t (*decrypt)(void *ctx, uint8_t *dst, uint8_t *src, uint32_t length, uint32_t offset);
    void (*reset)(void *ctx);
    void (*deinit)(void *ctx);
    void (*set_blocksize)(void *ctx, uint32_t size);
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
