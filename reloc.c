#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define BRANCH_MASK	0x0F000000
#define BRANCH_LINK	0x0B000000
#define BRANCH_INSTR	0x0A000000
#define BRANCH_OFFSET	0x00FFFFFF


/** Search through a memory region, looking for branch instructions */
void
fixup(
	uint32_t *		buf,
	uintptr_t		load_addr,
	uintptr_t		func_offset,
	size_t			func_len
)
{
	uintptr_t		i;
	uint8_t * const		mem = ((uint8_t*) buf) - load_addr;
	uintptr_t		func_end = func_offset + func_len;

	printf( "Fixing from %08x to %08x\n", func_offset, func_end );

	for( i=func_offset ; i<func_end ; i += 4 )
	{
		uint32_t instr = *(uint32_t*)( mem+i );
		uint32_t branch = instr & BRANCH_MASK;
		uint32_t offset = instr & BRANCH_OFFSET;

		// Sign extend the offset
		if( offset & 0x00800000 )
			offset |= 0xFF000000;
		uintptr_t dest = i + (offset << 2) + 8;

		// Check for branch
		if( branch != BRANCH_LINK
		&&  branch != BRANCH_INSTR
		)
			continue;

		// Ignore branches inside the reloc space
		int internal = (func_offset <= dest) && (dest < func_end);

		printf( "%08x: %08x B%s %08x => %08x%s\n",
			i,
			instr,
			branch == BRANCH_LINK ? "L" : " ",
			offset,
			dest,
			internal ? " (internal)" : ""
		);
	}
}


int
main(
	int			argc,
	char **			argv
)
{
	if( argc <= 1 )
		return -1;

	const char *		filename = argv[1];
	const uintptr_t		load_addr	= 0xFF800000;
	const size_t		offset		= 0xFF810894;
	const size_t		func_len	= 0x10000;

	int fd = open( filename, O_RDONLY );
	if( fd < 0 )
		goto abort;

	struct stat stat;
	if( fstat( fd, &stat ) < 0 )
		goto abort;

	const size_t len = stat.st_size;

	printf( "%s: %ld bytes\n", filename, len );
	void * buf = malloc( len );
	if( !buf )
		goto abort;

	if( read( fd, buf, len ) != len )
		goto abort;

	fixup( buf, load_addr, offset, func_len );

	return 0;
abort:
	perror( filename );
	return -1;
}
