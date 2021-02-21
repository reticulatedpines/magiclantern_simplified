
#include <dryos.h>
#include <mem.h>
#include <rand.h>

/* in this file we implement everything needed which is meant to make 
   Magic Lantern compatible to POSIX if needed.
   For a list of POSIX headers and the prototypes in it, see for example:
   http://pubs.opengroup.org/onlinepubs/9699919799/idx/head.html
   
   This does *not* mean, ML has to implement a whole POSIX system, but
   this is merely the file where will place missing functions that are
   defined in POSIX but not provided by our libc version or Canon's OS.
*/

/**
 * POSIX standard assumes rand() to return always positive integers
 * but we may return negative ones when casting an uint32_t to int
 */
int rand()
{
    uint32_t ret = 0;
    
    rand_fill(&ret, 1);

    // Clear sign bit
    return ret & 0x7fffffff; 
}

void srand(unsigned int seed)
{
    rand_seed(seed);
}

char *strdup(const char*str)
{
    char *ret = malloc(strlen(str) + 1);
    strcpy(ret, str);
    return ret;
}

/* warning - not implemented yet */
int time()
{
    return 0;
}

int clock()
{
    return rand();
}

void *calloc(size_t nmemb, size_t size)
{
    void *ret = malloc(nmemb * size);
    if (ret) memset(ret, 0x00, nmemb * size);
    
    return ret;
}

void *realloc(void *ptr, size_t size)
{
    void *ret = malloc(size);
    
    if (ptr)
    {
        memcpy(ret, ptr, size);
        free(ptr);
    }
    
    return ret;
}

size_t strlcat(char *dest, const char *src, size_t n)
{
    uint32_t dst_len = strlen(dest);
    uint32_t src_len = strlen(src);
    uint32_t len = MIN(n - dst_len - 1, src_len);
    
    memcpy(&dest[dst_len], src, len);
    dest[dst_len + len] = '\000';
    
    return dst_len + len;
}


char *strcat(char *dest, const char *src)
{
    strlcat(dest, src, 0x7FFFFFFF);
    return dest;
}


