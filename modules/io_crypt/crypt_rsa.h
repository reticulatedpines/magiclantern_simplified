#ifndef __CRYPT_RSA_H
#define __CRYPT_RSA_H

typedef struct
{
	unsigned char id;
	char *name;
	char *primefac;
	char *key;
} t_crypt_key;

typedef struct
{
    crypt_cipher_t cipher;
    t_crypt_key pub_key;
    t_crypt_key priv_key;
} rsa_ctx_t;

void crypt_rsa_init(void **crypt_ctx, uint64_t password);
void crypt_rsa_test();


#endif
