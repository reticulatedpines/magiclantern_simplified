@ compile with:
@ arm-none-eabi-gcc -Wl,-N,-Ttext,0x800000 -nostdlib -march=armv7 -mthumb cpuinfo.s -o cpuinfo.elf
@ and extract the needed bytes from the binary:
@ arm-none-eabi-objcopy -O binary cpuinfo.elf cpuinfo.bin
@ toolchain: [url]https://launchpad.net/gcc-arm-embedded[/url]
@ 0x00800000 is an arbitrary address, code is position independent anyway

.macro NHSTUB name addr
.globl _\name
.weak _\name
_\name:
ldr.w pc, _\addr
_\addr:
.long \addr
.endm

.globl _start
.org 0
_start:
.text
@ force thumb
.code 16
@ allow new instructions (ldr.w)
.syntax unified

MRC    p15, 0, R1,c0,c0 @ ident
STR    R1, [R0]

MRC    p15, 0, R1,c0,c0,1 @ CTR, cache
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c0,2 @ TCMTR, TCM
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c0,3 @ TLBTR
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c2,c0,2 @ TTBCR
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c2,c0,0 @ TTBR0
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c2,c0,1 @ TTBR1
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c0,5 @ MPIDR
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c0,6 @ REVIDR
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c1,0 @ ID_PFR0
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c1,1 @ ID_PFR1
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c1,2 @ ID_DFR0
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c1,3 @ ID_AFR0
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c1,4 @ ID_MMFR0
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c1,5 @ ID_MMFR1
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c1,6 @ ID_MMFR2
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c1,7 @ ID_MMFR3
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c2,0 @ ID_ISAR0
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c2,1 @ ID_ISAR1
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c2,2 @ ID_ISAR2
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c2,3 @ ID_ISAR3
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c2,4 @ ID_ISAR4
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c0,c2,5 @ ID_ISAR5
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 1, R1,c0,c0,1 @ CLIDR
ADD    R0, R0, #4
STR    R1, [R0]

MOV    R1, #0
MCR    p15, 2, R1,c0,c0,0 @ CSSELR (data cache, level0)

MRC    p15, 1, R1,c0,c0,0 @ CCSIDR (the currently selected one)
ADD    R0, R0, #4
STR    R1, [R0]

MOV    R1, #1
MCR    p15, 2, R1,c0,c0,0 @ CSSELR (inst cache, level0)

MRC    p15, 1, R1,c0,c0,0 @ CCSIDR (the currently selected one)
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c1,c0,0 @ SCTLR
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c1,c0,1 @ ACTLR
ADD    R0, R0, #4
STR    R1, [R0]

@#ifndef CONFIG_QEMU
MRC    p15, 0, R1,c15,c0,0 @ ACTLR2
ADD    R0, R0, #4
STR    R1, [R0]
@#endif

MRC    p15, 0, R1,c1,c0,2 @ CPACR
ADD    R0, R0, #4
STR    R1, [R0]

@"MRC    p15, 0, R1,c9,c1,1 @ ATCM region reg, Cortex R4
@"ADD    R0, R0, #4
@"STR    R1, [R0]

@"MRC    p15, 0, R1,c9,c1,0 @ BTCM region reg, Cortex R4
@"ADD    R0, R0, #4
@"STR    R1, [R0]

MRC    p15, 0, R1,c1,c1,2 @ NSACR
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c3,c0,0 @ DACR
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p14, 0, R1,c0,c0,0 @ DBGDIDR
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p14, 0, R1,c1,c0,0 @ DBGDRAR
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p14, 0, R1,c2,c0,0 @ DBGDSAR
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p14, 0, R1,c0,c1,0 @ DBGDSCR
ADD    R0, R0, #4
STR    R1, [R0]

@".word  0xEEF01A10 @"VMRS   R1, FPSID @ Floating Point System ID register
@"ADD    R0, R0, #4
@"STR    R1, [R0]

@".word  0xEEF71A10 @"VMRS   R1, MVFR0 @ Media and VFP Feature Register 0
@"ADD    R0, R0, #4
@"STR    R1, [R0]

@".word  0xEEF61A10 @"VMRS   R1, MVFR1 @ Media and VFP Feature Register 1
@"ADD    R0, R0, #4
@"STR    R1, [R0]

MRC    p15, 4, R1,c15,c0,0 @ config base addr reg (Cortex A9)
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c11,c0,0 @ PLEIDR (Cortex A9)
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c10,c0,0 @ TLB lockdown reg (Cortex A9)
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c10,c2,0 @ PRRR (Cortex A9)
ADD    R0, R0, #4
STR    R1, [R0]

MRC    p15, 0, R1,c10,c2,1 @ NMRR (Cortex A9)
ADD    R0, R0, #4
STR    R1, [R0]

BX     LR