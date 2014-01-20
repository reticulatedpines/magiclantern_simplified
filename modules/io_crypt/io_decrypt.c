
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "io_crypt.h"
#include "crypt_lfsr64.h"
#include "hash_password.h"


#define BLOCKSIZE (8 * 1024)


int main(int argc, char *argv[])
{
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
