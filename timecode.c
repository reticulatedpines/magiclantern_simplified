/** \file SMPTE timecode analyzer for the audio port
 */
#ifndef __ARM__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h> // for ntohl
#else
#include "dryos.h"
#include "tasks.h"
#include "audio.h"
#include "bmp.h"
#endif
#include <stdint.h>

// SMTPE timecode frame
static uint8_t smpte_frame[ 8 ];


// Returns 1 if there is a new frame available
static int
tc_sample(
	int16_t			sample
)
{
	static uint32_t last_transition;

	// Hold onto the last bit for hysteris
	static int bit = 0;
	static int bit_count = 0;

	static uint32_t word = 0;
	static uint32_t raw_word = 0;
	static int synced = 0;
	static int byte_count = 0;

	// Use some hysteris to avoid zero crosing errors
	int old_bit = bit;
#ifdef __ARM__
	if( sample > 1000 )
		bit = 0;
	else
	if( sample < -1000 )
		bit = 1;
#else
	if( sample > 40000 )
		bit = 0;
	else
	if( sample < 24000 )
		bit = 1;
#endif

	static int x, y = 32;

	// Record any transition to help reconstruct the clock
	if( bit == old_bit )
		return 0;

	unsigned (*read_clock)(void) = (void*) 0xff9948d8;
	unsigned now = read_clock();
	int delta = now - last_transition;
	last_transition = now;

	// Timer is only 24 bits?
	delta &= 0x00FFFFFF;

#if 0
	con_printf( FONT_SMALL, "%08x ", delta );
	static int delta_count;
	if( ((++delta_count) % 6 ) == 0 )
		con_printf( FONT_SMALL, "\n" );

	return 0;
#endif

#if 0 //def __ARM__
	bmp_printf( FONT_SMALL, x, y, "%02x ", delta );
	x += 3 * 8;
	if( x > 700 )
	{
		x = 0;
		y += 12;
		if( y > 400 )
		{
			y = 32;
			msleep( 2000 );
		}
	}
	bit_count = 0;
	return 0;
#endif

	int new_bit;
	int valid = 0;

	static unsigned last_bit;

	if( 0xA0 < delta && delta < 0x120 )
	{
		// Skip the second transition if we're short
		if( ++last_bit & 1 )
			return 0;
		valid = 1;
		new_bit = 0;
	} else
	if( 0x150 < delta && delta < 0x350 )
	{
		// Full bit width
		last_bit = 0;
		new_bit = 1;
		valid = 1;
	} else {
		con_printf( FONT(FONT_SMALL,COLOR_RED,0), " %04x\n", delta );
		// Lost sync somewhere
		synced = 0;
		word = 0;
		return 0;
	}

	con_printf( FONT(FONT_SMALL,synced?COLOR_WHITE:COLOR_BLUE,0), "%1x", new_bit );
#if 0
	static int delta_count;
	if( ((++delta_count) % 8 ) == 0 )
		con_printf( FONT_SMALL, "\n" );
#endif

	word = (word << 1) | new_bit;

	//printf( "%04x\n", word & 0xFFFF );
	//return 0;

	// Check for SMPTE sync signal
	if( !synced )
	{
		if( (word & 0x1FFFF) == 0x13FFD )
		{
			//fprintf( stderr, "%x: Got sync!\n", offset );
			//printf( "\n%08x", offset );
			con_printf( FONT_SMALL, "\n== " );
			synced = 1;
			word = 1;
			byte_count = 0;
		}

		return 0;
	}

	// We are locked; wait until we have 8 bits then write it
	if( (word & 0x100) == 0 )
		return 0;

	//printf( " %02x", word & 0xFF );

	// Reverse the word since we bit banged it in the wrong order
	word = 0
		| ( word & 0x01 ) << 7
		| ( word & 0x02 ) << 5
		| ( word & 0x04 ) << 3
		| ( word & 0x08 ) << 1
		| ( word & 0x10 ) >> 1
		| ( word & 0x20 ) >> 3
		| ( word & 0x40 ) >> 5
		| ( word & 0x80 ) >> 7
		;
#ifdef __ARM__
	//con_printf( FONT_SMALL, "%02x ", word );
#endif

	smpte_frame[ byte_count++ ] = word;
	word = 1;
	if( byte_count < 8 )
		return 0;

	// We have a complete frame
	synced = 0;
	return 1;
}


#ifndef __ARM__
struct au_hdr
{
	uint32_t	magic;
	uint32_t	offset;
	uint32_t	len;
	uint32_t	encoding;
	uint32_t	rate;
	uint32_t	channels;
};


int main( int argc, char ** argv )
{
	const char * filename = argc > 1 ? argv[1] : "timecode.au";
	int fd = open( filename, O_RDONLY );
	if( fd < 0 )
	{
		perror( filename );
		return -1;
	}

	struct au_hdr hdr;
	size_t offset = read( fd, &hdr, sizeof(hdr) );

	hdr.magic	= ntohl( hdr.magic );
	hdr.offset	= ntohl( hdr.offset );
	hdr.len		= ntohl( hdr.len );
	hdr.encoding	= ntohl( hdr.encoding );
	hdr.rate	= ntohl( hdr.rate );
	hdr.channels	= ntohl( hdr.channels );
	fprintf( stderr,
		"magic=%08x encoding=%x\n",
		hdr.magic,
		hdr.encoding
	);

	if( hdr.magic != 0x2e736e64
	||  hdr.encoding != 3 )
	{
		fprintf( stderr, "Bad magic or unsupported format!\n" );
		return -1;
	}

	uint16_t sample;
	uint32_t last_transition = 0;

	// Seek to the start of the data
	while( offset < hdr.offset )
		offset += read( fd, &sample, 1 );

	while( offset < hdr.len )
	{
		ssize_t rc = read( fd, &sample, sizeof(sample) );
		offset += rc;
		if( rc < 1 )
			break;

		sample = ntohs( sample );
		if( tc_sample( sample ) == 0 )
			continue;

		printf( "%08x", offset );
		int i;
		for( i=0 ; i<8 ; i++ )
			printf( " %02x", smpte_frame[ i ] );

		// Decode it:
#define BCD_BITS(x) \
	(smpte_frame[x/8] & 0xF)

		int f = BCD_BITS(0) + 10 * (BCD_BITS(8) & 0x3);
		int s = BCD_BITS(16) + 10 * (BCD_BITS(24) & 0x7);
		int m = BCD_BITS(32) + 10 * BCD_BITS(40);
		int h = BCD_BITS(48) + 10 * (BCD_BITS(56) & 0x3);
		
		printf( ": %02d:%02d:%02d.%02d\n", h, m, s, f  );
	}

	return 0;
}
#else

static void
tc_task( void )
{
	msleep( 1000 );

	while(1)
	{
		int sample = audio_read_level( 0 );
		tc_sample( sample );

		// Busy wait for a few ticks
		volatile int i;
		for( i=0 ; i<50 ; i++ )
			asm( "nop" );
	}
}

TASK_CREATE( __FILE__, tc_task, 0, 0x1f, 0x1000 );
#endif


