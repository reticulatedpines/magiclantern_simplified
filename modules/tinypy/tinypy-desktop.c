#include <stdio.h>
#include <sys/stat.h>

#define tcc_mallocz(x) calloc((x),1)
#define tcc_realloc(x,y) realloc(x,y)
#define tcc_free(x) free(x)

int GetFileSize(const char* fname)
{
    struct stat s;
    stat(fname, &s);
    return s.st_size;
}

#include "tinypy.c"

int main(int argc, char *argv[]) {
    tp_vm *tp = tp_init(argc, argv);
    tp_ez_call(tp,"py2bc","tinypy",tp_None);
    tp_deinit(tp);
    return(0);
}

/**/
