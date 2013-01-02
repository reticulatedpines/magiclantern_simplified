/* This file is generated automatically by configure */
/* It is valid only for the system type i686-pc-linux-gnu */

#ifndef __BYTEORDER_H
#define __BYTEORDER_H

#ifdef WIN32
#include <winsock2.h>
#else
/* ntohl and relatives live here */
#include <arpa/inet.h>

/* Define generic byte swapping functions */
#include <byteswap.h>
#endif

#define swap16(x) bswap_16(x)
#define swap32(x) bswap_32(x)
#define swap64(x) bswap_64(x)

/* The byte swapping macros have the form: */
/*   EENN[a]toh or htoEENN[a] where EE is be (big endian) or */
/* le (little-endian), NN is 16 or 32 (number of bits) and a, */
/* if present, indicates that the endian side is a pointer to an */
/* array of uint8_t bytes instead of an integer of the specified length. */
/* h refers to the host's ordering method. */

/* So, to convert a 32-bit integer stored in a buffer in little-endian */
/* format into a uint32_t usable on this machine, you could use: */
/*   uint32_t value = le32atoh(&buf[3]); */
/* To put that value back into the buffer, you could use: */
/*   htole32a(&buf[3], value); */

/* Define aliases for the standard byte swapping macros */
/* Arguments to these macros must be properly aligned on natural word */
/* boundaries in order to work properly on all architectures */

/* Uncomment if system endian.h does not provide them */
#define htobe16(x) htons(x)
#define htobe32(x) htonl(x)
#define be16toh(x) ntohs(x)
#define be32toh(x) ntohl(x)

#define HTOBE16(x) (x) = htobe16(x)
#define HTOBE32(x) (x) = htobe32(x)
#define BE32TOH(x) (x) = be32toh(x)
#define BE16TOH(x) (x) = be16toh(x)

/* On little endian machines, these macros are null */
#define htole16(x)      (x)
#define htole32(x)      (x)
#define htole64(x)      (x)
#define le16toh(x)      (x)
#define le32toh(x)      (x)
#define le64toh(x)      (x)

#define HTOLE16(x)      (void) (x)
#define HTOLE32(x)      (void) (x)
#define HTOLE64(x)      (void) (x)
#define LE16TOH(x)      (void) (x)
#define LE32TOH(x)      (void) (x)
#define LE64TOH(x)      (void) (x)

/* Uncomment if system endian.h does not provide them */
//#define htobe64(x)      swap64(x)
//#define be64toh(x)      swap64(x)

#define HTOBE64(x)      (x) = htobe64(x)
#define BE64TOH(x)      (x) = be64toh(x)

/* Define the C99 standard length-specific integer types */
#include "libptp-stdint.h"

/* Here are some macros to create integers from a byte array */
/* These are used to get and put integers from/into a uint8_t array */
/* with a specific endianness.  This is the most portable way to generate */
/* and read messages to a network or serial device.  Each member of a */
/* packet structure must be handled separately. */

/* The i386 and compatibles can handle unaligned memory access, */
/* so use the optimized macros above to do this job */
#define be16atoh(x)     be16toh(*(uint16_t*)(x))
#define be32atoh(x)     be32toh(*(uint32_t*)(x))
#define be64atoh(x)     be64toh(*(uint64_t*)(x))
#define le16atoh(x)     le16toh(*(uint16_t*)(x))
#define le32atoh(x)     le32toh(*(uint32_t*)(x))
#define le64atoh(x)     le64toh(*(uint64_t*)(x))

#define htobe16a(a,x)   *(uint16_t*)(a) = htobe16(x)
#define htobe32a(a,x)   *(uint32_t*)(a) = htobe32(x)
#define htobe64a(a,x)   *(uint64_t*)(a) = htobe64(x)
#define htole16a(a,x)   *(uint16_t*)(a) = htole16(x)
#define htole32a(a,x)   *(uint32_t*)(a) = htole32(x)
#define htole64a(a,x)   *(uint64_t*)(a) = htole64(x)

#endif /*__BYTEORDER_H*/
