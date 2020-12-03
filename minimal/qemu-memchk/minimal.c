/** \file
 * Minimal ML - for debugging
 */

#include "dryos.h"

extern void * _malloc(int);
extern void   _free(void *);

extern void * _AllocateMemory(int);
extern void   _FreeMemory(void *);

__attribute__((optimize("-fno-ipa-sra")))
static void malloc_test(const char * allocator_name, void * (*alloc)(int), void (*afree)(void*))
{
    qprintf("Allocating memory using %s...\n", allocator_name);
    uint32_t * p = alloc(32 * sizeof(p[0]));
    qprintf("Allocated from %X to %X\n", p, p + 127);
    qprintf("Reading without initializing...\n", p);
    p[5] = p[10] + 1;       /* this one should be easy to catch */
    p[6] = p[5] + 2;        /* this one won't trigger yet */
    qprintf("Reading outside bounds...\n", p);
    p[10] = p[-1] - 1;      /* this will be caught only on tracked memory heaps */
    p[11] = p[32] + 32;     /* same */
    p[12] = p[-10] - 10;    /* this may end up in some nearby block (so the error might not be caught) */
    p[13] = p[64] + 64;     /* same */
    /* writing outside bounds may disrupt future code execution */
    //qprintf("Writing outside bounds...\n", p);
    //p[-1] = -1;
    //p[32] = 32;
    //p[-10] = -10;
    //p[64] = 64;
    qprintf("Freeing memory...\n");
    afree(p);
    qprintf("Use after free...\n", p);
    p[15] = p[20] + 1;
    qprintf("%s test complete.\n\n", allocator_name);
}

static void malloc_tests()
{
    malloc_test("malloc", _malloc, _free);
    malloc_test("AllocateMemory", _AllocateMemory, _FreeMemory);
}

/* called before Canon's init_task */
void boot_pre_init_task(void)
{
    /* nothing to do */
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
    task_create("malloc_tests", 0x1e, 0x4000, malloc_tests, 0 );
}

/* dummy stubs */

void disp_set_pixel(int x, int y, int c) { }
int bmp_printf(uint32_t fontspec, int x, int y, const char *fmt, ... ) { return 0; }
int bfnt_draw_char() { return 0; }
