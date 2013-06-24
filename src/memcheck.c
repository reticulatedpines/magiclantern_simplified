
#include "compiler.h"
#include "tasks.h"

#define MEM_SEC_ZONE 32
#define MEMCHECK_ENTRIES 1024

typedef struct
{
    unsigned int id;
    unsigned int length;
} t_memcheck_hdr;

typedef struct
{
    unsigned int ptr;
    unsigned int failed;
    unsigned char * file;
    unsigned int line;
} t_memcheck_entry;

t_memcheck_entry memcheck_mallocbuf[MEMCHECK_ENTRIES];
unsigned int memcheck_bufpos = 0;

unsigned int memcheck_check(unsigned int entry)
{
    unsigned int ptr = memcheck_mallocbuf[entry].ptr;
    unsigned int failed = 0;
    
    for(int pos = sizeof(t_memcheck_hdr); pos < MEM_SEC_ZONE; pos++)
    {
        unsigned char value = ((unsigned char *)ptr)[pos];
        // console_printf("check %d %x\n ", pos, value);
        if (value != 0xA5)
            failed |= 2;
    }
    for(int pos = 0; pos < MEM_SEC_ZONE; pos++)
    {
        int pos2 = MEM_SEC_ZONE + ((t_memcheck_hdr *)ptr)->length + pos;
        unsigned char value = ((unsigned char *)ptr)[pos2];
        // console_printf("check %d %x\n ", pos2, value);
        if (value != 0xA5)
            failed |= 4;
    }  
    if((((t_memcheck_hdr *)ptr)->id != 0xFFFFFFFF) && (((t_memcheck_hdr *)ptr)->id != entry))
    {
        failed |= 8;
    }
    
    memcheck_mallocbuf[entry].failed |= failed;
    
    return failed;
}

unsigned int memcheck_get_failed(unsigned char **file, unsigned int *line)
{
    unsigned int buf_pos = 0;
    
    for(buf_pos = 0; buf_pos < MEMCHECK_ENTRIES; buf_pos++)
    {
        if(memcheck_mallocbuf[buf_pos].ptr)
        {
            memcheck_check(buf_pos);
            
            /* marked as failed? */
            if(memcheck_mallocbuf[buf_pos].failed)
            {
                *file = memcheck_mallocbuf[buf_pos].file;
                *line = memcheck_mallocbuf[buf_pos].line;
                /*
                memcheck_mallocbuf[buf_pos].failed = 0;
                memcheck_mallocbuf[buf_pos].ptr = 0;
                */
                return memcheck_mallocbuf[buf_pos].failed;
            }
        }
    }
    return 0;
}

void memcheck_add(unsigned int ptr, const char *file, unsigned int line)
{
    int tries = MEMCHECK_ENTRIES;
    
    unsigned int state = cli();
    while(memcheck_mallocbuf[memcheck_bufpos].ptr != 0)
    {
        memcheck_bufpos++;
        memcheck_bufpos %= MEMCHECK_ENTRIES;
        
        if(--tries <= 0)
        {
            ((t_memcheck_hdr *)ptr)->id = 0xFFFFFFFF;
            return;
        }
    }
    memcheck_mallocbuf[memcheck_bufpos].ptr = ptr;
    memcheck_mallocbuf[memcheck_bufpos].failed = 0;
    memcheck_mallocbuf[memcheck_bufpos].file = file;
    memcheck_mallocbuf[memcheck_bufpos].line = line;
    
    ((t_memcheck_hdr *)ptr)->id = memcheck_bufpos;
    
    sei(state);
}

void memcheck_remove(unsigned int ptr, unsigned int failed)
{
    unsigned int buf_pos = ((t_memcheck_hdr *)ptr)->id;
    
    if(buf_pos != 0xFFFFFFFF && (failed || memcheck_mallocbuf[buf_pos].ptr != ptr))
    {
        for(buf_pos = 0;buf_pos < MEMCHECK_ENTRIES;buf_pos++)
        {
            if(memcheck_mallocbuf[buf_pos].ptr == ptr)
            {
                memcheck_mallocbuf[buf_pos].ptr = 0xFFFFFFFF;
                memcheck_mallocbuf[buf_pos].failed |= (0x00000001 | failed);
            }            
        }
    }
    else
    {
        memcheck_mallocbuf[buf_pos].failed = 0;
        memcheck_mallocbuf[buf_pos].file = 0;
        
        unsigned int state = cli();
        memcheck_mallocbuf[buf_pos].ptr = 0;
        memcheck_bufpos = buf_pos;
        sei(state);
    }
}

void *memcheck_malloc( unsigned int len, const char *file, unsigned int line, int mode)
{
    unsigned int ptr;
    
    // console_printf("alloc %d %s:%d\n ", len, file, line);

    if(mode)
    {
        ptr = (unsigned int)malloc(len + 2 * MEM_SEC_ZONE);
    }
    else
    {
        ptr = (unsigned int)AllocateMemory(len + 2 * MEM_SEC_ZONE);
    }
    
    /* first fill all with 0xA5 */
    for(int pos = 0; pos < (len + 2 * MEM_SEC_ZONE); pos++)
    {
        ((unsigned char *)ptr)[pos] = 0xA5;
    }
    
    ((t_memcheck_hdr *)ptr)->length = len;
    
    memcheck_add(ptr, file, line);

    
    return ptr + MEM_SEC_ZONE;
}

void memcheck_free( void * buf, int mode)
{
    unsigned int ptr = ((unsigned int)buf - MEM_SEC_ZONE);
    unsigned int failed = 0;
    unsigned int failed_pos = 0;
    
    for(int pos = sizeof(t_memcheck_hdr); pos < MEM_SEC_ZONE; pos++)
    {
        unsigned char value = ((unsigned char *)ptr)[pos];
        // console_printf("free check %d %x\n ", pos, value);
        if (value != 0xA5)
        {
            failed |= 2;
            failed_pos = pos;
        }
    }
    for(int pos = 0; pos < MEM_SEC_ZONE; pos++)
    {
        int pos2 = MEM_SEC_ZONE + ((t_memcheck_hdr *)ptr)->length + pos;
        unsigned char value = ((unsigned char *)ptr)[pos2];
        // console_printf("free check %d %x\n ", pos2, value);
        if (value != 0xA5)
        {
            failed |= 4;
            failed_pos = pos2;
        }
    }
    
    if (failed)
    {
        int id = ((t_memcheck_hdr *)ptr)->id;
        char* file = memcheck_mallocbuf[id].file;
        int line = memcheck_mallocbuf[id].line;
        console_printf(
            "Failed: %s:%d\n"
            "Error:  %d\n"
            "Index:  %d\n", 
            file, line, failed, failed_pos - MEM_SEC_ZONE
        );
    }
    
    memcheck_remove(ptr, failed);

    if(mode)
    {
        free(ptr);
    }
    else
    {
        FreeMemory(ptr);
    }    
}

#if 0
void *memcheck_realloc( void * buf, unsigned int newlen, const char *file, unsigned int line)
{
    unsigned char *ptr = memcheck_malloc(newlen, file, line, 1);
    
    /* this is really bad */
    memcpy(ptr, buf, newlen); 
    memcheck_free(buf,1);
    
    return ptr;
}
#endif

void memcheck_main()
{
    unsigned char *file = (void *)0;
    unsigned int line = (void *)0;
    
    while(1)
    {
        unsigned int id = memcheck_get_failed(&file, &line);
        if(id)
        {
            console_show();
            console_printf(
                "Failed: %s:%d\n"
                "Error:  %d\n", 
                file, line, id
            );
        }
        msleep(200);
    }   
}

TASK_CREATE( "memcheck_task", memcheck_main, 0, 0x1e, 0x2000 );

