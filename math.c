#include <stdint.h>

/*
   Copyright stuff

   Use of this program, for any purpose, is granted the author,
   Ian Kaplan, as long as this copyright notice is included in
   the source code or any source code derived from this program.
   The user assumes all responsibility for using this code.

   Ian Kaplan, October 1996

*/
uint32_t _remainder;

uint32_t
unsigned_divide(
	uint32_t		dividend,
	uint32_t		divisor
)
{
	if( divisor == 0 )
		return 0xffffffff;

	if( divisor > dividend )
		return 0;

	if( divisor == dividend )
		return 1;

	uint32_t num_bits = 32;
	uint32_t remainder = 0;
	uint32_t d;


	while( remainder < divisor )
	{
		uint32_t bit = (dividend & 0x80000000) >> 31;
		remainder = (remainder << 1) | bit;
		d = dividend;
		dividend = dividend << 1;
		num_bits--;
	}


	/* The loop, above, always goes one iteration too far.
	 * To avoid inserting an "if" statement inside the loop
	 * the last iteration is simply reversed. */

	dividend = d;
	remainder = remainder >> 1;
	num_bits++;

	uint32_t quotient = 0;

	unsigned i;

	for( i = 0; i < num_bits; i++ )
	{
		uint32_t bit = (dividend & 0x80000000) >> 31;
		remainder = (remainder << 1) | bit;
		uint32_t t = remainder - divisor;
		uint32_t q = !((t & 0x80000000) >> 31);
		dividend = dividend << 1;
		quotient = (quotient << 1) | q;
		if( q )
			remainder = t;
	}
  
	return quotient;
}

#if 0

#include <stdio.h>
#include <stdlib.h>

int main( int argc, char ** argv )
{
	uint32_t a = atoi( argv[1] );
	uint32_t b = atoi( argv[2] );
	uint32_t div = a/b;
	uint32_t div2 = unsigned_divide( a, b );

	printf( "%s %d/%d = %d %s %d\n",
		div == div2 ? "PASS" : "FAIL",
		a,
		b,
		div,
		div == div2 ? "==" : "!=",
		div2
	);

	return div != div2;
}
#endif
