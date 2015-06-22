
#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <console.h>

#define START_ADDR 0xFF000000
#define END_ADDR 0xFFFF0000
#define STMFD 0xe92d0000

static uint32_t ror(uint32_t word, uint32_t count)
{
    return word >> count | word << (32 - count);
}

static uint32_t decode_immediate_shifter_operand(uint32_t insn)
{
    uint32_t inmed_8 = insn & 0xFF;
    uint32_t rotate_imm = (insn & 0xF00) >> 7;
    return ror(inmed_8, rotate_imm);
}

static uint32_t locate_func_start(uint32_t from_addr)
{
    /* search backwards in memory, until STMFD */
    for(uint32_t i = from_addr; i > START_ADDR; i -= 4 )
    {
        if( ((*(uint32_t*)i) & 0xFFFF0000) == (STMFD & 0xFFFF0000))
        {
            return i;
        }
    }
    return 0;
}

static uint32_t find_func_from_string(char* string, int Rd, uint32_t max_start_offset)
{
    for(uint32_t i = START_ADDR; i < END_ADDR; i += 4 )
    {
        uint32_t insn = *(uint32_t*)i;
        if ((insn & 0xFFFFF000) == (0xe28f0000 | (Rd << 12))) /* add Rd, pc, #offset */
        {
            /* let's check if it refers to our string */
            int offset = decode_immediate_shifter_operand(insn);
            int pc = i;
            int dest = pc + offset + 8;
            if (streq((char*)dest, string))
            {
                /* looks promising */
                uint32_t func_start = locate_func_start(i);
                if (func_start && (i - func_start < max_start_offset))
                {
                    /* bingo, start of function is not too far from the string reference */
                    return func_start;
                }
            }
        }
    }
    return 0;
}

static void (*MpuMonOpen)() = NULL;
static void (*MpuMonClose)() = NULL;
static void (*MpuMonRead)(int kind, uint32_t addr, int size, char *buf) = NULL;

static void mpu_dump(int kind, uint32_t addr, int size, char* filename)
{
    char buffer[1024];

    printf("Dumping MPU from %08X to %08X (%d)...\n    ", addr, addr+size-1, kind);
    FILE* f = FIO_CreateFile(filename);
    MpuMonOpen();
    for (int i = 0; i < size/1024; i++)
    {
        printf("\b\b\b\b\b%d%%", i * 100 / (size/1024-1));
        MpuMonRead(kind, addr + sizeof(buffer) * i, sizeof(buffer), buffer);
        FIO_WriteFile(f, (const void *) buffer, 1024);
    }
    MpuMonClose();
    FIO_CloseFile(f);
    printf("\b\b\b\b\b");
}

static void mpu_dump_task()
{
    console_show();
    msleep(1000);
    
    /* find the stubs in Canon code, from strings */
    MpuMonOpen  = (void*) find_func_from_string("MpuMonOpen Start", 2, 64);
    MpuMonClose = (void*) find_func_from_string("MpuMonClose Start", 2, 64);
    MpuMonRead  = (void*) find_func_from_string("MpuMonRead Start (%d, %#x, %#x, %#x)", 2, 64);
    printf("MpuMonOpen  %X\n", MpuMonOpen);
    printf("MpuMonClose %X\n", MpuMonClose);
    printf("MpuMonRead  %X\n", MpuMonRead);
    if (!MpuMonOpen || !MpuMonClose || !MpuMonRead)
    {
        printf("Stubs error.\n");
        return;
    }

    /**
     * These numbers are valid for TMP19A43 (60D, 550D, 500D, 600D, 5D2...)
     * See http://magiclantern.wikia.com/wiki/Tx19a#Memory_Map
     * There might be some more valid address ranges, if anyone
     * has the patience to do a brute force scan :)
     * 
     * Newer cameras (5D3, 650D) use a F74964A/F74965A, which looks like
     * a Fujitsu FR, with different memory map, and no datasheet :(
     */
    mpu_dump(0, 0x00000000, 512*1024, "ML/LOGS/MPU-ROM.BIN");       /* 550D: code executes from here */
//  mpu_dump(0, 0xBFC00000, 512*1024, "ML/LOGS/MPU-ROM2.BIN");      /* ROM mirror (same as previous) */
    mpu_dump(0, 0xFFFF8000,  24*1024, "ML/LOGS/MPU-RAM.BIN");       /* from datasheet */
//  mpu_dump(0, 0xFFFFE000,   8*1024, "ML/LOGS/MPU-IO.BIN");        /* 60D: crash */
    mpu_dump(1, 0x00000000,  56*1024, "ML/LOGS/MPU-UNK1.BIN");      /* 60D,500D: crash if trying to dump more */
    mpu_dump(2, 0x00000000,  64*1024, "ML/LOGS/MPU-EEP.BIN");       /* contents repeat after 0x8000 (60D) or 0x4000 (500D). 60D: contents match k287_eep.mot. */
    /* 60D: further increase of "kind" gives empty dumps */
    
    printf("Done.\n");
}

static struct menu_entry mpu_dump_menu[] =
{
    {
        .name = "Dump MPU memory",
        .select = run_in_separate_task,
        .priv = mpu_dump_task,
    }
};

static unsigned int mpu_dump_init()
{
    menu_add("Debug", mpu_dump_menu, COUNT(mpu_dump_menu));
    return 0;
}

static unsigned int mpu_dump_deinit()
{
    menu_remove("Debug", mpu_dump_menu, COUNT(mpu_dump_menu));
    return 0;
}


MODULE_INFO_START()
    MODULE_INIT(mpu_dump_init)
    MODULE_DEINIT(mpu_dump_deinit)
MODULE_INFO_END()
