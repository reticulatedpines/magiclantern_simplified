
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "io_crypt.h"
#include "crypt_lfsr64.h"
#include "crypt_rsa.h"
#include "hash_password.h"


#define BLOCKSIZE (8 * 1024)


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

int main(int argc, char *argv[])
{
    if(argc == 2 && !strcmp(argv[1], "-t"))
    {
        crypt_rsa_test();
        return;
    }
    
    if(argc != 4)
    {
        printf("Usage: '%s <infile> <outfile> <password>\n", argv[0]);
        return -1;
    }
    
    char *in_filename = argv[1];
    char *out_filename = argv[2];
    char *password = argv[3];

    /* hash the password */
    uint64_t key = 0;
    hash_password(password, &key);
    
    /* setup cipher with that hash */
    crypt_cipher_t *crypt_ctx;
    crypt_lfsr64_init(&crypt_ctx, key);
    
    /* open files */
    FILE *in_file = fopen(in_filename, "rb");
    if(!in_file)
    {
        printf("Could not open '%s'\n", in_filename);
        return -1;
    } 
    
    FILE *out_file = fopen(out_filename, "w");
    if(!in_file)
    {
        printf("Could not open '%s'\n", out_filename);
        return -1;
    }
    
    char *buffer = malloc(BLOCKSIZE);
    uint32_t file_offset = 0;
    
    while(!feof(in_file))
    {
        int ret = fread(buffer, 1, BLOCKSIZE, in_file);
        
        if(ret > 0)
        {
            crypt_ctx->decrypt(crypt_ctx, buffer, buffer, ret, file_offset);
            fwrite(buffer, 1, ret, out_file);
            file_offset += ret;
        }
    }
    
    fclose(in_file);
    fclose(out_file);
}
