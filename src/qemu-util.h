#ifndef _qemu_util_h
#define _qemu_util_h

#ifdef CONFIG_QEMU

#include <stdarg.h>

/** Some small engio API **/
#define REG_PRINT_CHAR 0xC0123400
#define REG_PRINT_NUM  0xC012340C
#define REG_DISAS_32   0xC0123410
#define REG_CALLSTACK  0xC0123430

/**
 * The following functions will print to QEMU console only when compiling
 * with CONFIG_QEMU=y. By default, they will not be compiled, therefore
 * not increasing the executable size; however, they will be type-checked.
 * 
 * You may need to use "make clean" before recompiling with CONFIG_QEMU=y.
 * 
 * The builds compiled with CONFIG_QEMU=y will *not* run on the camera!
 */

/* print non-formatted messages */
/* (useful for large strings without risking stack overflow) */
static inline int qprint(const char * msg)
{
    for (const char* c = msg; *c; c++)
    {
        *(volatile uint32_t*)REG_PRINT_CHAR = *c;
    }
    return 0;
}

/* from dryos.h
 * required to compile qprintf in bootloader context,
 * although you won't be able to use it (vsnprintf is in main firmware)
 * ... unless you can supply another vsnprintf, e.g. from dietlibc */
extern int vsnprintf( char* str, size_t n, const char* fmt, va_list ap );

/* print messages to the QEMU console */
static int qprintf(const char * fmt, ...)
{
    va_list ap;
    char buf[128];
    va_start( ap, fmt );
    vsnprintf( buf, sizeof(buf)-1, fmt, ap );
    va_end( ap );
    qprint(buf);
    return 0;
}

/* print a 32-bit number (also works very early in the boot process) */
/* number formatting done in QEMU */
static inline void qprintn(int32_t num)
{
    *(volatile uint32_t *)(REG_PRINT_NUM) = num;
}

/* disassemble a 32-bit address */
static inline void qdisas(uint32_t addr)
{
    *(volatile uint32_t *)(REG_DISAS_32) = addr;
}

#else /* without CONFIG_QEMU */

/* these don't execute anything on regular builds (without CONFIG_QEMU) */
/* also, the strings used as arguments won't be included in the compiled binary */
/* but the calls will still be type-checked, unlike ifdef */
static inline int qprintf(const char * fmt, ...) { return 0; }
static inline int qprint(const char * msg) { return 0; }
static inline void qprintn(int num) { }
static inline void qdisas(uint32_t addr) { }

#endif
#endif
