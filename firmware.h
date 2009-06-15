#ifndef _firmware_h_
#define _firmware_h_
/** \file
 * Firmware header file structures.
 *
 * This is a host-header!
 */

#include <stdint.h>
#include "compiler.h"

/** Firmware file header */
struct fw_header_t
{
	uint32_t	model_id;		// offset 0x00
	uint8_t		pad0[ 0x0C ];		// offset 0x04
	char		version[ 0x10 ];	// offset 0x10
	uint32_t	crc;			// offset 0x20
	uint32_t	flasher_offset;		// offset 0x24, points to 0xB0
	uint32_t	file_header_size;	// offset 0x28, must be 0x120
	uint32_t	some_size;		// offset 0x2C
	uint32_t	_data_offset;		// offset 0x30
	uint32_t	unknown1;		// offset 0x34
	uint32_t	file_size;		// offset 0x38
	uint32_t	unknown2;		// offset 0x3C
	uint32_t	sha1_hash;		// offset 0x40
	uint32_t	pad2[ 7 ];		// offset 0x44-0x5C
	uint32_t	data_offset;		// offset 0x60
	uint8_t		pad3[ 0x58 ];		// offset 0x64
	uint32_t	data_len;		// offset 0xBC
	uint8_t		pad4[ 0x60 ];		// offset 0xC0
} __attribute__((packed));

SIZE_CHECK_STRUCT( fw_header_t, 0x120 );

#endif
