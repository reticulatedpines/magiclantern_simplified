#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "eos.h"

#include "debug_message_helper.h"


// ANSI Colors
#define KRED   "\x1B[1;31m"
#define KCYN   "\x1B[1;36m"
#define KWHT   "\x1B[1;37m"
#define KRESET "\033[0m"


// Semaphore tag
#define SEM_TAG   KRED "[SEM]" KRESET

unsigned int eos_handle_gdb_helpers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    // 'set *0xCF999001 = 1'
    if (type & MODE_WRITE) {
        // This would be better (as we can pass a value), but it does not work
        printf("REG_GDB_FLAG: %d\n",value);
    }

    // 'x *0xCF999001'
    else {
        switch (address) {
            case REG_GDB_DEBUG_MSG:
                eos_debug_message(s, 1);
                break;

            case REG_GDB_SEM_NEW_IN:
            case REG_GDB_SEM_NEW_OUT:
            case REG_GDB_SEM_TAKE_IN:
            case REG_GDB_SEM_TAKE_OUT:
            case REG_GDB_SEM_GIVE:
                eos_debug_semaphore_tracker(s,address);
                break;

            default:
                if ((address % 4) == 0)
                    printf("[EOS-GDB] Invalid address 0x%X\n", address);
                break;
        }
    }
    return 0;
}

// This wrapper is kind of ugly, but it allows us to print most debug messages to stdout
// without using guest code injection. It can be triggered by executing: "x 0xCF999004"
// in gbd. Ideally we should use "set *0xCF999004 = 1", but qemu doesn't call this function
// when this is done.

void eos_debug_message(EOSState * s, int colorize)
{
    uint32_t r0 = s->cpu->env.regs[0]; // id 1
    uint32_t r1 = s->cpu->env.regs[1]; // id 2
    uint32_t r2 = s->cpu->env.regs[2]; // format string address
    uint32_t r3 = s->cpu->env.regs[3]; // first argument
    uint32_t sp = s->cpu->env.regs[13]; // stack pointer

    char format_string[128]; // 128 bytes should be enough for anyone...
    int arg_i = 0;
    char c;
    uint32_t address = r2;

    printf("[DebugMsg] (%d,%d) ", r0, r1);
    if (colorize) printf(KWHT);

    c = eos_get_mem_b(s, address++);
    while (c)
    {
        // Print until '%' or '\0'
        while (c != '\0' && c != '%')
        {
            if (c == '\n') {
                if (colorize) printf(KRESET);
                printf("\n[DebugMsg] (%d,%d) ", r0, r1);
                if (colorize) printf(KWHT);
            }
            else if (c != '\r')
                putchar(c);
            c = eos_get_mem_b(s, address++);
        }
        
        if (c == '%')
        {
            int n = 1;
            int is_long;
            int format;
            uint32_t arg;
            format_string[0] = '%';
            do
            {
                c = eos_get_mem_b(s, address++);
                format_string[n++] = c;
            } while (n < COUNT(format_string) && c != '\0' && !strchr("diuoxXsp%", c));

            if (c == '\0')
            {
                if (colorize) printf(KWHT);
                printf("%s", format_string);
                continue;
            }

            c = eos_get_mem_b(s, address++);

            // Skip if it fills format buffer or is {long long} or {short} type
            // (I've never seen those in EOS code)
            if (n == COUNT(format_string) || format_string[n-3] == 'h' || (n >= 4 && format_string[n-3] == 'l'))
            {
                if (colorize) printf(KRED);
                printf("[FORMATTING_ERROR]");
                if (colorize) printf(KWHT);
                printf("%.*s", n, format_string);
                continue;
            }

            format_string[n] = '\0';
            format  = format_string[n-1];
            is_long = (strchr("sp%", format) == NULL && (format_string[n-2] == 'l'));

            // Only parse "%s", other variants (eg "%20s") may expect additional parameters
            // or non zero-terminated strings.
            if (format == 's' && strcmp(format_string, "%s") != 0)
            {
                if (colorize) printf(KRED);
                printf("[FORMATTING_ERROR]");
                if (colorize) printf(KCYN);
                printf("%s", format_string);
                if (colorize) printf(KWHT);
                break;
            }

            if (colorize) printf(KCYN);

            // note: all ARM types {int, long, void*} are of size 32 bits,
            //       and {char, short} should be expanded to 32 bits.
            //       the {long long} is not included in this code.
            arg = (arg_i == 0) ? r3 : eos_get_mem_w(s, sp + 4 * (arg_i-1));
            arg_i++;

            //if (colorize) printf(KRED);
            //printf("[%s|%lX] ", format_string, arg);
            //if (colorize) printf(KCYN);
            if (format == 's')
            {
                uint32_t sarg = arg;
                char t = eos_get_mem_b(s, sarg++);
                while (t != '\0')
                {
                    if (t == '\n') {
                        if (colorize) printf(KRESET);
                        printf("\n[DMSG:%d,%d] ", r0, r1);
                        if (colorize) printf(KCYN);
                    }
                    else if (t != '\r')
                        putchar(t);
                    t = eos_get_mem_b(s, sarg++);
                }
            }
            else if (!is_long)
                printf(format_string, (unsigned int)arg);
            else 
                printf(format_string, (unsigned long int)arg);

            //printf("[%s:%X]",format_string,(uint32_t)arg);

            if (colorize) printf(KWHT);
        }
    }
    if (colorize) printf(KRESET);
    printf("\n");
}


void eos_debug_semaphore_tracker(EOSState * s, int event)
{
    static int semaphore_count = 0;
    static struct {
        uint32_t semaphore, state;
        char * name;
    } * sem = NULL;
    uint32_t r0 = s->cpu->env.regs[0];
    uint32_t r1 = s->cpu->env.regs[1];
    uint32_t lr = s->cpu->env.regs[14];
    int i;
    
    switch (event)
    {
        case REG_GDB_SEM_NEW_IN:
            semaphore_count++;
            sem = realloc(sem, semaphore_count * sizeof(sem[0]));
            if (sem != NULL)
            {
                int len = 0;
                char * name;
                while (len < 64 && eos_get_mem_b(s, r0 + len))
                    len++;
                name = malloc(len+1);
                for (i = 0; i < len; i++)
                    name[i] = eos_get_mem_b(s, r0 + i);
                name[len] = '\0';
                sem[semaphore_count-1].semaphore = 0xffffffff;
                sem[semaphore_count-1].state = r1;
                sem[semaphore_count-1].name = name;
                printf(SEM_TAG "  [0x%X] CreateSemaphore(name=\"%s\",state=%d)\n", lr, name, r1);
            }
            break;

        case REG_GDB_SEM_NEW_OUT:
            if (sem != NULL)
            {
                char * name = sem[semaphore_count-1].name;
                sem[semaphore_count-1].semaphore = r0;
                printf(SEM_TAG "  Created semaphore: \"%s\" (id=0x%X)\n", name, r0);
            }
            break;

        case REG_GDB_SEM_TAKE_IN:
            printf(SEM_TAG "  [0x%X] TakeSemaphore(id=0x%X) ", lr, r0);
            for (i = 0; i < semaphore_count; i++)
            {
                if (sem[i].semaphore == r0)
                {
                    printf("[name=\"%s\",state=%d]\n", sem[i].name, sem[i].state);
                    //sem[i].state = 1;
                    return;
                }
            }
            printf("[name=?,state=?]\n");
            break;

        case REG_GDB_SEM_TAKE_OUT:
            // We expect r1 to be the semaphore. This has to be handled by GDB,
            // since different ROMS cannot be expected to store it in the same
            // register.
            for (i = 0; i < semaphore_count; i++)
            {
                if (sem[i].semaphore == r1)
                {
                    printf(SEM_TAG "  Got semaphore(id=0x%X) [name=\"%s\",ret=%d]\n", 
                           r1, sem[i].name, r0);
                    sem[i].state = 1; // FIXME: if ok
                    return;
                }
            }
            printf(SEM_TAG "  Got semaphore(id=0x%X) [name=?,ret=%d]\n", r1, r0);
            break;

        case REG_GDB_SEM_GIVE:
            printf(SEM_TAG "  [0x%X] GiveSemaphore(id=0x%X) ", lr, r0);
            for (i = 0; i < semaphore_count; i++)
            {
                if (sem[i].semaphore == r0)
                {
                    printf("[name=\"%s\",state=%d]\n", sem[i].name, sem[i].state);
                    sem[i].state = 0;
                    return;
                }
            }
            printf("[name=?,state=?]\n");
            break;

        case REG_GDB_SEM_RESET:
            for (i = 0; i < semaphore_count; i++)
                if(sem[i].name)
                    free(sem[i].name);
            free(sem);
            sem = NULL;
            semaphore_count = 0;
            break;
    }
}

