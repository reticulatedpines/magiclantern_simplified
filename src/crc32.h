/* CRC32 code copied from ransrid (Redundant Array of Non-Striped Really Independent Disks)
 *
 * License: GPL v2
 * Author:  Matthias Hopf <mat@mshopf.de>
 */

/*
 * CRC32 functions
 * Based on public domain implementation by Finn Yannick Jacobs.
 */

#ifndef _CRC32_H_
#define _CRC32_H_

#include <stdint.h>

#define CRC32_DEFAULT_SEED 0xffffffff

/* Calculate crc32.
 * Standard seed is 0xffffffff or 0.
 * Some implementations xor result with 0xffffffff after calculation. */
uint32_t crc32 (void *data, unsigned int len, uint32_t seed);

/* Calculate crc32table */
void crc32_init();

#endif
