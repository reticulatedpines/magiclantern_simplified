#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define REG_PC		15
#define LOAD_MASK	0x0C000000
#define LOAD_INSTR	0x04000000
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
		uint32_t load = instr & LOAD_MASK;

		// Check for branch
		if( branch == BRANCH_LINK
		||  branch == BRANCH_INSTR
		) {
			uint32_t offset = instr & BRANCH_OFFSET;

			// Sign extend the offset
			if( offset & 0x00800000 )
				offset |= 0xFF000000;
			uintptr_t dest = i + (offset << 2) + 8;

			// Ignore branches inside the reloc space
			if( func_offset <= dest && dest < func_end )
				continue;

			printf( "%08x: %08x B%s %08x => %08x\n",
				i,
				instr,
				branch == BRANCH_LINK ? "L" : " ",
				offset,
				dest
			);

			continue;
		}

		// Check for load from %pc
		if( load == LOAD_INSTR )
		{
			uint32_t reg_base	= (instr >> 16) & 0xF;
			uint32_t reg_dest	= (instr >> 12) & 0xF;
			int32_t offset		= (instr >>  0) & 0xFFF;

			if( reg_base != REG_PC )
				continue;

			// Sign extend offset if the up bit is not set
			if( (instr & (1<<23)) == 0 )
				offset = -offset;

			uint32_t dest		= i + offset + 8;

			// Ignore ones that are within our reloc space
			if( func_offset <= dest && dest < func_end )
				continue;

			printf( "%08x: %08x LD %d, %d, %d => %08x\n",
				i,
				instr,
				reg_dest,
				reg_base,
				offset,
				dest
			);
			continue;
		}
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

	// DlgLiveViewApp to test
	const size_t		func_start	= 0xFFA96B1C;
	const size_t		func_end	= 0xFFA97FF8;
	const size_t		func_len	= func_end - func_start; 

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

	fixup( buf, load_addr, func_start, func_len );

	return 0;
abort:
	perror( filename );
	return -1;
}
