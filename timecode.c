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

	// Now we are at the data
	int bit = 0;
	int one_count = 0;
	uint32_t word = 0;
	uint32_t raw_word = 0;
	int synced = 0;
	int byte_count = 0;

	// Reconstruct the clock?
	while( offset < hdr.len )
	{
		ssize_t rc = read( fd, &sample, sizeof(sample) );
		offset += rc;
		if( rc < 1 )
			break;

		sample = ntohs( sample );

		//printf( "%d\n", sample );
		//continue;

		// Use some hysteris to avoid zero crosing errors
		if( sample > 40000 )
			bit = 0;
		else
		if( sample < 24000 )
			bit = 1;

		//printf( "%d %d %d\n", offset, sample, bit );
		//continue;

		raw_word = (raw_word << 1) | bit;

		// Wait until we have at least 16 bits worth of samples
		if( (raw_word & 0xFFFF0000) == 0 )
			continue;

		// Check for a solid lock
		uint32_t test_word = raw_word & 0xFFFF;
		if( test_word == 0xFF00
		||  test_word == 0x00FF )
		{
			//printf( "%d 1\n", (int) offset );
			word = (word << 1) | 1;
			raw_word = 0x1;
		} else
		if( test_word == 0xFFFF
		||  test_word == 0x0000 )
		{
			//printf( "%d 0\n", (int) offset );
			word = (word << 1) | 0;
			raw_word = 0x1;
		} else
			continue;

		//printf( "%04x\n", word & 0xFFFF );
		//continue;

		// Check for SMPTE sync signal
		if( !synced )
		{
			if( (word & 0xFFFF) == 0x3FFD )
			{
				fprintf( stderr, "%x: Got sync!\n", offset );
				printf( "\n%08x", offset );
				synced = 1;
				word = 1;
				byte_count = 0;
			}
			continue;
		}

		// We are locked; wait until we have 8 bits then write it
		if( (word & 0x100) == 0 )
			continue;

		printf( " %02x", word & 0xFF );
		word = 1;
		if( ++byte_count == 8 )
			synced = 0;
	}
	return 0;
}
