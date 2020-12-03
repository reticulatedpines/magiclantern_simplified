/** \file
 * Fixup ARM assembly to forcibly relocate it from ROM into RAM.
 *
 * This will walk through a copy of a ARM executable and find all of
 * the %pc relative addressing modes. These will be fixed-up to allow
 * the function to be run from a new location.
 */
#ifdef CONFIG_MAGICLANTERN

#ifdef CONFIG_QEMU
    /* redirect all printf's to QEMU console */
    #include <dryos.h>
    #define printf qprintf
#else
    /* by default, quiet all printf's */
    #define printf(...)
#endif

#else /* for host PC */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#endif

#include "reloc.h"
#include <patch.h>

#define REG_PC      15
#define LOAD_MASK   0x0C000000
#define LOAD_INSTR  0x04000000
#define BRANCH_MASK 0x0F000000
#define BRANCH_LINK 0x0B000000
#define BRANCH_INSTR    0x0A000000
#define BRANCH_OFFSET   0x00FFFFFF
#define ADD_MASK    0x0DE00000
#define ADD_INSTR   0x00800000


/**
 * Search through a memory region, looking for branch instructions
 * Returns a pointer to the new func_offset address.
 * Requires 64 bytes for fixups.
 */
uintptr_t
reloc(
    uint32_t *      buf,
    uintptr_t       load_addr,
    uintptr_t       func_offset,
    size_t          func_end,
    uintptr_t       new_pc
)
{
    uint8_t * const     mem = ((uint8_t*) buf) - load_addr;
    // const uintptr_t      func_len = func_end - func_offset;

    printf( "Fixing from %08x to %08x\n", func_offset, func_end );

    // Add up to 64 bytes of fixups
    uintptr_t fixups = new_pc;
    uintptr_t entry = new_pc += 64;

    for (uintptr_t pc = func_offset; pc < func_end; pc += 4, new_pc += 4)
    {
        uint32_t instr = *(uint32_t*)( mem+pc );
        uint32_t branch = instr & BRANCH_MASK;
        uint32_t load = instr & LOAD_MASK;

        // The default is to just copy the instruction
        *(uint32_t*) new_pc = instr;

        // Check for branch
        if( branch == BRANCH_LINK
        ||  branch == BRANCH_INSTR
        ) {
            uint32_t offset = instr & BRANCH_OFFSET;

            // Sign extend the offset
            if( offset & 0x00800000 )
                offset |= 0xFF000000;
            uintptr_t dest = pc + (offset << 2) + 8;

            // Ignore branches inside the reloc space
            if( func_offset <= dest && dest < func_end )
                continue;

            printf( "%08x: %08x B%s %08x => %08x\n",
                pc,
                instr,
                branch == BRANCH_LINK ? "L" : " ",
                offset,
                dest
            );

            // Can we make this work?
            int32_t new_jump = (dest - new_pc - 8);
            new_jump >>= 2;
            if( new_jump >= +0x00800000
            ||  new_jump <= -0x00800000
            ) {
                printf( "%08x: !!!! can not fixup jump from %08x to %08x (offset %s%08x)\n",
                    pc,
                    new_pc,
                    dest,
                    new_jump < 0 ? "-" : "+",
                    new_jump < 0 ? -new_jump : new_jump
                );
                continue;
            }

            uint32_t new_instr = 0
                | (instr & ~BRANCH_OFFSET)
                | (new_jump & BRANCH_OFFSET);

            printf( "%08x: %08x => %08x fixup offset %08x => %s%08x\n",
                pc,
                instr,
                new_instr,
                offset,
                new_jump < 0 ? "-" : "+",
                new_jump < 0 ? -new_jump : new_jump
            );

#ifdef __ARM__
            // Write the new instruction into memory
            *(uint32_t*) new_pc = new_instr;
#endif

            continue;
        }

        // Check for a add instr with %pc
        if( (instr & ADD_MASK) == ADD_INSTR )
        {
            uint32_t reg_base   = (instr >> 16) & 0xF;
            uint32_t reg_dest   = (instr >> 12) & 0xF;
            uint32_t shift      = (instr >>  8) & 0xF;
            uint32_t imm        = (instr >>  0) & 0xFF;

            // Only even shift values are supported
            shift <<= 1;

            if( reg_base != REG_PC )
                continue;

            // If this is a jump table, we assume it is safe
            // add pc, pc, r0 << 2
            if( reg_dest == REG_PC )
                continue;

            if( (instr & (1<<25)) == 0 )
            {
                // Not an immediate 12-bit value;
                // update the offset
                printf( "%08x: unknown mode?\n", pc );
                continue;
            }

            // Shift is actually a 32-bit rotate
            uint32_t offset = imm >> shift;
            if( shift > 8 )
                offset |= imm << (32 - shift);
            uint32_t dest = pc + 8 + offset;

            // Ignore offsets inside the reloc space
            if( func_offset <= dest && dest < func_end )
                continue;

            // Find the data that is being used and
            // compute a new offset so that it can be
            // accessed from the relocated space.
            // Replace with a LDR instruction:
            // ADR PC, something => LDR PC, =address_of_something
            uint32_t data = dest;
            int32_t new_offset = fixups - new_pc - 8;
            uint32_t sign_bit = (1 << 23);
            if ( new_offset < 0 )
            {
                // Set offset to negative
                sign_bit = 0;
                new_offset = -new_offset;
            }

            // Turn ADR into LDR; keep condition flags
            // and destination register from old instruction
            uint32_t new_instr = 0
                | ( instr &  0xF000F000 )
                | ( /*LDR*/  0x051F0000 )
                | ( sign_bit & 0x800000 )
                | ( new_offset &  0xFFF )
                ;


            printf( "%08x: %08x add pc shift=%x imm=%2x offset=%x => %08x\n",
                pc,
                instr,
                shift,
                imm,
                offset,
                dest
            );
#ifdef __ARM__
            // Copy the data to the offset location
            *(uint32_t*) fixups = data;
            *(uint32_t*) new_pc = new_instr;
#endif
            fixups += 4;

            continue;
        }

        // Check for load from %pc
        if( load == LOAD_INSTR )
        {
            uint32_t reg_base   = (instr >> 16) & 0xF;
            uint32_t reg_dest   = (instr >> 12) & 0xF;
            int32_t offset      = (instr >>  0) & 0xFFF;

            if( reg_base != REG_PC )
                continue;

            // Check direction bit and flip the sign
            if( (instr & (1<<23)) == 0 )
                offset = -offset;

            // Compute the destination, including the change in pc
            uint32_t dest       = pc + offset + 8;

            // Ignore ones that are within our reloc space
            if( func_offset <= dest && dest < func_end )
                continue;

            // Find the data that is being used and
            // compute a new offset so that it can be
            // accessed from the relocated space.
            uint32_t data = *(uint32_t*)( dest + mem );
            int32_t new_offset = fixups - new_pc - 8;
            uint32_t sign_bit = (1 << 23);
            if( new_offset < 0 )
            {
                // Set offset to negative
                sign_bit = 0;
                new_offset = -new_offset;
            }

            uint32_t new_instr = 0
                | ( instr &  0xFF7FF000 )
                | ( sign_bit & 0x800000 )
                | ( new_offset &  0xFFF )
                ;

            // This is one that will need to be copied
            // but we currently don't do anything!
            printf( "%08x: %08x LD %d, %d, %d => %08x: %08x %d data=%08x\n",
                pc,
                instr,
                reg_dest,
                reg_base,
                offset,
                dest,
                new_instr,
                new_offset,
                data
            );

#ifdef __ARM__
            // Copy the data to the offset location
            *(uint32_t*) fixups = data;
            *(uint32_t*) new_pc = new_instr;
#endif
            fixups += 4;

            continue;
        }
    }

    printf( "Fixups=%x entry=%x free_space=%x\n", fixups, entry, entry - fixups);

#ifdef __ARM__
    /* don't return if executable code was overwritten by fixups */
    while ((intptr_t)entry - (intptr_t)fixups < 0);
    
    /* before we execute code, make sure a) data caches are drained and b) instruction caches are clean */
    sync_caches();
#endif

    // Return the entry point of the new function
    return entry;
}


#ifndef __ARM__
int
main(
    int         argc,
    char **         argv
)
{
    if( argc <= 1 )
        return -1;

    const char *        filename = argv[1];
    const uintptr_t     load_addr   = 0xFF800000;

    // DlgLiveViewApp and other routines to test
    size_t      func_start  = 0xFFA96B1C; // DlgLiveViewApp
    //size_t        func_start  = 0xffa96390; // early data
    size_t      func_end    = 0xFFA97FF8;
    size_t      func_len    = func_end - func_start;

    if( argc > 2 )
        func_start = strtoul( argv[2], 0, 0 );
    if( argc > 3 )
        func_end = strtoul( argv[3], 0, 0 );

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

    reloc( buf, load_addr, func_start, func_end, 0x9000 );

    return 0;
abort:
    perror( filename );
    return -1;
}
#endif
