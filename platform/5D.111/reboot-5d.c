#include "arm-mcr.h"

asm(
".text\n"
".globl _start\n"
"_start:\n"
"   b 1f\n"
".ascii \"gaonisoy\"\n"     // 0x124, 128
"1:\n"
"MRS     R0, CPSR\n"
"BIC     R0, R0, #0x3F\n"   // Clear I,F,T
"ORR     R0, R0, #0xD3\n"   // Set I,T, M=10011 == supervisor
"MSR     CPSR, R0\n"
"   ldr sp, =0x1900\n"  // 0x130
"   mov fp, #0\n"
"   b cstart\n"
);

static void busy_wait(int n)
{
    int i,j;
    static volatile int k = 0;
    for (i = 0; i < n; i++)
        for (j = 0; j < 100000; j++)
            k++;
}

static void blink(int n)
{
    while (1)
    {
        *(int*)0xC02200A0 |= 2;  // card LED on
        busy_wait(n);
        *(int*)0xC02200A0 &= ~2; // card LED off
        busy_wait(n);
    }
}

void
__attribute__((noreturn))
cstart( void )
{
    blink(50);
    while(1);
}

