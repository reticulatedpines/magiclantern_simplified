    .text
    .org 0
    .globl _start, start

start:
_start:
		 B	 main

    .org 0x10
_mgr_loop1:
		 B       _mgr_loop1


    .org 0x20
_mgr_loop2:
		 B       _mgr_loop2
