/** \file
 * Generic file I/O test code in QEMU.
 */

#include "dryos.h"
#include "vram.h"
#include "lens.h"
#include "timer.h"

/** These are called when new tasks are created */
static int my_init_task(int a, int b, int c, int d);

/** This just goes into the bss */
#define RELOCSIZE 0x10000 // look in HIJACK macros for the highest address, and subtract ROMBASEADDR
static uint8_t _reloc[ RELOCSIZE ];
#define RELOCADDR ((uintptr_t) _reloc)

/** Translate a firmware address into a relocated address */
#define INSTR( addr ) ( *(uint32_t*)( (addr) - ROMBASEADDR + RELOCADDR ) )

/** Fix a branch instruction in the relocated firmware image */
#define FIXUP_BRANCH( rom_addr, dest_addr ) \
    INSTR( rom_addr ) = BL_INSTR( &INSTR( rom_addr ), (dest_addr) )

/** Specified by the linker */
extern uint32_t _bss_start[], _bss_end[];

static inline void
zero_bss( void )
{
    uint32_t *bss = _bss_start;
    while( bss < _bss_end )
        *(bss++) = 0;
}

void
__attribute__((noreturn,noinline,naked))
copy_and_restart( int offset )
{
    zero_bss();

    // Copy the firmware to somewhere safe in memory
    const uint8_t * const firmware_start = (void*) ROMBASEADDR;
    const uint32_t firmware_len = RELOCSIZE;
    uint32_t * const new_image = (void*) RELOCADDR;

    blob_memcpy( new_image, firmware_start, firmware_start + firmware_len );

    /*
     * in entry2() (0xff010134) make this change to
     * return to our code before calling cstart().
     * This should be a "BL cstart" instruction.
     */
    INSTR( HIJACK_INSTR_BL_CSTART ) = RET_INSTR;

    /*
     * in cstart() (0xff010ff4) make these changes:
     * calls bzero(), then loads bs_end and calls
     * create_init_task
     */
    // Reserve memory after the BSS for our application
    INSTR( HIJACK_INSTR_BSS_END ) = (uintptr_t) _bss_end;

    // Fix the calls to bzero32() and create_init_task()
    FIXUP_BRANCH( HIJACK_FIXBR_BZERO32, bzero32 );
    FIXUP_BRANCH( HIJACK_FIXBR_CREATE_ITASK, create_init_task );

    // Set our init task to run instead of the firmware one
    INSTR( HIJACK_INSTR_MY_ITASK ) = (uint32_t) my_init_task;
    
    // Make sure that our self-modifying code clears the cache
    sync_caches();

    // We enter after the signature, avoiding the
    // relocation jump that is at the head of the data
    thunk reloc_entry = (thunk)( RELOCADDR + 0xC );
    reloc_entry();

    /*
    * We're back!
    * The RAM copy of the firmware startup has:
    * 1. Poked the DMA engine with what ever it does
    * 2. Copied the rw_data segment to 0x1900 through 0x20740
    * 3. Zeroed the BSS from 0x20740 through 0x47550
    * 4. Copied the interrupt handlers to 0x0
    * 5. Copied irq 4 to 0x480.
    * 6. Installed the stack pointers for CPSR mode D2 and D3
    * (we are still in D3, with a %sp of 0x1000)
    * 7. Returned to us.
    *
    * Now is our chance to fix any data segment things, or
    * install our own handlers.
    */

    // This will jump into the RAM version of the firmware,
    // but the last branch instruction at the end of this
    // has been modified to jump into the ROM version
    // instead.
    void (*ram_cstart)(void) = (void*) &INSTR( cstart );
    ram_cstart();

    // Unreachable
    while(1)
        ;
}

char* get_current_task_name()
{
    /* DryOS: right after current_task we have a flag
     * set to 1 when handling an interrupt */
    uint32_t interrupt_active = MEM((uintptr_t)&current_task + 4);
    
    if (!interrupt_active)
    {
        return current_task->name;
    }
    else
    {
        static char isr[] = "**INT-00h**";
        int i = current_interrupt >> 2;
        int i0 = (i & 0xF);
        int i1 = (i >> 4) & 0xF;
        int i2 = (i >> 8) & 0xF;
        isr[5] = i2 ? '0' + i2 : '-';
        isr[6] = i1 < 10 ? '0' + i1 : 'A' + i1 - 10;
        isr[7] = i0 < 10 ? '0' + i0 : 'A' + i0 - 10;
        return isr;
    }
}

/* adapted from file_man.c */
static const char * format_date( unsigned timestamp )
{
    static char str[32];
    char datestr[11];
    int year=1970;                   // Unix Epoc begins 1970-01-01
    int month=11;                    // This will be the returned MONTH NUMBER.
    int day;                         // This will be the returned day number. 
    int dayInSeconds=86400;          // 60secs*60mins*24hours
    int daysInYear=365;              // Non Leap Year
    int daysInLYear=daysInYear+1;    // Leap year
    int days=timestamp/dayInSeconds; // Days passed since UNIX Epoc
    int tmpDays=days+1;              // If passed (timestamp < dayInSeconds), it will return 0, so add 1

    while(tmpDays>=daysInYear)       // Start adding years to 1970
    {      
        year++;
        if ((year)%4==0&&((year)%100!=0||(year)%400==0)) tmpDays-=daysInLYear; else tmpDays-=daysInYear;
    }

    int monthsInDays[12] = {-1,30,59,90,120,151,181,212,243,273,304,334};
    if (!(year)%4==0&&((year)%100!=0||(year)%400==0))  // The year is not a leap year
    {
        monthsInDays[0] = 0;
        monthsInDays[1] =31;
    }

    while (month>0)
    {
        if (tmpDays>monthsInDays[month]) break;       // month+1 is now the month number.
        month--;
    }
    day=tmpDays-monthsInDays[month];                  // Setup the date
    month++;                                          // Increment by one to give the accurate month
    if (day==0) {year--; month=12; day=31;}			  // Ugly hack but it works, eg. 1971.01.00 -> 1970.12.31

    snprintf( datestr, sizeof(datestr), "%02d/%02d/%d ", day, month, year);

    int minute = (timestamp / 60) % 60;
    int hour = (timestamp / 60 / 60) % 24;

    snprintf( str, sizeof(str), "%s %02d:%02d", datestr, hour, minute);

    return str;
}

struct fio_dirent * _FIO_FindFirstEx(const char * dirname, struct fio_file * file);

#ifndef GET_DIGIC_TIMER /* FIXME */
#define GET_DIGIC_TIMER() *(volatile uint32_t*)0xC0242014   /* 20-bit microsecond timer */
#endif

static void test_findfirst()
{
    struct fio_file file;
    struct fio_dirent * dirent;
    int card_type = -1; /* 0 = CF, 1 = SD */

    /* file I/O backend may or may not be started; retry a few times if needed */
    for (int i = 0; i < 10; i++)
    {
        qprintf("Trying SD card...\n");
        dirent = _FIO_FindFirstEx( "B:/", &file );
        if (!IS_ERROR(dirent)) {
            card_type = 1;
            break;
        } else {
            qprintf("FIO_FindFirstEx error %x.\n", dirent);
        }

        qprintf("Trying CF card...\n");
        dirent = _FIO_FindFirstEx( "A:/", &file );
        if (!IS_ERROR(dirent)) {
            card_type = 0;
            break;
        } else {
            qprintf("FIO_FindFirstEx error %x.\n", dirent);
        }
        msleep(500);
    }

    if (card_type < 0)
    {
        qprintf("FIO_FindFirst test failed.\n");
        return;
    }

    qprintf("    filename     size     mode     timestamp\n");
    do {
        /* FIXME: %12s not working */
        char file_name[13];
        snprintf(file_name, sizeof(file_name), "%s            ", file.name);
        qprintf("--> %s %08x %08x %s\n", file_name, file.size, file.mode, format_date(file.timestamp));
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_FindClose(dirent);

    /* test for FindClose */
    /* keep run for 2 seconds, and report how many iterations it performed */
    /* card emulation speed in QEMU varies a lot across different models,
     * sometimes by as much as 2 orders of magnitude */
    int i = 0;
    uint32_t elapsed_time = 0;
    uint32_t last = GET_DIGIC_TIMER();
    while (elapsed_time < 2000000)
    {
        uint32_t now = GET_DIGIC_TIMER();
        elapsed_time += (now << 12) - (last << 12) >> 12;
        last = now;
        i++;

        dirent = _FIO_FindFirstEx(card_type == 1 ? "B:/" : "A:/", &file);
        if (IS_ERROR(dirent)) {
            qprintf("FIO_FindFirstEx error %x at iteration %d.\n", dirent, i);
            return;
        }
        /* iterate through some of the files, but not necessarily all of them */
        for (int j = 0; j < i % 16; j++) {
            if (FIO_FindNextEx(dirent, &file))
                break;
        }
        /* commenting out FindClose will fail the test (too many open handles) */
        FIO_FindClose(dirent);
    }
    qprintf("FIO_FindClose: completed %d iterations.\n", i);
}

/** Initial task setup.
 *
 * This is called instead of the task at 0xFF811DBC.
 * It does all of the stuff to bring up the debug manager,
 * the terminal drivers, stdio, stdlib and armlib.
 */
static int
my_init_task(int a, int b, int c, int d)
{

    init_task(a,b,c,d);

    msleep(2000);
    task_create("run_test", 0x1e, 0x4000, test_findfirst, 0 );

    return 0;
}

/* dummy stubs */

void disp_set_pixel(int x, int y, int c) { }

void ml_assert_handler(char* msg, char* file, int line, const char* func) { };
