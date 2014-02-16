
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "io_crypt.h"
#include "crypt_lfsr64.h"
#include "crypt_rsa.h"
#include "hash_password.h"


#define BLOCKSIZE (8 * 1024)


static const uint8_t cr2_magic[] = "\x49\x49\x2A\x00";
static const uint8_t jpg_magic[] = "\xff\xd8\xff\xe1";
static const uint8_t rsa_magic[] = "\xff\xd8\xff\xd8";
static const uint8_t lfsr_magic[] = "\xff\xd8\xff\x8d";


static uint32_t lfsr113[] = { 0x00009821, 0x00098722, 0x00986332, 0x961FEFA7 };

void rand_fill(uint32_t *buffer, uint32_t length)
{
    for(uint32_t pos = 0; pos < length; pos++)
    {
        lfsr113[0] = ((lfsr113[0] & 0xFFFFFFFE) << 18) ^ (((lfsr113[0] <<  6) ^ lfsr113[0]) >> 13);
        lfsr113[1] = ((lfsr113[1] & 0xFFFFFFF8) <<  2) ^ (((lfsr113[1] <<  2) ^ lfsr113[1]) >> 27);
        lfsr113[2] = ((lfsr113[2] & 0xFFFFFFF0) <<  7) ^ (((lfsr113[2] << 13) ^ lfsr113[2]) >> 21);
        lfsr113[3] = ((lfsr113[3] & 0xFFFFFF80) << 13) ^ (((lfsr113[3] <<  3) ^ lfsr113[3]) >> 12);

        buffer[pos] = lfsr113[0] ^ lfsr113[1] ^ lfsr113[2] ^ lfsr113[3];
    }
}

void rand_seed(uint32_t seed)
{
    uint32_t tmp = 0;

    /* apply seed to internal states and do a few rounds to equally distribute seed bits */
    for(int loops = 0; loops < 128; loops++)
    {
        lfsr113[loops%4] ^= seed;
        rand_fill(&tmp, 1);
    }
}

/* this routine checks if the LFSR64 encryption is handling all cases correctly */
static void io_decrypt_test()
{
    uint64_t key = 0xDEADBEEFDEADBEEF;
    uint32_t lfsr_blocksize = 0x00000224;
    uint32_t file_offset = 0;
    crypt_cipher_t crypt_ctx;

    /* initialize encryption with some common parameters */
    crypt_lfsr64_init(&crypt_ctx, key);
    crypt_ctx.set_blocksize(&crypt_ctx, lfsr_blocksize);
    
    rand_seed(0x12341234);
    
    uint32_t bufsize = 1 * 1024 * 1024;
    char *buf_src = malloc(bufsize);
    char *buf_dst = malloc(bufsize);
    
    for(int loop = 0; loop < 1000; loop++)
    {
        /* prepare both buffers */
        rand_fill(buf_src, bufsize / 4);
        memcpy(buf_dst, buf_src, bufsize);
        
        /* forge some test start and length */
        uint32_t start = 0;
        uint32_t length = 0;
        
        rand_fill(&start, 1);
        rand_fill(&length, 1);
        
        start %= bufsize;
        length %= (bufsize - start + 1);
        
        /* now do en- and de-cryption */
        printf("#%03d 0x%08X 0x%08X\n", loop, start, length);
        crypt_ctx.encrypt(&crypt_ctx, &buf_dst[start], &buf_src[start], length, 0);
        crypt_ctx.decrypt(&crypt_ctx, &buf_dst[start], &buf_dst[start], length, 0);
        
        /* check if both match */
        if(memcmp(buf_src, buf_dst, bufsize))
        {
            printf("  --> Check failed!!\n");
            return;
        }
    }
    
    free(buf_src);
    free(buf_dst);
}

static crypt_cipher_t iocrypt_rsa_ctx;
int main(int argc, char *argv[])
{
    //io_decrypt_test();
    //crypt_rsa_test();
    
    if(argc < 2)
    {
        printf("Usage: '%s <infile> [outfile] [password]\n", argv[0]);
        return -1;
    }
    
    uint64_t key = 0;
    uint32_t lfsr_blocksize = 0x00020000;
    
    char *in_filename = argv[1];
    char *out_filename = malloc(strlen(in_filename) + 9);
    
    sprintf(out_filename, "%s_out.cr2", in_filename);
    
    if(argc >= 3)
    {
        out_filename = strdup(argv[2]);
    }
    
    /* password is optional */
    if(argc >= 4)
    {
        /* hash the password */
        hash_password(argv[3], &key);
    }
    
    /* open files */
    FILE *in_file = fopen(in_filename, "rb");
    if(!in_file)
    {
        printf("Could not open '%s'\n", in_filename);
        return -1;
    } 
    
    char *buffer = malloc(BLOCKSIZE);
    
    
    /* try to detect file type */
    if(fread(buffer, 1, 4, in_file) != 4)
    {
        printf("Could not read '%s'\n", in_filename);
        return -1;
    }
    
    if(!memcmp(buffer, jpg_magic, 4))
    {
        printf("File type: JPEG (plain)\n");
        return 0;
    }
    else if(!memcmp(buffer, cr2_magic, 4))
    {
        printf("File type: CR2 (plain)\n");
        return 0;
    }
    else if(!memcmp(buffer, lfsr_magic, 4))
    {
        printf("File type: LFSR64\n");
        
        if(!key)
        {
            printf("Error: Please specify a password\n");
            return -2;
        }
        if(fread(&lfsr_blocksize, 1, sizeof(uint32_t), in_file) != sizeof(uint32_t))
        {
            printf("Could not read '%s'\n", in_filename);
            return -1;
        }
        
        fseek(in_file, 0x200, SEEK_SET);
    }
    else if(!memcmp(buffer, rsa_magic, 4))
    {
        printf("File type: RSA+LFSR64\n");
        
        crypt_rsa_init(&iocrypt_rsa_ctx);
        
        if(crypt_rsa_get_keysize(iocrypt_rsa_ctx.priv) < 64)
        {
            printf("Invalid key size\n");
            return;
        }

        uint32_t encrypted_size = 0;
        
        if(fread(&encrypted_size, 1, sizeof(uint32_t), in_file) != sizeof(uint32_t))
        {
            printf("Could not read '%s'\n", in_filename);
            return -1;
        }
        
        if(fread(&lfsr_blocksize, 1, sizeof(uint32_t), in_file) != sizeof(uint32_t))
        {
            printf("Could not read '%s'\n", in_filename);
            return -1;
        }

        if(!encrypted_size || encrypted_size > 32768 * 4)
        {
            printf("encrypted_size: %d\n", encrypted_size);
            return -1;
        }
        
        if(!lfsr_blocksize || lfsr_blocksize > 0x10000000)
        {
            printf("lfsr_blocksize: %d\n", lfsr_blocksize);
            return -1;
        }
        
        char *encrypted = malloc(encrypted_size);
        if(fread(encrypted, 1, encrypted_size, in_file) != encrypted_size)
        {
            printf("Could not read '%s'\n", in_filename);
            return -1;
        }
        uint32_t decrypted_size = iocrypt_rsa_ctx.decrypt(iocrypt_rsa_ctx.priv, (uint8_t *)encrypted, (uint8_t *)encrypted, encrypted_size, 0);

        if(!decrypted_size || decrypted_size > encrypted_size)
        {
            printf("decrypted_size: %d. maybe key mismatch?\n", decrypted_size);
            return -1;
        }
        
        /* that decrypted data is the file crypt key */
        memcpy(&key, encrypted, sizeof(uint64_t));
        
        free(encrypted);
    
        uint32_t used_header = 4 + sizeof(uint32_t) + encrypted_size;
        uint32_t aligned_header = (used_header + 0x1FF) & ~0x1FF;
        
        /* now skip that header and continue with LFSR113 decryption */
        fseek(in_file, aligned_header, SEEK_SET);
    }
    else
    {
        if(key)
        {
            printf("File type: unknown. assuming LFSR64\n");
        }
        else
        {
            printf("File type: unknown. assuming LFSR64. Please specify a password\n");
            return -2;
        }
    }
    
    /* setup cipher with that hash */
    uint32_t first = 1;
    FILE *out_file = NULL;
    
    uint32_t file_offset = 0;
    crypt_cipher_t crypt_ctx;
    crypt_lfsr64_init(&crypt_ctx, key);
    crypt_ctx.set_blocksize(crypt_ctx.priv, lfsr_blocksize);
    
    
    while(!feof(in_file))
    {
        int ret = fread(buffer, 1, BLOCKSIZE, in_file);
        
        if(ret > 0)
        {
            crypt_ctx.decrypt(crypt_ctx.priv, (uint8_t *)buffer, (uint8_t *)buffer, ret, file_offset);
            
            /* try to detect file type */
            if(first)
            {
                first = 0;
                
                if(!memcmp(buffer, jpg_magic, 4))
                {
                    printf("File type: JPEG (decrypted)\n");
                }
                else if(!memcmp(buffer, cr2_magic, 4))
                {
                    printf("File type: CR2 (decrypted)\n");
                }
                else
                {
                    printf("File type: unknown. invalid key?\n");
                    fclose(in_file);
                    free(out_filename);
                    return 0;
                }
                
                out_file = fopen(out_filename, "w");
                if(!out_file)
                {
                    printf("Could not open '%s'\n", out_filename);
                    return -1;
                }
            }
            
            fwrite(buffer, 1, ret, out_file);
            file_offset += ret;
        }
        if(ret < 0)
        {
            printf("Could not read '%s'\n", in_filename);
            break;
        }
    }
    
    fclose(in_file);
    fclose(out_file);
    free(out_filename);
}
