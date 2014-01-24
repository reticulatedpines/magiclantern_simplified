
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

int rand()
{
    uint32_t ret = 0;
    
    rand_fill(&ret, 1);
    return ABS(ret);
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
    memset(ret, 0x00, nmemb * size);
    
    return ret;
}

void *realloc(void *ptr, size_t size)
{
    void *ret = malloc(size);
    memcpy(ret, ptr, size);
    free(ptr);
    
    return ret;
}
