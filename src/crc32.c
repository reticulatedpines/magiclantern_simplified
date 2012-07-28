/* CRC32 code copied from ransrid (Redundant Array of Non-Striped Really Independent Disks)
 *
 * License: GPL v2
 * Author:  Matthias Hopf <mat@mshopf.de>
 */

/*
 * CRC32 functions
 * Based on public domain implementation by Finn Yannick Jacobs.
 */

/* Written and copyright 1999 by Finn Yannick Jacobs
 * No rights were reserved to this, so feel free to
 * manipulate or do with it, what you want or desire :)
 */

#include "crc32.h"

/* crc32table[] built by crc32_init() */
static uint32_t crc32table[256];

/* Calculate crc32. Little endian.
 * Standard seed is 0xffffffff or 0.
 * Some implementations xor result with 0xffffffff after calculation. */
uint32_t crc32 (void *data, unsigned int len, uint32_t seed)
{
  uint8_t *d = data;
  while (len--)
    seed = ((seed>>8) & 0x00FFFFFF) ^ crc32table [(seed ^ *d++) & 0xFF];
  return seed;
}

/* Calculate crc32table */
void crc32_init()
{
  uint32_t poly = 0xEDB88320L;
  uint32_t crc;
  int i, j;

  for (i=0; i<256; i++) {
    crc = i;
    for (j=8; j>0; j--)
      crc = (crc>>1) ^ ((crc&1) ? poly : 0);
    crc32table[i] = crc;
  }
}
