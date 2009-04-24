    .text
    .org 0
    .globl _start

_start:
		 B	 main

    .org 0x10
_mgr_loop1:
		 B       _mgr_loop1


    .org 0x20
_mgr_loop2:
		 B       _mgr_loop2

#define SYMBOL( addr, name ) \
	name = addr; \
	.global name; \

SYMBOL( 0x800794, panic )
SYMBOL( 0x8001e4, _dryos_start )
SYMBOL( 0x805ab8, update_manager )
SYMBOL( 0x800cc8, Debug )
SYMBOL( 0x8060b8, shutdown )
SYMBOL( 0x82df2c, open )
SYMBOL( 0x82dfe0, creat )
SYMBOL( 0x82e4c8, write )
SYMBOL( 0x82e088, close )
SYMBOL( 0x83C634, Assert0 )
SYMBOL( 0x838F6C, Assert1 )
SYMBOL( 0x857824, RegisterClient )
SYMBOL( 0x8B07FC, assert )
SYMBOL( 0x8B0868, __assert )
SYMBOL( 0x8B0FE8, __putc )
SYMBOL( 0x8B0FB8, puts )
