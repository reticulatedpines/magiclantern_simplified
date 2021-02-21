/* $Id: bigd.h $ */

/******************** SHORT COPYRIGHT NOTICE**************************
This source code is part of the BigDigits multiple-precision
arithmetic library Version 2.4 originally written by David Ireland,
copyright (c) 2001-13 D.I. Management Services Pty Limited, all rights
reserved. It is provided "as is" with no warranties. You may use
this software under the terms of the full copyright notice
"bigdigitsCopyright.txt" that should have been included with this
library or can be obtained from <www.di-mgt.com.au/bigdigits.html>.
This notice must always be retained in any copy.
******************* END OF COPYRIGHT NOTICE***************************/
/*
    Last updated:
	$Date: 2013-04-27 17:19:00 $
	$Revision: 2.4.0 $
	$Author: dai $
*/

/* ANNOTATION FOR DOXYGEN... */

/** @mainpage notitle

\section intro_sec Introduction

BigDigits is a free library of multiple-precision arithmetic routines written in ANSI C 
to carry out calculations with the large natural numbers you use in cryptography.

You can download it from http://www.di-mgt.com.au/bigdigits.html.

The BigDigits library is designed to work with the set of natural numbers \b N; 
that is, the non-negative integers 0,1,2,... . 

The library has two interfaces: the "bd" library and the underlying "mp" library.
The "bd" functions use an opaque BIGD object and
handle memory allocation automatically.
The "mp" functions use fixed-length arrays and
require a bit more care to use but the "mp" library is faster and
has an option (\c NO_ALLOCS) to avoid all memory allocation calls completely.
The "bd" library is an easier-to-use wrapper around the "mp" library.

The "bd" functions are described in bigd.h and the more elementary "mp" functions are in bigdigits.h.

There are some optional functions that generate random numbers using a "pretty-good" internal RNG 
in bigdRand.h and bigdigitsRand.h for the "bd" and "mp" libraries, respectively.

*/

/** @file
    Interface to the BigDigits "bd" functions using BIGD objects with automatic memory allocation

@par PROGRAMMER'S NOTES
Where the function computes a new BIGD value,
the result is returned in the first argument.
Some functions do not allow variables to overlap.
Functions of type \c int generally return 0 to denote success 
but some return True/False (1/0) or borrow (+1) or error (-1).
Functions of type \c size_t (an unsigned int) return a length.

@par Memory allocation
Memory for each variable is allocated as required and is increased automatically if needed.
However, memory for a variable is only released when freed with bdFree(). 
The standard code for a variable \c b should be as follows:
@code
	BIGD b;
	b = bdNew();
	// operations using b...
	bdFree(&b);
@endcode*/


#ifndef BIGD_H_
#define BIGD_H_ 1

#include "bigdtypes.h"

/**** USER CONFIGURABLE SECTION ****/

/* [v2.1] Changed to use exact width integer types.
   [v2.2] Moved macros for exact-width types to "bigdtypes.h"
*/

/** A synonym for a single digit (DIGIT_T is not exposed) */
typedef uint32_t bdigit_t;

/**** END OF USER CONFIGURABLE SECTION ****/
/** @cond */
/**** OPTIONAL PREPROCESSOR DEFINITIONS ****/
/* 
   Choose one of { USE_SPASM | USE_64WITH32 }
   USE_SPASM: to use the faster x86 ASM routines (if __asm option is available with your compiler).
   USE_64WITH32: to use the 64-bit integers if available (e.g. long long).
   Default: use default internal routines spDivide and spMultiply.
   The USE_SPASM option takes precedence over USE_64WITH32.
*/

#ifdef NO_ALLOCS 
#error NO_ALLOCS is not permitted with bd functions
#endif

/*
This interface uses opaque pointers of type BIGD using
the conventions in "C Interfaces and Implementions" by
David R. Hanson, Addison-Wesley, 1996, pp21-4.
Thanks to Ian Tree for the C++ fudge.
*/
#define T BIGD
#ifdef __cplusplus
	typedef struct T2 *T;
#else
	typedef struct T *T;
#endif

#ifdef __cplusplus
extern "C" {
#endif
/** @endcond */


/******************************/
/* CONSTRUCTOR AND DESTRUCTOR */
/******************************/
/** Create new BIGD object
@returns Handle to new object
*/
BIGD bdNew(void);

/** Destroy BIGD object: zeroise contents and free allocated memory.
@remark Pass a <em>pointer</em> to the handle
*/
void bdFree(BIGD *bd);	

/*************************/
/* ARITHMETIC OPERATIONS */
/*************************/

/** Compute sum w = u + v
@remark \c w and \c v must be separate variables. 
Use bdAdd_s() to avoid overlap restriction.
*/
int bdAdd(BIGD w, BIGD u, BIGD v);

/** Compute difference w = u - v
@return borrow
@remark \c w and \c v must be separate variables.
Use bdSubtract_s() to avoid overlap restriction.
@warning Behaviour is undefined for negative numbers.
*/
int bdSubtract(BIGD w, BIGD u, BIGD v);

/** Compute product w = u * v
@remark \c w and \c u must be separate variables.
Use bdMultiply_s() to avoid overlap restriction.
*/
int bdMultiply(BIGD w, BIGD u, BIGD v);

/** Compute integer division of u by v such that u=qv+r
@param[out] q To receive quotient = u div v
@param[out] r To receive remainder = u mod v
@param[in] u Dividend
@param[in] v Divisor
@remark Trashes q and r first: \c q and \c r must be separate variables.
Use bdDivide_s() to avoid overlap restriction.
*/
int bdDivide(BIGD q, BIGD r, BIGD u, BIGD v);

/** Compute remainder r = u mod v
@remark \c r and \c u must be separate variables.
Use bdModulo_s() to avoid overlap restriction.
*/
int bdModulo(BIGD r, BIGD u, BIGD v);

/** Compute square w = x^2
@remark \c w and \c x must be separate variables.
Use bdSquare_s() to avoid overlap restriction.
*/
int bdSquare(BIGD w, BIGD x);

/** Computes y = g^n (up to available memory!) 
@remark Be very careful that \c n is not too large
*/
int bdPower(BIGD y, BIGD g, unsigned short int n);

/** Computes integer square root s = floor(sqrt(x)) */
int bdSqrt(BIGD s, BIGD x);

/** Computes integer cube root s = floor(cuberoot(x)) */
int bdCubeRoot(BIGD s, BIGD x);

/** Sets a = a + 1
@return carry */
int bdIncrement(BIGD a);

/** Sets a = a - 1
@return borrow 
@warning Behaviour is undefined if a becomes negative.
*/
int bdDecrement(BIGD a);

/* 'Safe' versions with no restrictions on overlap */
/* [v2.2] Changed name from ~Ex to ~_s */

/** Compute w = u + v without restrictions on overlap of bdAdd() */
int bdAdd_s(BIGD w, BIGD u, BIGD v);
/** Compute w = u - v without restrictions on overlap of bdSubtract() */
int bdSubtract_s(BIGD w, BIGD u, BIGD v);
/** Compute w = u * v without restrictions on overlap of bdMultiply() */
int bdMultiply_s(BIGD w, BIGD u, BIGD v);
/** Compute u div v without restrictions on overlap of bdDivide() */
int bdDivide_s(BIGD q, BIGD r, BIGD u, BIGD v);
/** Compute u mod v without restrictions on overlap of bdModulo() */
int bdModulo_s(BIGD r, BIGD u, BIGD v);
/** Compute s = x * x without restrictions on overlap of bdSquare() */
int bdSquare_s(BIGD s, BIGD x);

/* Keep the faith with the deprecated ~Ex version names */
#define bdAddEx(w, u, v) bdAdd_s((w), (u), (v))
#define bdSubtractEx(w, u, v) bdSubtract_s((w), (u), (v))
#define bdMultiplyEx(w, u, v) bdMultiply_s((w), (u), (v))
#define bdDivideEx(q, r, u, v) bdDivide_s((q), (r), (u), (v))
#define bdModuloEx(r, u, v) bdModulo_s((r), (u), (v))
#define bdSquareEx(s, x) bdSquare_s((s), (x))

/*************************/
/* COMPARISON OPERATIONS */
/*************************/

/** Returns true (1) if a equals b, else false (0) */
int bdIsEqual(BIGD a, BIGD b);

/** Returns sign {0,1,-1} of (a-b) */
int bdCompare(BIGD a, BIGD b);

/** Returns true (1) if a is zero, else false (0) */
int bdIsZero(BIGD a);

/** Returns true (1) if a is even, else false (0) */
int bdIsEven(BIGD a);

/** Returns true (1) if a is odd, else false (0) */
int bdIsOdd(BIGD a);

/*************************/
/* ASSIGNMENT OPERATIONS */
/*************************/

/** Sets a = b */
int bdSetEqual(BIGD a, BIGD b);

/** Sets a = 0 */
int bdSetZero(BIGD a);

/****************************/
/* NUMBER THEORY OPERATIONS */
/****************************/

/** Computes y = x^e mod m */
int bdModExp(BIGD y, BIGD x, BIGD e, BIGD m);

/** Computes a = (x * y) mod m */
int bdModMult(BIGD a, BIGD x, BIGD y, BIGD m);

/** Computes the inverse of \c a modulo \c m, x = a^{-1} mod m */
int bdModInv(BIGD x, BIGD a, BIGD m);

/** Computes g = gcd(x, y), the greater common divisor of x and y */
int bdGcd(BIGD g, BIGD x, BIGD y);

/** Returns the Jacobi symbol (a/n) = {-1, 0, +1} 
@remark If n is prime then the Jacobi symbol becomes the Legendre symbol (a/p) defined to be
- (a/p) = +1 if a is a quadratic residue modulo p
- (a/p) = -1 if a is a quadratic non-residue modulo p
- (a/p) = 0 if a is divisible by p
*/
int bdJacobi(BIGD a, BIGD n);

/** Returns true (1) if \c b is probably prime 
@param[in] b Number to test
@param[in] ntests The count of Rabin-Miller primality tests to carry out (recommended at least 80)
@returns true (1) if b is probably prime otherwise false (0)
@remark Uses FIPS-186-2/Rabin-Miller with trial division by small primes, 
which is faster in most cases than bdRabinMiller().
@see bdRabinMiller().
*/
int bdIsPrime(BIGD b, size_t ntests);

/** Returns true (1) if \c b is probably prime using just the Rabin-Miller test
@param[in] b Number to test (b > 2)
@param[in] ntests The count of Rabin-Miller primality tests to carry out (recommended at least 80)
@returns true (1) if b is probably prime otherwise false (0)
@remark bdIsPrime() is the recommended function to test for primality. 
Use this variant if, for some reason, 
you do not want to do the quicker trial division tests first.
@see bdIsPrime()
*/
int bdRabinMiller(BIGD b, size_t ntests);

/**********************************************/
/* FUNCTIONS THAT OPERATE WITH A SINGLE DIGIT */
/**********************************************/

/** Converts a single digit into a BIGD
@param[out] b To receive the result
@param[in] d A single digit
@remark Use this to set a BIGD variable to a small integer.
*/
int bdSetShort(BIGD b, bdigit_t d);

/** Computes w = u + d where d is a single digit */
int bdShortAdd(BIGD w, BIGD u, bdigit_t d);

/** Computes w = u - d where d is a single digit 
@return borrow
*/
int bdShortSub(BIGD w, BIGD u, bdigit_t d);

/** Computes w = u * d where d is a single digit */
int bdShortMult(BIGD w, BIGD u, bdigit_t d);

/** Computes integer division of u by single digit d
@param[out] q To receive quotient = u div d
@param[out] r To receive remainder = u mod d
@param[in] u Dividend, a BIGD object
@param[in] d Divisor, a single digit
@remark A separate BIGD object \c r must be provided to receive the remainder.
*/
int bdShortDiv(BIGD q, BIGD r, BIGD u, bdigit_t d);

/** Computes r = u mod d where d is a single digit */
bdigit_t bdShortMod(BIGD r, BIGD u, bdigit_t d);

/** Returns sign of (a-d) where d is a single digit */
int bdShortCmp(BIGD a, bdigit_t d);

/***********************/
/* BIT-WISE OPERATIONS */
/***********************/
/* [v2.1.0] Added ModPowerOf2, Xor, Or and AndBits functions */
/* [v2.2.0] Added NotBits, GetBit, SetBit functions */

/** Returns number of significant bits in b */
size_t bdBitLength(BIGD b);

/** Set bit n of a (0..nbits-1) with value 1 or 0 */
int bdSetBit(BIGD a, size_t n, int value);

/** Returns value 1 or 0 of bit n of a (0..nbits-1) */
int bdGetBit(BIGD a, size_t ibit);

/** Computes a = b << n  */
void bdShiftLeft(BIGD a, BIGD b, size_t n);

/** Computes a = b >> n */
void bdShiftRight(BIGD a, BIGD b, size_t n);

/** Computes a = a mod 2^L, ie clears all bits greater than L */
void bdModPowerOf2(BIGD a, size_t L);

/** Computes bitwise operation a = b XOR c */
void bdXorBits(BIGD a, BIGD b, BIGD c);

/** Computes bitwise operation a = b OR c */
void bdOrBits(BIGD a, BIGD b, BIGD c);

/** Computes bitwise operation a = b AND c */
void bdAndBits(BIGD a, BIGD b, BIGD c);

/** Computes bitwise a = NOT b 
@remark This flips all the bits up to the most significant digit, which may not be exactly what you want. */
void bdNotBits(BIGD a, BIGD b);

/*******************/
/* MISC OPERATIONS */
/*******************/

/** Returns number of significant digits in b */
size_t bdSizeof(BIGD b);

/** Print b in hex format with optional prefix and suffix strings */
void bdPrintHex(const char *prefix, BIGD b, const char *suffix);

/** Print b in decimal format with optional prefix and suffix strings */
void bdPrintDecimal(const char *prefix, BIGD b, const char *suffix);

/** Print b in bit (0/1) format with optional prefix and suffix strings */
void bdPrintBits(const char *prefix, BIGD b, const char *suffix);

/* Options for bdPrint */
#define BD_PRINT_NL   0x1	/* append a newline after printing */
#define BD_PRINT_TRIM 0x2	/* trim leading zero digits */

/** Print b in hex format to stdout
@param[in] b Number to print
@param[in] flags (see below)
@remark Flags are
- \c 0 default display leading zeros but no newline
- \c BD_PRINT_NL \c 0x1 append a newline after printing
- \c BD_PRINT_TRIM \c 0x2 trim leading zero digits
@deprecated Use bdPrintHex() or bdPrintDecimal().
*/
void bdPrint(BIGD b, size_t flags);

/***************/
/* CONVERSIONS */
/***************/
/* All bdConv functions return 0 on error */
/* `Octet' = an 8-bit byte */

/** Converts a byte array into a BIGD object
@param[out] b To receive the result
@param[in] octets Pointer to array of bytes (octets)
@param[in] nbytes Number of bytes in sequence
@returns Number of digits actually set, which may be larger than bdSizeof(BIGD b).
*/
size_t bdConvFromOctets(BIGD b, const unsigned char *octets, size_t nbytes);

/** Converts a BIGD into an array of bytes, in big-endian order
@param[in] b BIGD object to be converted
@param[out] octets Array of bytes (octets) to receive output
@param[in] nbytes Length of array to receive output
@returns Exact number of significant bytes in the result. 
	If \c octets is NULL or \c nbytes is zero then just returns required size (nsig).
@remark Will pad on the left with zeros if \c nbytes is larger than required,
	or will truncate from the left if it is too short.
- if nbytes = nsig, octets <--    mmmnnn;
- if nbytes > nsig, octets <-- 000mmmnnn;
- if nbytes < nsig, octets <--       nnn. 
*/
size_t bdConvToOctets(BIGD b, unsigned char *octets, size_t nbytes);


/** Convert string of hexadecimal characters into a BIGD object
@param[out] b To receive the result
@param[in] s Null-terminated string of hex digits \c [0-9A-Fa-f]
@returns Number of significant digits actually set, which could be 0.
*/
size_t bdConvFromHex(BIGD b, const char *s);

/** Convert BIGD object into a string of hexadecimal characters
@param[in] b BIGD object to be converted
@param[out] s String buffer to receive output
@param[in] smax Size of buffer \e including the terminating zero
@returns Exact number of characters required \e excluding the terminating zero. 
	If \c s is NULL or \c smax is zero then just returns required number.
@code
	char *s;
	size_t nchars = bdConvToHex(b, NULL, 0);
	s = malloc(nchars+1);  // NB add one extra
	nchars = bdConvToHex(b, s, nchars+1);
@endcode
@remark Always returns the length of the string it tried to create, 
	even if the actual output was truncated, so remember to check.
*/
size_t bdConvToHex(BIGD b, char *s, size_t smax);

/** Convert string of decimal characters into a BIGD object
@param[out] b To receive the result
@param[in] s Null-terminated string of decimal digits \c [0-9]
@returns Number of significant digits actually set, which could be 0.
*/
size_t bdConvFromDecimal(BIGD b, const char *s);

/** Convert BIGD object into a string of decimal characters
@param[in] b BIGD object to be converted
@param[out] s String buffer to receive output
@param[in] smax Size of buffer \e including the terminating zero
@returns Exact number of characters required \e excluding the terminating zero. 
	If \c s is NULL or \c smax is zero then just returns required number.
@code
	char *s;
	size_t nchars = bdConvToDecimal(b, NULL, 0);
	s = malloc(nchars+1);  // NB add one extra
	nchars = bdConvToDecimal(b, s, nchars+1);
@endcode
@remark Always returns the length of the string it tried to create, 
	even if the actual output was truncated, so remember to check.
*/
size_t bdConvToDecimal(BIGD b, char *s, size_t smax);

/****************************/
/* RANDOM NUMBER OPERATIONS */
/****************************/

/* [Version 2.1: bdRandDigit and bdRandomBits moved to bigdRand.h] */

/** Set a with a "random" value of random length up to ndigits long suitable for quick tests 
 *  @return Number of digits actually set
 *  @remark For testing only. Not crypto secure. Not thread safe.
 *  @remark Fills \c a with 1 to \c ndigits pseudo-random digits and clears a random number of
 *  bits in the most significant digit. */
size_t bdSetRandTest(BIGD a, size_t ndigits);

/** Generate a quick-and-dirty random number a of bit length at most \c nbits using plain-old-rand 
 *  @return Number of digits actually set
 *  @remark Not crypto secure. May give same value on repeated calls.  
 *  @see bdRandomBits() or bdRandomSeeded().
 */
size_t bdQuickRandBits(BIGD a, size_t nbits);

/** TYPEDEF for user-defined random byte generator function */
typedef int (* BD_RANDFUNC)(unsigned char *buf, size_t nbytes, const unsigned char *seed, size_t seedlen);

/** Generate a random number nbits long using RandFunc
 *  @param[out] a To receive the result
 *  @param [in] nbits Maximum number of bits in number
 *  @param [in] seed Optional seed to add extra entropy
 *  @param [in] seedlen Number of bytes in seed
 *  @param [in] RandFunc User function to generate random bytes */
int bdRandomSeeded(BIGD a, size_t nbits, const unsigned char *seed, 
	size_t seedlen, BD_RANDFUNC RandFunc);

/** Generate a prime number
 *  @param[out] a To receive the result
 *  @param [in] nbits Maximum number of bits in number
 *  @param [in] ntests Number of primality tests to carry out (recommended at least 80)
 *  @param [in] seed Optional seed to add extra entropy
 *  @param [in] seedlen Number of bytes in seed
 *  @param [in] RandFunc User function to generate random bytes */
int bdGeneratePrime(BIGD a, size_t nbits, size_t ntests, const unsigned char *seed, 
	size_t seedlen, BD_RANDFUNC RandFunc);

/****************/
/* VERSION INFO */
/****************/
/** Returns version number = major*1000+minor*100+release*10+PP_OPTIONS */
int bdVersion(void);
	/* Returns version number = major*1000+minor*100+release*10+uses_asm(0|1)+uses_64(0|2) 
		 E.g. Version 2.3.0 will return 230x where x denotes the preprocessor options
		 x | USE_SPASM | USE_64WITH32 | NO_ALLOCS
		 ----------------------------------------
		 0      No            No           N/A
		 1      Yes           No           N/A
		 2      No            Yes          N/A
		 3      Yes           Yes*         N/A
		 ----------------------------------------
		 * USE_SPASM will take precedence over USE_64WITH32.
	 */


#undef T /* (for opaque BIGD pointer) */

#ifdef __cplusplus
}
#endif

#endif /* BIGD_H_ */
