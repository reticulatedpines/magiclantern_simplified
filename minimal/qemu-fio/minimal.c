/** \file
 * Generic file I/O test code in QEMU.
 */

#include "dryos.h"
#include "vram.h"
#include "lens.h"
#include "timer.h"

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

/* called before Canon's init_task */
void boot_pre_init_task(void)
{
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
    msleep(2000);
    task_create("run_test", 0x1e, 0x4000, test_findfirst, 0 );
}

/* dummy stubs */

void disp_set_pixel(int x, int y, int c) { }

void ml_assert_handler(char* msg, char* file, int line, const char* func) { };
