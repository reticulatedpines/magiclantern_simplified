

#ifdef MODULE

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <string.h>

#define TRACE_DISABLED
#include "../trace/trace.h"

extern char *module_card_drive;

#else

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define trace_write(x,...) do { (void)0; } while (0)
//#define trace_write(x,...) do { printf(__VA_ARGS__); printf("\n"); } while (0)

#define NotifyBox(x,...) do { printf(__VA_ARGS__); printf("\n"); } while (0)
#define NotifyBoxHide() do { } while(0)
#define beep() do { } while(0)
#define beep_times(x) do { } while(0)
#define msleep(x) usleep((x)*1000)
#define task_create(a,b,c,d,e) do { d(e); } while(0)

#define FIO_CreateFileEx(file) fopen(file, "w+")
#define FIO_WriteFile(f,data,len) fwrite(data, 1, len, f)
#define FIO_ReadFile(f,data,len) fread(data, 1, len, f)
#define FIO_CloseFile(x) fclose(f)
#define FIO_Open(file,mode) fopen(file, "r")
#define FIO_GetFileSize(f,ret) getFileSize(f,ret)
#define INVALID_PTR 0
#define O_RDONLY 0
#define O_SYNC 0

size_t getFileSize(const char * filename, int *ret)
{
    struct stat st;
    stat(filename, &st);
    *ret = st.st_size;
    
    return 0;
}

char *module_card_drive = "";
#endif

#include <rand.h>

#include "io_crypt.h"
#include "crypt_rsa.h"

#include "bigd.h"
#include "bigdigits.h"  

#define assert(x) do {} while(0)

static uint32_t crypt_rsa_keysize = 1024;
extern uint32_t iocrypt_trace_ctx;


static bdigit_t small_primes[] = {
	3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43,
	47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101,
	103, 107, 109, 113,
	127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
	179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
	233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
	283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
	353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
	419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
	467, 479, 487, 491, 499, 503, 509, 521, 523, 541,
	547, 557, 563, 569, 571, 577, 587, 593, 599, 601,
	607, 613, 617, 619, 631, 641, 643, 647, 653, 659,
	661, 673, 677, 683, 691, 701, 709, 719, 727, 733,
	739, 743, 751, 757, 761, 769, 773, 787, 797, 809,
	811, 821, 823, 827, 829, 839, 853, 857, 859, 863,
	877, 881, 883, 887, 907, 911, 919, 929, 937, 941,
	947, 953, 967, 971, 977, 983, 991, 997,
};
#define N_SMALL_PRIMES (sizeof(small_primes)/sizeof(bdigit_t))


static int crypt_rsa_rand(unsigned char *bytes, size_t nbytes, const unsigned char *seed, size_t seedlen)
{
    if(0 && seed)
    {
        for(size_t pos = 0; pos < seedlen / 4; pos++)
        {
            rand_seed(((uint32_t *)seed)[pos]);
        }
    }
    
    while(((uint32_t)bytes % 4) && nbytes)
	{
        uint32_t rn = 0;
        rand_fill(&rn, 1);
		*bytes++ = rn & 0xFF;
        nbytes--;
	}

    uint32_t words = (nbytes / 4);
    uint32_t remain = (nbytes % 4);
    
    rand_fill((uint32_t*)bytes, words);
    
	for(uint32_t pos = 0; pos < remain; pos++)
	{
        uint32_t rn = 0;
        rand_fill(&rn, 1);
		bytes[words * 4 + pos] = (rn & 0xFF);
	}

	return 0;
}


int generateRSAPrime(BIGD p, size_t nbits, bdigit_t e, size_t ntests,
				 unsigned char *seed, size_t seedlen, BD_RANDFUNC randFunc)
/* Create a prime p such that gcd(p-1, e) = 1.
   Returns # prime tests carried out or -1 if failed.
   Sets the TWO highest bits to ensure that the
   product pq will always have its high bit set.
   e MUST be a prime > 2.
   This function assumes that e is prime so we can
   do the less expensive test p mod e != 1 instead
   of gcd(p-1, e) == 1.
   Uses improvement in trial division from Menezes 4.51.
  */
{
	BIGD u;
	size_t i, j, iloop, maxloops, maxodd;
	int done, overflow, failedtrial;
	int count = 0;
	bdigit_t r[N_SMALL_PRIMES];

	/* Create a temp */
	u = bdNew();

	maxodd = nbits * 100;
	maxloops = 5;

	done = 0;
	for (iloop = 0; !done && iloop < maxloops; iloop++)
	{
		/* Set candidate n0 as random odd number */
		bdRandomSeeded(p, nbits, seed, seedlen, randFunc);
		/* Set two highest and low bits */
		bdSetBit(p, nbits - 1, 1);
		bdSetBit(p, nbits - 2, 1);
		bdSetBit(p, 0, 1);

		/* To improve trial division, compute table R[q] = n0 mod q
		   for each odd prime q <= B
		*/
		for (i = 0; i < N_SMALL_PRIMES; i++)
		{
			r[i] = bdShortMod(u, p, small_primes[i]);
		}

		done = overflow = 0;
		/* Try every odd number n0, n0+2, n0+4,... until we succeed */
		for (j = 0; j < maxodd; j++, overflow = bdShortAdd(p, p, 2))
		{
			/* Check for overflow */
			if (overflow)
				break;

			count++;

			/* Each time 2 is added to the current candidate
			   update table R[q] = (R[q] + 2) mod q */
			if (j > 0)
			{
				for (i = 0; i < N_SMALL_PRIMES; i++)
				{
					r[i] = (r[i] + 2) % small_primes[i];
				}
			}

			/* Candidate passes the trial division stage if and only if
			   NONE of the R[q] values equal zero */
			for (failedtrial = 0, i = 0; i < N_SMALL_PRIMES; i++)
			{
				if (r[i] == 0)
				{
					failedtrial = 1;
					break;
				}
			}
			if (failedtrial)
				continue;

			/* If p mod e = 1 then gcd(p, e) > 1, so try again */
			bdShortMod(u, p, e);
			if (bdShortCmp(u, 1) == 0)
				continue;

			/* Do expensive primality test */
			if (bdRabinMiller(p, ntests))
			{	/* Success! - we have a prime */
				done = 1;
				break;
			}

		}
	}

	/* Clear up */
	bdFree(&u);

	return (done ? count : -1);
}

int generateRSAKey(BIGD n, BIGD e, BIGD d, BIGD p, BIGD q, BIGD dP, BIGD dQ, BIGD qInv,
	size_t nbits, bdigit_t ee, size_t ntests, unsigned char *seed, size_t seedlen,
	BD_RANDFUNC randFunc)
{
	BIGD g, p1, q1, phi;
	size_t np, nq;
	unsigned char *myseed = NULL;

	/* Initialise */
	g = bdNew();
	p1 = bdNew();
	q1 = bdNew();
	phi = bdNew();

//	printf("Generating a %d-bit RSA key...\n", nbits);

	/* We add an extra byte to the user-supplied seed */
	myseed = (unsigned char*)malloc(seedlen + 1);
	if (!myseed)
    {
        return -1;
    }
	memcpy(myseed, seed, seedlen);

	/* Do (p, q) in two halves, approx equal */
	nq = nbits / 2 ;
	np = nbits - nq;

	/* Make sure seeds are slightly different for p and q */
	myseed[seedlen] = rand();
	generateRSAPrime(p, np, ee, ntests, myseed, seedlen+1, randFunc);

	myseed[seedlen] = rand();
	generateRSAPrime(q, nq, ee, ntests, myseed, seedlen+1, randFunc);
    
	/* Check that p != q (if so, RNG is faulty!) */
	assert(!bdIsEqual(p, q));

	generateRSAPrime(e, nq, ee, ntests, myseed, seedlen+1, randFunc);

	/* If q > p swap p and q so p > q */
	if (bdCompare(p, q) < 1)
	{
		bdSetEqual(g, p);
		bdSetEqual(p, q);
		bdSetEqual(q, g);
	}

	/* Calc p-1 and q-1 */
	bdSetEqual(p1, p);
	bdDecrement(p1);
	bdSetEqual(q1, q);
	bdDecrement(q1);

	/* Check gcd(p-1, e) = 1 */
	bdGcd(g, p1, e);
	assert(bdShortCmp(g, 1) == 0);
	bdGcd(g, q1, e);
	assert(bdShortCmp(g, 1) == 0);

	/* Compute n = pq */
	bdMultiply(n, p, q);

	/* Compute d = e^-1 mod (p-1)(q-1) */
	bdMultiply(phi, p1, q1);
	bdModInv(d, e, phi);

	/* Check ed = 1 mod phi */
	bdModMult(g, e, d, phi);
	assert(bdShortCmp(g, 1) == 0);

	/* Calculate CRT key values */
	bdModInv(dP, e, p1);
	bdModInv(dQ, e, q1);
	bdModInv(qInv, q, p);
    
	/* Clean up */
    free(myseed);
	bdFree(&g);
	bdFree(&p1);
	bdFree(&q1);
	bdFree(&phi);

	return 0;
}


int crypt_rsa_generate(int nbits, t_crypt_key *priv_key, t_crypt_key *pub_key)
{
	int res = 0;
	char *buffer = NULL;
	unsigned char *tmp_buf = NULL;

	BIGD n, e, d, p, q, dP, dQ, qInv;
	BIGD source;
	BIGD result;

	/* Initialise */
	p = bdNew();
	q = bdNew();
	n = bdNew();
	e = bdNew();
	d = bdNew();
	dP = bdNew();
	dQ = bdNew();
	qInv = bdNew();

	/* Create RSA key pair (n, e),(d, p, q, dP, dQ, qInv) */
	res = generateRSAKey(n, e, d, p, q, dP, dQ, qInv, nbits+1, 3, 50, NULL, 0, crypt_rsa_rand);

	if(res != 0)
	{
		NotifyBox(5000, "Failed to generate RSA key!\n");
		goto clean_up;
	}

	priv_key->id = 0x00;
	pub_key->id = 0x00;
	priv_key->name = strdup("new key");
	pub_key->name = strdup("new key");

	size_t nchars = bdConvToHex(n, NULL, 0);
	buffer = malloc(nchars+1);
	nchars = bdConvToHex(n, buffer, nchars+1);
    
	priv_key->primefac = (char*)strdup((char *)buffer);
	pub_key->primefac = (char*)strdup((char *)buffer);
	free(buffer);
    
    nchars = bdConvToHex(d, NULL, 0);
	buffer = malloc(nchars+1);
	nchars = bdConvToHex(d, buffer, nchars+1);
    
	priv_key->key = (char*)strdup((char *)buffer);
	free(buffer);

    nchars = bdConvToHex(e, NULL, 0);
	buffer = malloc(nchars+1);
	nchars = bdConvToHex(e, buffer, nchars+1);
    
	pub_key->key = (char*)strdup((char *)buffer);
	free(buffer);


	bdConvFromHex(d, priv_key->key);
	bdConvFromHex(e, pub_key->key);
	bdConvFromHex(n, pub_key->primefac);

	source = bdNew ();
	result = bdNew ();
	tmp_buf = (unsigned char*)malloc ( nbits*8*4 );
	strcpy((char*)tmp_buf, "Test");

	bdConvFromOctets( source, tmp_buf, 8);
	bdModExp(result, source, d, n);

	bdConvToOctets(result, tmp_buf, bdSizeof(result)*4);
	bdConvFromOctets(result, tmp_buf, bdSizeof(result)*4);

	bdModExp(source, result, e, n);
	bdConvToOctets(source, tmp_buf, bdSizeof(source)*4);
	if(strcmp((char*)"Test", (char*)tmp_buf))
    {
		printf ( "Key check FAILED!!\n" );
        beep();
    }
    
    free(tmp_buf);

clean_up:
	bdFree(&n);
	bdFree(&e);
	bdFree(&d);
	bdFree(&p);
	bdFree(&q);
	bdFree(&dP);
	bdFree(&dQ);
	bdFree(&qInv);

	return 0;
}


unsigned int crypt_rsa_crypt(uint8_t *dst, uint8_t *src, int length, t_crypt_key *key)
{
	BIGD keyval;
	BIGD primefac;
	BIGD buffer;
	BIGD result;
	unsigned int bytes = 0;

	keyval = bdNew();
	primefac = bdNew();
	buffer = bdNew();
	result = bdNew();

	bdConvFromHex(keyval, key->key);
	bdConvFromHex(primefac, key->primefac);
	bdConvFromOctets(buffer, src, length);

	bdModExp(result, buffer, keyval, primefac);

	bytes = bdSizeof (result)*4;
	bdConvToOctets(result, dst, bytes);

	bdFree(&keyval);
	bdFree(&primefac);
	bdFree(&buffer);
	bdFree(&result);

	return bytes;
}

t_crypt_key *crypt_rsa_get_priv(crypt_cipher_t *crypt_ctx)
{
    if(!crypt_ctx || !crypt_ctx->priv)
    {
        return NULL;
    }
    
    rsa_ctx_t *ctx = (rsa_ctx_t *)crypt_ctx->priv;
    
    if(!strlen(ctx->priv_key.name))
    {
        return NULL;
    }
    
    return &ctx->priv_key;
}

t_crypt_key *crypt_rsa_get_pub(crypt_cipher_t *crypt_ctx)
{
    if(!crypt_ctx || !crypt_ctx->priv)
    {
        return NULL;
    }
    
    rsa_ctx_t *ctx = (rsa_ctx_t *)crypt_ctx->priv;
    
    if(!strlen(ctx->pub_key.name))
    {
        return NULL;
    }
    
    return &ctx->pub_key;
}

/* returns the key size in bits */
uint32_t crypt_rsa_get_keysize(crypt_cipher_t *crypt_ctx)
{
    if(!crypt_ctx || !crypt_ctx->priv)
    {
        return 0;
    }
    
    rsa_ctx_t *ctx = (rsa_ctx_t *)crypt_ctx->priv;
    
    int nibbles = strlen(ctx->pub_key.primefac);
    
    return nibbles * 4;
}

void crypt_rsa_set_keysize(uint32_t size)
{
    crypt_rsa_keysize = size;
}

void crypt_rsa_clear_key(t_crypt_key *key)
{
    key->name = "";
    key->primefac = "";
    key->key = "";
}

/* returns the key size in bits */
uint32_t crypt_rsa_blocksize(crypt_cipher_t *crypt_ctx)
{
    uint32_t keysize = crypt_rsa_get_keysize(crypt_ctx);
    
    if(!keysize)
    {
        return 0;
    }
    
    return (keysize / 8);
}

static uint32_t crypt_rsa_encrypt(crypt_cipher_t *crypt_ctx, uint8_t *dst, uint8_t *src, uint32_t length, uint32_t offset)
{
    if(!crypt_ctx || !crypt_ctx->priv)
    {
        return 0;
    }
    rsa_ctx_t *ctx = (rsa_ctx_t *)crypt_ctx->priv;
    
    if(crypt_rsa_blocksize(crypt_ctx) > length)
    {
        trace_write(iocrypt_trace_ctx, "crypt_rsa_decrypt: key size mismatch %d vs. %d", crypt_rsa_blocksize(crypt_ctx), length);
        return 0;
    }
    
    uint32_t new_len = crypt_rsa_crypt(dst, src, length, &ctx->pub_key);
    
    return new_len;
}

static uint32_t crypt_rsa_decrypt(crypt_cipher_t *crypt_ctx, uint8_t *dst, uint8_t *src, uint32_t length, uint32_t offset)
{
    if(!crypt_ctx || !crypt_ctx->priv)
    {
        return 0;
    }
    rsa_ctx_t *ctx = (rsa_ctx_t *)crypt_ctx->priv;
    
    if(crypt_rsa_blocksize(crypt_ctx) > length)
    {
        trace_write(iocrypt_trace_ctx, "crypt_rsa_decrypt: key size mismatch %d vs. %d", crypt_rsa_blocksize(crypt_ctx), length);
        return 0;
    }
    
    uint32_t new_len = crypt_rsa_crypt(dst, src, length, &ctx->priv_key);
    
    return new_len;
}

static void crypt_rsa_deinit(void **crypt_ctx)
{
    if(*crypt_ctx)
    {
        free(*crypt_ctx);
        *crypt_ctx = NULL;
    }
}

static uint32_t crypt_rsa_save(char *file, t_crypt_key *key)
{
    char filename[32];
    
    snprintf(filename, sizeof(filename), "%s%s", module_card_drive, file);
    
    FILE* f = FIO_CreateFileEx(filename);
    if(f == INVALID_PTR)
    {
        return 0;
    }
    
    FIO_WriteFile(f, key->primefac, strlen(key->primefac));
    FIO_WriteFile(f, "\n", 1);
    FIO_WriteFile(f, key->key, strlen(key->key));
    FIO_WriteFile(f, "\n", 1);
    
    FIO_CloseFile(f);
    
    return 1;
}

uint32_t crypt_rsa_load(char *file, t_crypt_key *key)
{
    int size = 0;
    char filename[32];
    
    snprintf(filename, sizeof(filename), "%s%s", module_card_drive, file);
    
    if(FIO_GetFileSize(filename, &size))
    {
        trace_write(iocrypt_trace_ctx, "io_crypt: crypt_rsa_load: file not found: '%s'", filename);
        return 0;
    }
    
    FILE* f = FIO_Open(filename, O_RDONLY | O_SYNC);
    if(f == INVALID_PTR)
    {
        return 0;
    }
    
    char *buffer = malloc(size);
    if(FIO_ReadFile(f, buffer, size) != (int)size)
    {
        trace_write(iocrypt_trace_ctx, "io_crypt: crypt_rsa_load: FIO_ReadFile failed");
        free(buffer);
        return 0;
    }
    
    char *sep = strchr(buffer, '\n');
    if(!sep)
    {
        trace_write(iocrypt_trace_ctx, "io_crypt: crypt_rsa_load: invalid file format");
        free(buffer);
        return 0;
    }
    
    /* split strings */
    *sep = '\000';
    sep++;
    
    /* remove termination */
    char *sep2 = strchr(sep, '\n');
    if(sep2)
    {
        *sep2 = '\000';
    }
    
    /* now fill key */
    key->name = strdup(filename);
    key->primefac = strdup(buffer);
    key->key = strdup(sep);

    free(buffer);
    FIO_CloseFile(f);
    
    return 1;
}


void crypt_rsa_generate_keys(crypt_cipher_t *crypt_ctx)
{
    t_crypt_key priv_key;
    t_crypt_key pub_key;
    
    NotifyBox(60000, "Creating RSA key (%d bits)\nthis may take a while", crypt_rsa_keysize);
    trace_write(iocrypt_trace_ctx, "io_crypt: crypt_rsa_generate %d...", crypt_rsa_keysize);
    crypt_rsa_generate(crypt_rsa_keysize, &priv_key, &pub_key);
    trace_write(iocrypt_trace_ctx, "io_crypt: crypt_rsa_generate %d done", crypt_rsa_keysize);
    NotifyBoxHide();
    beep();
    NotifyBox(2000, "RSA key generated!");
    
    crypt_rsa_save("ML/DATA/io_crypt.key", &priv_key);
    crypt_rsa_save("ML/DATA/io_crypt.pub", &pub_key);
    
    /* now reload to make sure all is file */
    rsa_ctx_t *ctx = (rsa_ctx_t *)crypt_ctx->priv;
    
    crypt_rsa_clear_key(&ctx->pub_key);
    crypt_rsa_clear_key(&ctx->priv_key);
    
    crypt_rsa_load("ML/DATA/io_crypt.pub", &ctx->pub_key);
    crypt_rsa_load("ML/DATA/io_crypt.key", &ctx->priv_key);
}

static uint32_t crypt_rsa_testfunc(int size, t_crypt_key *priv_key, t_crypt_key *pub_key)
{
    uint32_t ret = 0;
    
    NotifyBox(2000, "crypt_rsa_generate %d...", size);
    trace_write(iocrypt_trace_ctx, "io_crypt: crypt_rsa_generate %d...", size);
    crypt_rsa_generate(size, priv_key, pub_key);
    trace_write(iocrypt_trace_ctx, "io_crypt: crypt_rsa_generate %d done", size);
    NotifyBox(2000, "crypt_rsa_generate %d... DONE", size);
    
    trace_write(iocrypt_trace_ctx, "priv_key: name     %s", priv_key->name);
    trace_write(iocrypt_trace_ctx, "priv_key: primefac %s", priv_key->primefac);
    trace_write(iocrypt_trace_ctx, "priv_key: key      %s", priv_key->key);
    trace_write(iocrypt_trace_ctx, "pub_key:  name     %s", pub_key->name);
    trace_write(iocrypt_trace_ctx, "pub_key:  primefac %s", pub_key->primefac);
    trace_write(iocrypt_trace_ctx, "pub_key:  key      %s", pub_key->key);
    
    uint32_t size_bytes = size / 8;
    uint32_t *data = malloc(size_bytes * 2);
    uint32_t *data_orig = malloc(size_bytes * 2);
    
    for(int pos = 0; pos < size_bytes; pos++)
    {
        ((uint8_t *)data)[pos] = pos;
        ((uint8_t *)data_orig)[pos] = pos;
    }
    
    trace_write(iocrypt_trace_ctx, "Encryption test:");
    trace_write(iocrypt_trace_ctx, "   pre-crypt:    0x%08X%08X%08X%08X (%d bytes)", data[3], data[2], data[1], data[0], size_bytes);
    uint32_t new_len = crypt_rsa_crypt((uint8_t*)data, (uint8_t*)data, size / 8, pub_key);
    trace_write(iocrypt_trace_ctx, "   post-crypt:   0x%08X%08X%08X%08X (%d bytes)", data[3], data[2], data[1], data[0], new_len);
    new_len = crypt_rsa_crypt((uint8_t*)data, (uint8_t*)data, new_len, priv_key);
    trace_write(iocrypt_trace_ctx, "   post-decrypt: 0x%08X%08X%08X%08X (%d bytes)", data[3], data[2], data[1], data[0], size_bytes);
    
    for(int pos = 0; pos < (size_bytes / 4); pos++)
    {
        if(data[pos] != data_orig[pos])
        {
            ret = 1;
            trace_write(iocrypt_trace_ctx, "   post-decrypt: FAILED at pos %d", pos);

            NotifyBox(5000, "Test failed, check log!\n");
            beep();
            break;
        }
    }
    
    free(data);
    free(data_orig);
    
    return ret;
}

void crypt_rsa_test()
{
    /* dome some tests */
    t_crypt_key priv_key;
    t_crypt_key pub_key;
    uint32_t ret = 0;
    
    ret |= crypt_rsa_testfunc(128, &priv_key, &pub_key);
    ret |= crypt_rsa_testfunc(256, &priv_key, &pub_key);
    ret |= crypt_rsa_testfunc(512, &priv_key, &pub_key);
    ret |= crypt_rsa_testfunc(1024, &priv_key, &pub_key);
    
    msleep(5000);
    beep();

    if(ret)
    {
        NotifyBox(5000, "Test failed, check log!\n");
        beep_times(5);
    }
    else
    {
        NotifyBox(5000, "Test finished successfully\n");
    }
}

static void crypt_rsa_set_blocksize(crypt_cipher_t *crypt_ctx, uint32_t size)
{
    crypt_rsa_keysize = size;
}

/* allocate and initialize an RSA cipher ctx and save to pointer */
void crypt_rsa_init(crypt_cipher_t *crypt_ctx)
{
    rsa_ctx_t *ctx = malloc(sizeof(rsa_ctx_t));
    
    if(!ctx)
    {
        trace_write(iocrypt_trace_ctx, "crypt_rsa_init: failed to malloc");
        return;
    }
    
    /* setup cipher ctx */
    crypt_ctx->encrypt = (uint32_t (*)(void *, uint8_t *, uint8_t *, uint32_t, uint32_t))&crypt_rsa_encrypt;
    crypt_ctx->decrypt = (uint32_t (*)(void *, uint8_t *, uint8_t *, uint32_t, uint32_t))&crypt_rsa_decrypt;
    crypt_ctx->deinit = (void (*)(void *))&crypt_rsa_deinit;
    crypt_ctx->set_blocksize = (void (*)(void *, uint32_t))&crypt_rsa_set_blocksize;
    crypt_ctx->priv = ctx;
    
    /* load all keys that are on card */
    crypt_rsa_clear_key(&ctx->pub_key);
    crypt_rsa_clear_key(&ctx->priv_key);
    
    crypt_rsa_load("ML/DATA/io_crypt.pub", &ctx->pub_key);
    crypt_rsa_load("ML/DATA/io_crypt.key", &ctx->priv_key);
    crypt_rsa_load("io_crypt.pub", &ctx->pub_key);
    crypt_rsa_load("io_crypt.key", &ctx->priv_key);
    
    //trace_write(iocrypt_trace_ctx, "    pub_key:  name     %s", ctx->pub_key.name);
    //trace_write(iocrypt_trace_ctx, "    pub_key:  primefac %s", ctx->pub_key.primefac);
    //trace_write(iocrypt_trace_ctx, "    pub_key:  key      %s", ctx->pub_key.key);
    //trace_write(iocrypt_trace_ctx, "    priv_key:  name     %s", ctx->priv_key.name);
    //trace_write(iocrypt_trace_ctx, "    priv_key:  primefac %s", ctx->priv_key.primefac);
    //trace_write(iocrypt_trace_ctx, "    priv_key:  key      %s", ctx->priv_key.key);
    
    trace_write(iocrypt_trace_ctx, "crypt_rsa_init: initialized");
}




