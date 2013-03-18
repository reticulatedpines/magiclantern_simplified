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

char my_program[] =
"int fib(int n)\n"
"{\n"
"    if (n <= 2)\n"
"        return 1;\n"
"    else\n"
"        return fib(n-1) + fib(n-2);\n"
"}\n"
"\n"
"int foo(int n)\n"
"{\n"
"    printf(\"Hello World!\\n\");\n"
"    printf(\"fib(%d) = %d\\n\", n, fib(n));\n"
"    printf(\"add(%d, %d) = %d\\n\", n, 2 * n, add(n, 2 * n));\n"
"    return 0;\n"
"}\n";

/* call this from don't click me */
int tcc_hello()
{
    /* display the script console */
    console_show();
    msleep(1000);
    
    TCCState *s;
    int (*func)(int);

    /* create a TCC compilation context */
    s = tcc_new();
    if (!s)
        return 1;
    
    /* do not load standard libraries - we don't have them */
    tcc_set_options(s, "-nostdinc -nostdlib");

    /* MUST BE CALLED before any compilation */
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    /* compile our program from string */
    if (tcc_compile_string(s, my_program) == -1)
        return 1;

    /* as a test, we add a symbol that the compiled program can use.
       You may also open a dll with tcc_add_dll() and use symbols from that */
    tcc_add_symbol(s, "add", add);

    /* relocate the code */
    if (tcc_relocate(s, TCC_RELOCATE_AUTO) < 0)
        return 1;

    /* get entry symbol */
    func = tcc_get_symbol(s, "foo");
    if (!func)
        return 1;
    
    /* enable the caching bit (todo: use AllocateMemory instead of alloc_dma_memory) */
    func = CACHEABLE(func);
    
    /* run the code */
    func(32);

    /* delete the compilation state */
    tcc_delete(s);

    /* hide the script console */
    msleep(10000);
    console_hide();
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

/* fixme: \n at the end of line not working? */
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
