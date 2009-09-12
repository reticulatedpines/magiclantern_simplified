#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h> // for ntohl

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
	||  hdr.encoding != 2 )
	{
		fprintf( stderr, "Bad magic or unsupported format!\n" );
		return -1;
	}

	int8_t sample;
	uint32_t last_transition = 0;

	// Seek to the start of the data
	while( offset < hdr.offset )
		offset += read( fd, &sample, 1 );

	// Now we are at the data
	int bit = 0;
	int one_count = 0;
	int word = 0;
	int synced = 0;
	int bit_count = 0;

	// Reconstruct the clock?
	while( offset < hdr.len )
	{
		ssize_t rc = read( fd, &sample, 1 );
		offset += rc;
		if( rc < 1 )
			break;

		int old_bit = bit;

		if( sample > 50 )
			bit = 0;
		else
		if( sample < -50 )
			bit = 1;

		if( old_bit == bit )
			continue;

		// Transistion!
		uint32_t delta = offset - last_transition;
		last_transition = offset;
		int new_bit = delta < 8 ? 1 : 0;

		//printf( "%d\n", new_bit );
		//continue;

		// Look for a sync pulse of 12 ones in a row
		if( !synced )
		{
			if( new_bit == 0 )
			{
				one_count = 0;
				continue;
			}

			if( ++one_count < 11 )
				continue;

			// Got it!
			synced = 1;
			word = 1; // Preset a bit
			bit_count = 0;
			printf( "\n" );
			fprintf( stderr, "%08x: synced!\n", offset );
			continue;
		}

		// Store it
		word = (word << 1) | new_bit;

		if( word & 0x100 )
		{
			printf( "%02x ", word & 0xFF );
			word = 1;
		}

		if( ++bit_count > 64 )
			synced = 0;
	}
	return 0;
}
