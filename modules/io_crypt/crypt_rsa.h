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
    t_crypt_key pub_key;
    t_crypt_key priv_key;
} rsa_ctx_t;

void crypt_rsa_init(crypt_cipher_t *crypt_ctx);
void crypt_rsa_test();
void crypt_rsa_generate_keys();
uint32_t crypt_rsa_blocksize(crypt_cipher_t *crypt_ctx);
uint32_t crypt_rsa_get_keyize(crypt_cipher_t *crypt_ctx);
void crypt_rsa_set_keysize(uint32_t size);
t_crypt_key *crypt_rsa_get_priv(crypt_cipher_t *crypt_ctx);
t_crypt_key *crypt_rsa_get_pub(crypt_cipher_t *crypt_ctx);

#endif
