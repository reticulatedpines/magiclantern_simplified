#include "stdio.h"
#include "stdint.h"

static inline uint32_t thumb_branch_instr(uint32_t pc, uint32_t dest, uint32_t opcode)
{
    /* thanks atonal */
    uint32_t offset = dest - ((pc + 4) & ~3); /* according to datasheets, this should be the correct calculation -> ALIGN(PC, 4) */
    uint32_t s = (offset >> 24) & 1;
    uint32_t i1 = (offset >> 23) & 1;
    uint32_t i2 = (offset >> 22) & 1;
    uint32_t imm10 = (offset >> 12) & 0x3ff;
    uint32_t imm11 = (offset >> 1) & 0x7ff;
    uint32_t j1 = (!(i1 ^ s)) & 0x1;
    uint32_t j2 = (!(i2 ^ s)) & 0x1;

    return opcode | (s << 10) | imm10 | (j1 << 29) | (j2 << 27) | (imm11 << 16);
}

#define THUMB_B_W_INSTR(pc,dest)      thumb_branch_instr(pc,dest,0x9000f000)
#define THUMB_BLX_W_INSTR(pc,dest)    thumb_branch_instr(pc,dest,0xc000f000)

int main()
{
    /* compare this to gcc output from test.S */
    printf("  0x8000\tB.W 0x1234:\t%x\n", THUMB_B_W_INSTR(0x8000,0x1234));
    printf("  0x8004\tB.W 0x1234:\t%x\n", THUMB_BLX_W_INSTR(0x8004,0x1234));
    return 0;
}
