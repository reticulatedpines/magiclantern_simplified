/* TCC Hello World example */
/* It compiles C programs on the camera and runs them as scripts at native speed! */
/* Memory usage: 150K as ARM, 115K as Thumb */

/* You will have to link this with libtcc.a and dietlib. */

/* Based on tests/tcctest.c from tcc-0.9.26 */

#include "dryos.h"
#include "libtcc.h"

/* this function is called by the generated code */
int add(int a, int b)
{
    return a + b;
}

void tcc_init_library(TCCState *s)
{
    /* as a test, we add a symbol that the compiled program can use.
       You may also open a dll with tcc_add_dll() and use symbols from that */
    tcc_add_symbol(s, "add", add);
}

int tcc_compile_and_run(char* filename)
{
    int exit_code = 0;
    
    /* Read the source file */
    int size;
    char* source = (char*)read_entire_file(filename, &size);
    if (!source)
    {
        printf("Error loading '%s'\n", filename);
        exit_code = 1; goto end;
    }

    TCCState *s;
    int (*main)(int,int);

    /* create a TCC compilation context */
    s = tcc_new();
    if (!s)
        { exit_code = 1; goto end; }
    
    /* do not load standard libraries - we don't have them */
    tcc_set_options(s, "-nostdinc -nostdlib");

    /* MUST BE CALLED before any compilation */
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    /* compile our program from string */
    if (tcc_compile_string(s, source) == -1)
        { exit_code = 1; goto end; }

    tcc_init_library(s);

    /* relocate the code */
    if (tcc_relocate(s, TCC_RELOCATE_AUTO) < 0)
        { exit_code = 1; goto end; }

    /* get entry symbol */
    main = tcc_get_symbol(s, "main");
    if (!main)
    {
        printf("Could not find main()\n");
        exit_code = 1; goto end; 
    }
    
    /* enable the caching bit (todo: use AllocateMemory instead of alloc_dma_memory) */
    main = CACHEABLE(main);
    
    /* run the code */
    main(0, 0);

end:
    /* delete the compilation state and the source code buffer */
    if (s) tcc_delete(s);
    if (source) free_dma_memory(source);
    return exit_code;
}

void exit(int code) 
{
    console_printf("exit(%d)\n", code); 
    while(1) msleep(100); // fixme: stop the task and exit cleanly
}

int fprintf(FILE* unused, const char * fmt, ...)
{
    va_list            ap;
    char buf[128];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );
    console_puts(buf);
    return 0;
}

int printf(const char * fmt, ...)
{
    va_list            ap;
    char buf[128];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );
    console_puts(buf);
    return 0;
}

int puts(const char * fmt)
{
    console_puts(fmt);
    console_puts("\n");
    return 0;
}

int fputs(FILE* unused, const char * fmt)
{
    console_puts(fmt);
    return 0;
}

// no file I/O for now, but feel free to implement it

#define DUMMY(x) int x() { printf( #x "\n "); return 0; }

DUMMY(read)
DUMMY(lseek)
DUMMY(fclose)
DUMMY(fputc)
DUMMY(fwrite)
DUMMY(fdopen)
DUMMY(fopen)
DUMMY(open)
DUMMY(unlink)
DUMMY(getenv)
DUMMY(time)
DUMMY(localtime)
DUMMY(close)
DUMMY(getcwd)
int _impure_ptr;

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
