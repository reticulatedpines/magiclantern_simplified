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
    uint32_t progress;
} rsa_ctx_t;


t_crypt_key *crypt_rsa_get_priv(crypt_priv_t *priv);
t_crypt_key *crypt_rsa_get_pub(crypt_priv_t *priv);
uint32_t crypt_rsa_get_keysize(crypt_priv_t *priv);
uint32_t crypt_rsa_get_keyprogress(crypt_priv_t *priv);
uint32_t crypt_rsa_blocksize(crypt_priv_t *priv);
void crypt_rsa_generate_keys(crypt_priv_t *priv);
void crypt_rsa_set_keysize(uint32_t size);
void crypt_rsa_clear_key(t_crypt_key *key);
uint32_t crypt_rsa_load(char *file, t_crypt_key *key);
void crypt_rsa_test();
void crypt_rsa_init(crypt_cipher_t *crypt_ctx);


#endif
