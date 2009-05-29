/** \file
 * Initial entry point for relocated code.
 *
 * Modern gcc will re-order the functions, so this is necessary to have
 * as the very first entry point.
 */
.text
.org 0
.globl _start
_start:
	B	copy_and_restart
