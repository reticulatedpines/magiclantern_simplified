.text
.org 0
.align 2
.globl _start
 
// stubs_2_elf.S
// (c) 2008 chr
// GPL v3+
// create elf file from a binary blob
// and mix with symbols from stubs files
 
// compile with:
// arm-linux-gnu-gcc -Wl,-N,-Ttext,0xff810000 -nostdlib stubs_2_elf.S -o rom.elf
 
 
// cheat around gdb ignoring absolute symbols
here = .
#define offs BASE
//#define offs 0
#define NSTUB(addr, name) \
	.global name; \
	name = here + addr - BASE;

#define NHSTUB NSTUB

 
// include rom dump
_start:
.text

// include translated file
#include "stubs.S"

blob_start:
.incbin "ROM0.bin"
blob_end:
