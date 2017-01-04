/* TCC Hello World example */
/* It compiles C programs on the camera and runs them as scripts at native speed! */
/* Memory usage: 150K as ARM, 115K as Thumb */

/* You will have to link this with libtcc.a and dietlib. */

/* Based on tests/tcctest.c from tcc-0.9.26 */

#include "dryos.h"
#include "libtcc.h"
#include "console.h"

void _tcc_exit(int code) 
{
    printf("exit(%d)\n", code);
    console_show();
    while(1) msleep(100); // fixme: stop the task and exit cleanly
}

/* i don't really understand FIO_SeekSkipFile etc yet, so build seek-able open function */
typedef struct
{
    int size;
    int pos;
    char data;
} filehandle_t;

int _tcc_open(const char *pathname, int flags)
{
    uint32_t size = 0;
    FILE* file = NULL;
    filehandle_t *handle = NULL;
    
    if( FIO_GetFileSize( pathname, &size ) != 0 )
    {
        printf("Error loading '%s': File does not exist\n", pathname);
        return -1;
    }
    handle = fio_malloc(sizeof(filehandle_t) + size);
    if(!handle)
    {
        printf("Error loading '%s': File too large\n", pathname);
        return -1;
    }
    
    handle->size = size;
    handle->pos = 0;
    
    file = FIO_OpenFile(pathname, flags);
    if (!file)
    {
        printf("Error loading '%s': File does not exist\n", pathname);
        fio_free(handle);
        return -1;
    }
    FIO_ReadFile(file, &handle->data, size);
    FIO_CloseFile(file);
    
    return (int)handle;
}

int _tcc_read(int fd, void *buf, int size)
{
    filehandle_t *handle = (filehandle_t *)fd;
    int count = (size + handle->pos < handle->size)? (size) : MAX(handle->size - handle->pos, 0);
    
    memcpy(buf, ((void*)&handle->data) + handle->pos, count);
    handle->pos += count;
    
    return count;
}

int _tcc_close(int fd)
{
    fio_free((void*)fd);
    return 0;
}

int _tcc_lseek(int fd, int offset, int whence)
{
    filehandle_t *handle = (filehandle_t *)fd;
    
    switch(whence)
    {
        case 0:
            handle->pos = offset;
            break;
        case 1:
            handle->pos += offset;
            break;
        case 2:
            handle->pos = handle->size - offset;
            break;
    }
    
    return handle->pos;
}

/*
for debugging: compile TCC with CFLAGS+=-finstrument-functions

void __cyg_profile_func_enter(void *this_fn, void *call_site)
                              __attribute__((no_instrument_function));

void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  printf("ENTER: %x, from %x\n", this_fn, call_site);
}

void __cyg_profile_func_exit(void *this_fn, void *call_site)
                             __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *this_fn, void *call_site) {
  printf("EXIT:  %x, from %x\n", this_fn, call_site);
}

*/
