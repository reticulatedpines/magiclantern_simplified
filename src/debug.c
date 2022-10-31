/** \file
 * Magic Lantern debugging and reverse engineering code
 */
#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"
#include "version.h"
#include "edmac.h"
#include "asm.h"
#include "beep.h"
#include "screenshot.h"
#include "console.h"
#include "zebra.h"
#include "shoot.h"
#include "cropmarks.h"
#include "fw-signature.h"
#include "lvinfo.h"
#include "raw.h"

#ifdef CONFIG_DEBUG_INTERCEPT
#include "dm-spy.h"
#include "tp-spy.h"
#endif

#ifdef CONFIG_MODULES
#include "module.h"
#endif
//#include "lua.h"

#if defined(CONFIG_600D) && defined(CONFIG_AUDIO_600D_DEBUG)
void audio_reg_dump_once();
#endif

#if defined(CONFIG_EDMAC_MEMCPY)
#include "edmac-memcpy.h"
#endif

extern int config_autosave;
extern void config_autosave_toggle(void* unused, int delta);

static struct semaphore * beep_sem = 0;

static void debug_init_func()
{
    beep_sem = create_named_semaphore("beep_sem",1);
}
INIT_FUNC("debug", debug_init_func);

void NormalDisplay();
void MirrorDisplay();
static void HijackFormatDialogBox_main();
void debug_menu_init();
void display_on();
void display_off();


void fake_halfshutter_step();

#ifdef CONFIG_DEBUG_INTERCEPT
void j_debug_intercept() { debug_intercept(); }
void j_tp_intercept() { tp_intercept(); }
#endif

#if CONFIG_DEBUGMSG
static int draw_prop = 0;

static int dbg_propn = 0;
static void
draw_prop_reset( void * priv )
{
    dbg_propn = 0;
}
#endif

void _card_led_on()
{
    *(volatile uint32_t*) (CARD_LED_ADDRESS) = (LEDON);
}

void _card_led_off()
{
    *(volatile uint32_t*) (CARD_LED_ADDRESS) = (LEDOFF);
}

void info_led_on()
{
    #ifdef CONFIG_VXWORKS
    LEDBLUE = LEDON;
    #elif defined(CONFIG_BLUE_LED)
    call("EdLedOn");
    #else
    _card_led_on();
    #endif
}
void info_led_off()
{
    #ifdef CONFIG_VXWORKS
    LEDBLUE = LEDOFF;
    #elif defined(CONFIG_BLUE_LED)
    call("EdLedOff");
    #else
    _card_led_off();
    #endif
}
void info_led_blink(int times, int delay_on, int delay_off)
{
    for (int i = 0; i < times; i++)
    {
        info_led_on();
        msleep(delay_on);
        info_led_off();
        msleep(delay_off);
    }
}

static void dump_rom_task(void* priv, int unused)
{
    msleep(200);
    FILE * f = NULL;

// Digic 6 doesn't have ROM0
#if defined(CONFIG_DIGIC_45) || defined(CONFIG_DIGIC_78)
    f = FIO_CreateFile("ML/LOGS/ROM0.BIN");
    if (f)
    {
        bmp_printf(FONT_LARGE, 0, 60, "Writing ROM0");
    #if defined(CONFIG_DIGIC_45)
        FIO_WriteFile(f, (void*) 0xF0000000, 0x01000000);
    #elif defined(CONFIG_DIGIC_78)
        FIO_WriteFile(f, (void*) 0xE0000000, 0x04000000); // max seen so far
    #endif
        FIO_CloseFile(f);
    }
    msleep(200);
#endif

    f = FIO_CreateFile("ML/LOGS/ROM1.BIN");
    if (f)
    {
        bmp_printf(FONT_LARGE, 0, 60, "Writing ROM1");
    #if defined(CONFIG_DIGIC_45)
        FIO_WriteFile(f, (void*) 0xF8000000, 0x01000000);
    #elif defined(CONFIG_DIGIC_6)
        FIO_WriteFile(f, (void*) 0xFE000000, 0x02000000);
    #elif defined(CONFIG_DIGIC_78)
        FIO_WriteFile(f, (void*) 0xF0000000, 0x02000000); // max seen so far
    #endif
        FIO_CloseFile(f);
    }
    msleep(200);

    dump_big_seg(4, "ML/LOGS/RAM4.BIN");
}

static void dump_img_task(void* priv, int unused)
{
    for (int i = 5; i > 0; i--)
    {
        NotifyBox(1000, "Will dump VRAMs in %d s...", i);
        msleep(1000);
    }
    NotifyBox(5000, "Dumping VRAMs...");
    
    FILE * f = NULL;
    char pattern[0x80];
    char filename[0x80];
    
    char* video_mode = get_video_mode_name(0);
    char* display_device = get_display_device_name();
    

    int path_len = snprintf(pattern, sizeof(pattern), "%s/%s/%s/", CAMERA_MODEL, video_mode, display_device);
    
    /* make sure the VRAM parameters are updated */
    get_yuv422_vram();
    get_yuv422_hd_vram();

    snprintf(pattern + path_len, sizeof(pattern) - path_len, "LV-%%03d.422", 0);
    get_numbered_file_name(pattern, 999, filename, sizeof(filename));
    f = FIO_CreateFile(filename);
    if (f)
    {
        FIO_WriteFile(f, vram_lv.vram, vram_lv.height * vram_lv.pitch);
        FIO_CloseFile(f);
    }

    snprintf(pattern + path_len, sizeof(pattern) - path_len, "HD-%%03d.422", 0);
    get_numbered_file_name(pattern, 999, filename, sizeof(filename));
    f = FIO_CreateFile(filename);
    if (f)
    {
        FIO_WriteFile(f, vram_hd.vram, vram_hd.height * vram_hd.pitch);
        FIO_CloseFile(f);
    }

#ifdef CONFIG_RAW_LIVEVIEW
    snprintf(pattern + path_len, sizeof(pattern) - path_len, "RAW-%%03d.DNG", 0);
    get_numbered_file_name(pattern, 999, filename, sizeof(filename));
    
    if (lv) raw_lv_request();
    if (raw_update_params())
    {
        /* first frames right after enabling the raw buffer might be corrupted, figure out why */
        /* todo: fix it in the raw backend */
        wait_lv_frames(3);
        raw_set_dirty();
        raw_update_params();
        
        /* make a copy of the raw buffer, because it's being updated while we are saving it */
        void* buf = malloc(raw_info.frame_size);
        if (buf)
        {
            memcpy(buf, raw_info.buffer, raw_info.frame_size);
            struct raw_info local_raw_info = raw_info;
            local_raw_info.buffer = buf;
            save_dng(filename, &local_raw_info);
            free(buf);
        }
    }
    if (lv) raw_lv_release();
    
    if (!is_file(filename))
    {
        /* if we don't have any raw data, create an empty DNG just to keep file numbering consistent */
        f = FIO_CreateFile(filename);
        FIO_CloseFile(f);
    }
#endif

    /* create a log file with relevant settings */
    snprintf(pattern + path_len, sizeof(pattern) - path_len, "VRAM-%%03d.LOG", 0);
    get_numbered_file_name(pattern, 999, filename, sizeof(filename));
    f = FIO_CreateFile(filename);
    if (f)
    {
        my_fprintf(f, "display=%d (hdmi=%d code=%d rca=%d)\n", EXT_MONITOR_CONNECTED, ext_monitor_hdmi, hdmi_code, _ext_monitor_rca);
        my_fprintf(f, "lv=%d (zoom=%d dispmode=%d rec=%d)\n", lv, lv_dispsize, lv_disp_mode, RECORDING_H264);
        my_fprintf(f, "movie=%d (res=%d crop=%d fps=%d)\n", is_movie_mode(), video_mode_resolution, video_mode_crop, video_mode_fps);
        my_fprintf(f, "play=%d (ph=%d, mv=%d, qr=%d)\n", PLAY_MODE, is_pure_play_photo_mode(), is_pure_play_movie_mode(), QR_MODE);
        
        FIO_CloseFile(f);
    }

    NotifyBox(2000, "Done :)");
    beep();
}

#ifdef FEATURE_GUIMODE_TEST
// beware, might be dangerous, some gui modes will give errors
void guimode_test()
{
    msleep(1000);
    for (int i = 0; i < 99; i++)
    {
        // some GUI modes may lock-up the camera or reboot
        // if this is the case, the troublesome mode will be skipped at next reboot.
        char fn[50];
        snprintf(fn, sizeof(fn), "VRAM%d.BMP", i);

        if (FIO_GetFileSize_direct(fn) != 0xFFFFFFFF) // this gui mode was already tested?
            continue;

        NotifyBox(500, "Trying GUI mode %d...", i);
        dump_seg(0, 0, fn); // temporary flag to indicate that this GUI mode was tried (and probably found to be troublesome)
        msleep(200);

        SetGUIRequestMode(i);

        msleep(1000);
        FIO_RemoveFile(fn);

        take_screenshot(SCREENSHOT_FILENAME_AUTO, SCREENSHOT_BMP);

        // try to reset to initial gui mode
        SetGUIRequestMode(0);
        SetGUIRequestMode(1);
        SetGUIRequestMode(0);

        msleep(1000);
    }
}
#endif

#ifdef CONFIG_200D
extern void GetMemoryInformation(uint32_t *, uint32_t *);
extern char* WinSys_AllocateMemory(uint32_t size);
extern void WinSys_FreeMemory(void *);
extern void maybe_read_mpu_logs(void);
static uint32_t is_hooked = 0;
static uint32_t intercepted_val = 0;
static void hook_target()
{
    //DryosDebugMsg(0, 15, "in hook code"); // hangs in this context
    //info_led_blink(3, 150, 150); // also hangs in this context
    //hook_result = 1; // this is okay

    uint32_t val;

    // r1 contains size to alloc
    asm __volatile__ (
        "mov %0, r1" : "=r" (val)
    );

    intercepted_val = val;
}

void hook_memoryManager_AllocateMemory()
{
/*
Hook code:
.syntax unified
.code 16

hook_function_address:
    .align 2
    .word 0x01010101
ret_address:
    .word 0x02020202

code:
    .align 2
    nop
    nop
    nop
    nop
    push {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, lr}
    ldr r6, hook_function_address
    blx r6

ret:
    pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, lr}
    // do stolen instruction here
    push { r4, r5, r6, r7, r8, r9, r10, lr  } // from df007a64
    ldr pc, #ret_address

arm-none-eabi-gcc -c -fPIC -march=armv7-a -mthumb arm_test.S && arm-none-eabi-objdump -drwC arm_test.o

00000008 <code>:
   8:	bf00      	nop
   a:	bf00      	nop
   c:	bf00      	nop
   e:	bf00      	nop
  10:	e92d 47ff 	stmdb	sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, lr}
  14:	f85f 6018 	ldr.w	r6, [pc, #-24]	; 0 <hook_function_address>
  18:	47b0      	blx	r6

0000001a <ret>:
  1a:	e8bd 47ff 	ldmia.w	sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, lr}
  1e:	e92d 47f0 	stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, lr}
  22:	f85f f020 	ldr.w	pc, [pc, #-32]	; 4 <ret_address>
  26:	bf00      	nop
*/

    uint32_t stage2_addr = 0xdf00f5c1; // Note thumb bit set on both of these,
    uint32_t hook_addr = 0xdf007a65;   // when using below, sometimes you must
                                       // adjust this, sometimes not.

    // setup 2nd stage hook in empty space at 0xdf00f600
    // (allows 1st stage hook to be shorter, as within 32MB)
    int *stage2 = (int *)(stage2_addr & 0xfffffffe);
    *(stage2 + 0) = (uint32_t)hook_target | 0x1; // ensure Thumb bit set
    *(stage2 + 1) = 0xdf007a69; // addr to jump back to after hook code finished

    *(stage2 + 2) = 0xbf00bf00;
    *(stage2 + 3) = 0xbf00bf00;
    *(stage2 + 4) = 0x47ffe92d;
    *(stage2 + 5) = 0x6018f85f;
    *(stage2 + 6) = 0xe8bd47b0;
    *(stage2 + 7) = 0xe92d47ff;
    *(stage2 + 8) = 0xf85f47f0;
    *(stage2 + 9) = 0xbf00f020;
    *(stage2 + 10) = 0xbf00bf00;
    sync_caches();

// 1st stage hook
    // insert jump to 2nd stage hook

    // What instruction to hook with depends on whether
    // the target instruction is Arm or Thumb.  If we used
    // bx reg we could avoid this, but then we need to
    // modify more bytes for the hook.
    //
    // Here we are hooking Thumb code, and this code is Thumb,
    // so we can use b.w.  The T4 encoding allows the most range,
    // 23 bits, +-16MB, 0xfffffe to ff000002
    //
    // The encoding is kind of funky, the bits of the branch target
    // are non-contiguous:
    // https://developer.arm.com/documentation/ddi0406/cb/Application-Level-Architecture/Instruction-Details/Alphabetical-list-of-instructions/B?lang=en
    // 
    // 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0|15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0|
    // ------------------------------------------------------------------------------------------------
    //  1  1  1  1  0| S|            imm10            | 1  0|J1| 1|J2|             imm11              |
    //
    // I1 = NOT(J1 EOR S);  I2 = NOT(J2 EOR S);  imm32 = SignExtend(S:I1:I2:imm10:imm11:’0’, 32);

    uint32_t hook_instr = 0x9000f000; // all the fixed bits set, NB, words are swapped from above,
                                      // because Thumb mode is 16-bit little-endian
    int32_t offset = ((stage2_addr + 10) - hook_addr - 4) / 2; // +8 to skip the consts preceding the stage2 start,
                                                               // then some of the nop slide "for luck", 
                                                               // -4 for PC offset in Thumb mode,
                                                               // /2 because offsets are encoded as 16-bit wide instruction count
    DryosDebugMsg(0, 15, "raw offset: 0x%x", stage2_addr - hook_addr);
    uint32_t sign_bit = 0;
    if (offset < 0)
        sign_bit = 1 << 10;
    hook_instr |= sign_bit;

    if (offset > 0x7fffff || offset < -0x7fffff)
    {
        DryosDebugMsg(0, 15, "hook too far to encode, offset: 0x%x", offset);
        goto bail;
    }

    uint32_t imm11 = offset & 0x7ff; // lowest 11 bits
    uint32_t imm10 = (offset >> 11) & 0x3ff;
    hook_instr |= imm10;
    hook_instr |= (imm11 << 16);

    uint32_t i1 = (offset >> 22) & 0x1;
    uint32_t i2 = (offset >> 21) & 0x1;
    uint32_t j1, j2;
    if (sign_bit)
    {
        j1 = i1;
        j2 = i2;
    }
    else
    {
        j1 = i1 ^ 1;
        j2 = i2 ^ 1;
    }
    hook_instr |= j1 << (16 + 13);
    hook_instr |= j2 << (16 + 11);

    // last sanity check!
    DryosDebugMsg(0, 15, "hook instr to insert: 0x%x", hook_instr);
    DryosDebugMsg(0, 15, "hook insert at: 0x%x", hook_addr & 0xfffffffe);
    msleep(1000);
    //return;

    // activate hook, this will break things if
    // you did anything wrong
    *(int *)(hook_addr & 0xfffffffe) = hook_instr;
    sync_caches();
    is_hooked = 1;
    DryosDebugMsg(0, 15, "hook after insert: 0x%x", *(int *)(hook_addr & 0xfffffffe));

bail:
    return;
}
#endif

#if 1
static void test_task(void *size)
{
    DryosDebugMsg(0, 15, " ==== test_task, size: 0x%x", (int)size);
}
#endif

void mem_to_file(char *name, uint32_t addr, uint32_t size)
{
    FILE *f = NULL;
    f = FIO_CreateFile(name);
    if (!f)
        return;
    FIO_WriteFile(f, (uint32_t *)addr, size);
    FIO_CloseFile(f);
}

#if 0 && defined(CONFIG_200D) && defined(CONFIG_MMU_REMAP)
#include "sgi.h"
#include "cpu.h"
#include "patch_mmu.h"
#include "mmu_utils.h"
extern int uart_printf(const char *fmt, ...);
extern int send_software_interrupt(uint32_t interrupt, uint32_t cpu_id);
extern int apply_data_patch(struct mmu_config *, struct region_patch *);
extern void change_mmu_tables(uint8_t *ttbr0, uint8_t *ttbr1, uint32_t cpu_id);

static const unsigned char earl_grey_str[] = "Earl Grey, hot";

#if CONFIG_FW_VERSION == 101 // ensure our hard-coded patch addresses are not broken
                             // by a FW upgrade
struct region_patch mmu_data_patches_debug[] =
{
    {
        // replace "Dust Delete Data" with "Earl Grey, hot",
        // as a low risk (non-code) test that MMU remapping works.
        .patch_addr = 0xf00d84e7,
        .orig_content = NULL,
        .patch_content = earl_grey_str,
        .size = sizeof(earl_grey_str),
        .description = "Tea"
    }
};

static void test_patch(void *unused)
{
    uint32_t cpu_id = get_cpu_id();
    DryosDebugMsg(0, 15, "Post-patch, %d: 0x%x", cpu_id, *(int *)0xf00d84e7);
}

#endif // CONFIG_FW_VERSION == 101
#endif // 200D && REMAP

#if 0 && defined(CONFIG_200D)
extern int uart_printf(const char *fmt, ...);
void print_match(uint32_t addr)
{
    for (uint32_t offset = 0x0; offset < 0x40; offset += 0x10)
    {
        uart_printf("0x%08x: ", addr + offset);
        for (uint32_t i = 0; i < 0x10; i += 4)
        {
            uart_printf("%02x %02x %02x %02x ",
                        *(uint8_t *)(addr + offset + i),
                        *(uint8_t *)(addr + offset + i + 1),
                        *(uint8_t *)(addr + offset + i + 2),
                        *(uint8_t *)(addr + offset + i + 3)
                      );
        }
        uart_printf("\n");
    }
    uart_printf("\n");
}

void dump_match(uint32_t addr)
{
    FILE* f = FIO_CreateFileOrAppend("ML/LOGS/iso_hunt.log");
    // CMOS ISO table is setup by a DMA read from f8910000,
    // but the dst is a heap address and not reliably predictable.
    // The DMA src addr is recorded in some linked-list struct, possibly
    // property related.  That struct also holds the dst addr.

    my_fprintf(f, "Orig addr: 0x%08x\n", addr);
    addr = addr & 0xfffffff0;
    for (uint32_t offset = 0x0; offset < 0x100; offset += 0x10)
    {
        my_fprintf(f, "0x%08x: ", addr + offset);
        for (uint32_t i = 0; i < 0x10; i += 4)
        {
            my_fprintf(f, "%02x %02x %02x %02x ",
                        *(uint8_t *)(addr + offset + i),
                        *(uint8_t *)(addr + offset + i + 1),
                        *(uint8_t *)(addr + offset + i + 2),
                        *(uint8_t *)(addr + offset + i + 3)
                      );
        }
        my_fprintf(f, "\n");
    }
    FIO_CloseFile(f);
}

static uint32_t ___get_photo_cmos_iso_start_200d(void)
{
    uint32_t addr = 0x3e0000;
    uint32_t max_search_addr = 0xa00000;

    uint32_t rom_copy_start = 0xe1980000;
    uint32_t ram_copy_start = 0;

    // search for DMA src addr, to find our dst addr
    for (; addr < max_search_addr; addr += 4)
    {
        if (*(uint32_t *)addr == rom_copy_start)
        {
            // A bunch of checks to give us higher confidence
            // we found the right value.  So far, none of these
            // have been required; the first hit is the correct
            // one.  But these are cheap checks, and should avoid
            // ever finding a random match on the 32-bit DMA addr value.

            uint32_t *probe = (uint32_t *)addr;
            // we expect to find 2 copies of the DMA src addr nearby
            if (probe[0] != probe[4])
                continue;
            if (probe[0] != probe[5])
                continue;
            DryosDebugMsg(0, 15, "Found DMA addr copies");

//            ram_copy_start = probe[6] + 0x4538;
            ram_copy_start = probe[6] + 0x4fb8;
            // we expect this to be Uncacheable
            if (ram_copy_start == (uint32_t)CACHEABLE(ram_copy_start))
                continue;

            // we expect to find the original ISO value
            if (*(uint32_t *)ram_copy_start != 0x0b400000)
                continue;

            // passed all checks, stop search
            DryosDebugMsg(0, 15, "Found ram_copy_start, 0x%08x: 0x%08x",
                          &probe[6], ram_copy_start);
            break;
        }
    }
    if (*(uint32_t *)addr != rom_copy_start || addr >= max_search_addr)
    {
        DryosDebugMsg(0, 15, "Failed to find rom_copy_start!");
        return 0; // failed to find target
    }

#if 1 // SJE FIXME remove this, it's debug code!
    // dump the table to check contents in case we later throw isoless err()
    FILE* f = FIO_CreateFile("ML/LOGS/iso_tabl.bin");
    FIO_WriteFile(f, (uint32_t *)ram_copy_start, 0x2000);
    FIO_CloseFile(f);
#endif

    return ram_copy_start;
}
#endif

int yuv_dump_sec = 0;
static void run_test()
{
    DryosDebugMsg(0, 15, "run_test fired");

#if 0 && defined(CONFIG_200D)
    #include "patch.h"

    // Three candidate CMOS ISO tables, RAM addresses
    // (not stable in theory but are for my cam, real code
    // should search for these)
    // 0x21f9dc0: +0x19c0, size 36, count 24
    // 0x21fc938: +0x4538, size 112 / 0x70, count 24
    // 0x21fd3b8: +0x4fb8, size 112 / 0x70, count 24

    msleep(100);
    uint32_t *p = NULL;
    for (p = (uint32_t *)0x2000; p < (uint32_t *)0x2800000; p++)
    {
        if (p[0] == 0x0b400000
            && p[1] == 0x0cc03333
            && p[2] == 0x0d001f1f
            && p[3] == 0x0d401f1f)
        {
            //DryosDebugMsg(0, 15, "Match: 0x%08x", p);
            print_match((uint32_t)p);
        }
    }
    //DryosDebugMsg(0, 15, " ==== Search finished ====", p);

    for (p = (uint32_t *)0x2000; p < (uint32_t *)0x2800000; p++)
    {
        if (p[0] == 0x0b400000
            && p[1] == 0x0cc03333
            && p[2] == 0xffffffff
            && p[3] == 0x03080201)
        {
            //DryosDebugMsg(0, 15, "Match: 0x%08x", p);
            print_match((uint32_t)p);
        }
    }
    //DryosDebugMsg(0, 15, " ==== Search finished ====", p);
/*
    uint32_t addr, size, count;
    for (addr = 0x21f9dc0, size = 36, count = 24; count > 0; count--)
    {
        if ((*(uint32_t *)addr & 0xfff00000) == 0x0b400000)
            patch_memory(addr, *(uint32_t *)addr, 0x0b444000, NULL);
        addr += size;
    }
*/


#endif

#if 0 && defined(CONFIG_200D)
    //___get_photo_cmos_iso_start_200d();
    DryosDebugMsg(0, 15, "0x421fc938: 0x%08x", *(uint32_t *)0x421fc938);
    DryosDebugMsg(0, 15, "0x21fc938:  0x%08x", *(uint32_t *)0x21fc938);

/*
    info_led_on();
    // CMOS ISO table is setup by a DMA read from f8910000,
    // but the dst is a heap address and not reliably predictable.
    // The DMA src addr is recorded in some linked-list struct, possibly
    // property related.  That struct also holds the dst addr.
    uint32_t addr = 0x300000;
    while (addr <  0x3000000)
    {
        if (*(uint32_t *)addr == 0x0b400000
            && *(uint32_t *)(addr + 4) == 0x0cc03333)
        {
            DryosDebugMsg(0, 15, "Possible match: 0x%08x\n", addr);
            dump_match(addr);
        }
        addr += 4;
    }
    info_led_off();
*/
#endif

#if 0 && defined(CONFIG_550D)
    // try to walk to the CMOS ISO tables, logging the steps along the way

    info_led_on();
    FILE* f = FIO_CreateFile("ML/LOGS/iso_hunt.log");
    // CMOS ISO table is setup by a DMA read from f8910000,
    // but the dst is a heap address and not reliably predictable.
    // The DMA src addr is recorded in some linked-list struct, possibly
    // property related.  That struct also holds the dst addr.
    uint32_t addr = 0x3d0000;
    while (*(uint32_t *)addr != 0xf8910000
           && addr < 0x800000)
    {
        addr += 4;
    }
    if (*(uint32_t *)addr != 0xf8910000)
        goto close;
    my_fprintf(f, "Found f8910000: 0x%08x\n", addr);
    my_fprintf(f, "+0x18: 0x%08x\n", *(uint32_t *)(addr + 0x18));

close:
    FIO_CloseFile(f);
    info_led_off();
#endif

#if 0 && (defined(CONFIG_200D) || defined(CONFIG_850D))
    // trigger an assert
    extern void debug_assert(char *msg, char *file, int line);
    debug_assert("LIFE == FAIR", "this file", 1);
    // or the following will trigger an exception on MMU cams
//    int crash_now_please = *(int *)0x0;
//    DryosDebugMsg(0, 15, "not unused: 0x%x", crash_now_please);
#endif

#if 0
    if (is_hooked)
    {
        DryosDebugMsg(0, 15, "last value seen: 0x%x", intercepted_val);
    }
    else
    {
        hook_memoryManager_AllocateMemory();
    }
    DryosDebugMsg(0, 15, "returned from hooking");
#endif

#if 0
    static int dm_toggle = 1;
    if (dm_toggle)
    {
        DryosDebugMsg(0, 15, "Logging less");
//        dm_set_store_level(0x80, 0x17);
        dm_set_print_level(0x0, 0x8);
        dm_set_store_level(0x0, 0x8);
        dm_set_print_level(0x80, 0x5);
        dm_set_store_level(0x80, 0x5);
        dm_toggle = 0;
    }
    else
    {
        DryosDebugMsg(0, 15, "Logging more");
//        dm_set_store_level(0x80, 0x1); // re-enables SRM related logging
        dm_set_print_level(0x0, 0x5);
        dm_set_store_level(0x0, 0x5);
        dm_set_print_level(0x80, 0x3);
        dm_set_store_level(0x80, 0x3);
        dm_toggle = 1;
    }
#endif

}

#ifdef FEATURE_BOOTFLAG_MENU
static void bootflag_disable(void* priv, int delta)
{
    console_show();
    printf("Call DisableBootDisk()\n");

    call("DisableBootDisk");

    printf("done.\n");
}

static void bootflag_enable(void* priv, int delta)
{
  console_show();
  printf("Call EnableBootDisk()\n");

  call("EnableBootDisk");

  printf("done.\n");
}
#endif

static void unmount_sd_card()
{
    extern void FSUunMountDevice(int drive);
    
    msleep(1000);
    console_clear();
    console_show();
    
    /* call shutdown hooks that need to save configs */
    extern int module_shutdown();
    config_save_at_shutdown();
    module_shutdown();
    
    /* unmount the SD card */
    FSUunMountDevice(2);
    
    printf("Unmounted SD card.\n");
    printf("You may now copy files remotely on your wifi card.\n");
    printf("Press shutter halfway to reboot.\n");
    
    while (!get_halfshutter_pressed())
    {
        info_led_on();
        msleep(10);
    }

    int reboot = 0;
    prop_request_change(PROP_REBOOT, &reboot, 4);
}

#if CONFIG_DEBUGMSG
static void dbg_draw_props(int changed);
static unsigned dbg_last_changed_propindex = 0;

void
memfilt(void* m, void* M, int value)
{
    int k = 0;
    bmp_printf(FONT_SMALL, 0, 0, "%8x", value);
    for (void* i = m; i < M; i ++)
    {
        if ((*(uint8_t*)i) == value)
        {
            int x =  10 + 4 * 22 * (k % 8);
            int y =  10 + 12 * (k / 8);
            bmp_printf(FONT_SMALL, x, y, "%8x", i);
            k = (k + 1) % 240;
        }
    }
    int x =  10 + 4 * 22 * (k % 8);
    int y =  10 + 12 * (k / 8);
    bmp_printf(FONT_SMALL, x, y, "        ");
}
#endif

static int screenshot_sec = 0;


#ifdef CONFIG_HEXDUMP

CONFIG_INT("hexdump", hexdump_addr, 0x24298);

int hexdump_enabled = 0;

static MENU_UPDATE_FUNC (hexdump_print_value_hex)
{
    MENU_SET_VALUE("0x%x",
        MEMX(hexdump_addr)
    );
}

static MENU_UPDATE_FUNC (hexdump_print_value_int32)
{
    MENU_SET_VALUE(
        "%d",
        MEMX(hexdump_addr)
    );
}

static MENU_UPDATE_FUNC (hexdump_print_value_int16)
{
    int value = MEMX(hexdump_addr);
    MENU_SET_VALUE(
        "%d %d",
        value & 0xFFFF, (value>>16) & 0xFFFF
    );
}

static MENU_UPDATE_FUNC (hexdump_print_value_int8)
{
    int value = MEMX(hexdump_addr);
    MENU_SET_VALUE(
        "%d %d %d %d",
        (int8_t)( value      & 0xFF),
        (int8_t)((value>>8 ) & 0xFF),
        (int8_t)((value>>16) & 0xFF),
        (int8_t)((value>>24) & 0xFF)
    );
}

static MENU_UPDATE_FUNC (hexdump_print_value_str)
{
    if (hexdump_addr & 0xF0000000) return;
    MENU_SET_VALUE(
        "%s",
        (char*)hexdump_addr
    );
}

static void
hexdump_toggle_value_int32(void * priv, int delta)
{
    MEM(hexdump_addr) += delta;
}

static void
hexdump_toggle_value_int16(void * priv, int delta)
{
    (*(int16_t*)(hexdump_addr+2)) += delta;
}

int hexdump_prev = 0;
void hexdump_back(void* priv, int dir)
{
    hexdump_addr = hexdump_prev;
}
void hexdump_deref(void* priv, int dir)
{
    if (dir < 0) hexdump_back(priv, dir);
    hexdump_prev = hexdump_addr;
    hexdump_addr = MEMX(hexdump_addr);
}
#endif

static int crash_log_requested = 0;
void request_crash_log(int type)
{
    crash_log_requested = type;
}

static int core_dump_requested = 0;
static int core_dump_req_from = 0;
static int core_dump_req_size = 0;
void request_core_dump(int from, int size)
{
    core_dump_req_from = from;
    core_dump_req_size = size;
    core_dump_requested = 1;
}

extern int GetFreeMemForAllocateMemory();

#ifdef CONFIG_CRASH_LOG
static void save_crash_log()
{
    static char log_filename[100];

    int log_number = 0;
    for (log_number = 0; log_number < 100; log_number++)
    {
        snprintf(log_filename, sizeof(log_filename), crash_log_requested == 1 ? "CRASH%02d.LOG" : "ASSERT%02d.LOG", log_number);
        uint32_t size;
        if( FIO_GetFileSize( log_filename, &size ) != 0 ) break;
        if (size == 0) break;
    }

    FILE* f = FIO_CreateFile(log_filename);
    if (f)
    {
        my_fprintf(f, "%s\n", get_assert_msg());
        my_fprintf(f,
            "Magic Lantern version: %s\n"
            "Git commit: %s\n"
            "Built on %s by %s.\n",
            build_version,
            build_id,
            build_date,
            build_user);

        int M = GetFreeMemForAllocateMemory();
        int m = MALLOC_FREE_MEMORY;
        my_fprintf(f,
            "Free Memory  : %dK + %dK\n",
            m/1024, M/1024
        );

        FIO_CloseFile(f);
    }

    msleep(1000);

    if (crash_log_requested == 1)
    {
        NotifyBox(5000, "Crash detected - log file saved.\n"
                        "Pls send CRASH%02d.LOG to ML devs.\n"
                        "\n"
                        "%s", log_number, get_assert_msg());
    }
    else
    {
        printf("%s\n", get_assert_msg());
        console_show();
    }

}

static void crash_log_step()
{
    static int dmlog_saved = 0;
    if (crash_log_requested)
    {
        //~ beep();
        save_crash_log();
        crash_log_requested = 0;
        msleep(2000);
    }

    if (core_dump_requested)
    {
        NotifyBox(100000, "Saving core dump, please wait...\n");
        dump_seg((void*)core_dump_req_from, core_dump_req_from + core_dump_req_size, "COREDUMP.DAT");
        NotifyBox(10000, "Pls send COREDUMP.DAT to ML devs.\n");
        core_dump_requested = 0;
    }

    //~ bmp_printf(FONT_MED, 100, 100, "%x ", get_current_dialog_handler());
    extern thunk ErrForCamera_handler;
    if (get_current_dialog_handler() == &ErrForCamera_handler)
    {
        if (!dmlog_saved)
        {
            beep();
            NotifyBox(10000, "Saving debug log...");
            call("dumpf");
        }
        dmlog_saved = 1;
    }
    else dmlog_saved = 0;
}
#endif

static void
debug_loop_task( void* unused ) // screenshot, draw_prop
{
    TASK_LOOP
    {
#ifdef CONFIG_HEXDUMP
        if (hexdump_enabled)
            bmp_hexdump(FONT_SMALL, 0, 480-120, (void*) hexdump_addr, 32*10);
#endif

        #ifdef FEATURE_SCREENSHOT
        if (screenshot_sec)
        {
            info_led_blink(1, 20, 1000-20-200);
            screenshot_sec--;
            if (!screenshot_sec)
                take_screenshot(SCREENSHOT_FILENAME_AUTO, SCREENSHOT_BMP | SCREENSHOT_YUV);
        }
        #endif

#if defined(CONFIG_200D) || defined(CONFIG_850D)
//#ifdef CONFIG_850D
        // SJE FIXME hack code to dump probably YUV buffers
        // (areas listed in smemShowFix with YUV in name)
        if (yuv_dump_sec)
        {
            info_led_blink(1, 20, 1000 - 20 - 200);
            yuv_dump_sec--;
            if (!yuv_dump_sec)
            {
                char path[100];

                // addr, size
                uint32_t regions[] = {
                #ifdef CONFIG_200D
                    *(DISP_VRAM_STRUCT_PTR + (0x70 / 4)), 0x00405600,
                    *(DISP_VRAM_STRUCT_PTR + (0x74 / 4)), 0x00405600,
                    *(DISP_VRAM_STRUCT_PTR + (0x78 / 4)), 0x00405600,
                    //0x5f3efe00, 0x00405600, // IMG_VRAM1
                    //0x5f7f5400, 0x00405600, // IMG_VRAM2
                    //0x5fbfaa00, 0x00405600, // IMG_VRAM3
                #elif defined(CONFIG_850D)
                    0x9F420000, 0x003F4800, // IMG_VRAM1
                #endif
                };

                for (uint32_t i = 0; i < sizeof(regions) / 4; i += 2)
                {
                    snprintf(path, sizeof(path), "%x.yuv", regions[i]);
                    mem_to_file(path, regions[i], regions[i + 1]);
                }
            }
        }
#endif

        #ifdef CONFIG_RESTORE_AFTER_FORMAT
        if (MENU_MODE)
        {
            HijackFormatDialogBox_main();
        }
        #endif

        #if CONFIG_DEBUGMSG
        if (draw_prop)
        {
            dbg_draw_props(dbg_last_changed_propindex);
            continue;
        }
        #endif

        #ifdef CONFIG_CRASH_LOG
        crash_log_step();
        #endif

        msleep(200);
    }
}

static void screenshot_start(void* priv, int delta)
{
    screenshot_sec = 10;
}

#ifdef FEATURE_SHOW_IMAGE_BUFFERS_INFO
static MENU_UPDATE_FUNC(image_buf_display)
{
    MENU_SET_VALUE(
        "%dx%d, %dx%d",
        vram_lv.width, vram_lv.height,
        vram_hd.width, vram_hd.height
    );
}
#endif

static MENU_UPDATE_FUNC(shuttercount_display)
{
#ifdef CONFIG_DIGIC_VIII // Digic 8 and up
    // just shutter count value
    MENU_SET_VALUE("%d", shutter_count);
#else
    MENU_SET_VALUE(
        "%dK = %d+%d",
        (shutter_count_plus_lv_actuations + 500) / 1000,
        shutter_count, shutter_count_plus_lv_actuations - shutter_count
    );
#endif

    if (shutter_count_plus_lv_actuations > CANON_SHUTTER_RATING*2)
    {
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Lets break Guiness World Records (rated lifespan %d).", CANON_SHUTTER_RATING);
    }
    else if (shutter_count_plus_lv_actuations > CANON_SHUTTER_RATING)
    {
        MENU_SET_WARNING(MENU_WARN_INFO, "Lifespans are for wimps (rated lifespan %d).", CANON_SHUTTER_RATING);
    }
    else if (shutter_count_plus_lv_actuations > CANON_SHUTTER_RATING/2)
    {
        MENU_SET_WARNING(MENU_WARN_INFO, "I hope I get to rated lifespan (rated lifespan %d).", CANON_SHUTTER_RATING);
    }
    else
    {
        MENU_SET_WARNING(MENU_WARN_INFO, "You may get around %d.", CANON_SHUTTER_RATING);
    }
}

#ifdef CONFIG_DIGIC_VIII // Digic 8 and up
static MENU_UPDATE_FUNC(totalshutter_display)
{
    MENU_SET_VALUE("%d", shutter_count);
    MENU_SET_WARNING(
        MENU_WARN_ADVICE,
        "May drift a bit - check after restart for accurate values.");
}
static MENU_UPDATE_FUNC(totalmirror_display)
{
    MENU_SET_VALUE("%d", total_mirror_count);
    MENU_SET_WARNING(
        MENU_WARN_ADVICE,
        "May drift a bit - check after restart for accurate values.");
}
static MENU_UPDATE_FUNC(totalshoot_display)
{
    MENU_SET_VALUE("%d", total_shots_count);
    MENU_SET_WARNING(
        MENU_WARN_ADVICE,
        "May drift a bit - check after restart for accurate values.");
}
#endif

#ifdef FEATURE_SHOW_CMOS_TEMPERATURE
#ifdef EFIC_CELSIUS
#define FAHRENHEIT (EFIC_CELSIUS * 9 / 5 + 32)
static MENU_UPDATE_FUNC(efictemp_display)
{
    MENU_SET_VALUE(
        "%d C, %d F, %d raw",
        EFIC_CELSIUS, FAHRENHEIT, efic_temp
    );
}
#else
static MENU_UPDATE_FUNC(efictemp_display)
{
    MENU_SET_VALUE(
        "%d raw (help needed)",
        efic_temp
    );
}
#endif
#endif

#if 0 // CONFIG_5D2
static void ambient_display(
    void *            priv,
    int            x,
    int            y,
    int            selected
)
{
    extern int lightsensor_raw_value;
    int ev = gain_to_ev_scaled(lightsensor_raw_value, 10);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Ambient light: %d.%d EV",
        ev/10, ev%10
    );
    menu_draw_icon(x, y, MNI_ON, 0);
}
#endif

#ifdef FEATURE_DEBUG_PROP_DISPLAY
static CONFIG_INT("prop.i", prop_i, 0);
static CONFIG_INT("prop.j", prop_j, 0);
static CONFIG_INT("prop.k", prop_k, 0);

static MENU_UPDATE_FUNC (prop_display)
{
    unsigned prop = (prop_i << 24) | (prop_j << 16) | (prop_k);
    int* data = 0;
    size_t len = 0;
    int err = prop_get_value(prop, (void **) &data, &len);
    MENU_SET_VALUE(
    "%8x: %d: %x %x %x %x\n"
        "'%s' ",
        prop,
        len,
        len > 0x00 ? data[0] : 0,
        len > 0x04 ? data[1] : 0,
        len > 0x08 ? data[2] : 0,
        len > 0x0c ? data[3] : 0,
        strlen((const char *) data) < 100 ? (const char *) data : ""
    );
}

void prop_dump()
{
    FILE* f = FIO_CreateFile("ML/LOGS/PROP.LOG");
    if (!f)
    {
        return;
    }

    FILE* g = FIO_CreateFile("ML/LOGS/PROP-STR.LOG");
    if (!g)
    {
        FIO_CloseFile(f);
        return;
    }

    unsigned i, j, k;

    for( i=0 ; i<256 ; i++ )
    {
        if (i > 0x10 && i != 0x80) continue;
        for( j=0 ; j<=0xA ; j++ )
        {
            for( k=0 ; k<0x50 ; k++ )
            {
                unsigned prop = 0
                    | (i << 24)
                    | (j << 16)
                    | (k <<  0);

                bmp_printf(FONT_LARGE, 0, 0, "PROP %x...", prop);
                int* data = 0;
                size_t len = 0;
                int err = prop_get_value(prop, (void **) &data, &len);
                if (!err)
                {
                    my_fprintf(f, "\nPROP %8x: %5d:", prop, len );
                    my_fprintf(g, "\nPROP %8x: %5d:", prop, len );
                    for (unsigned int i = 0; i < (MIN(len,40)+3)/4; i++)
                    {
                        my_fprintf(f, "%8x ", data[i]);
                    }
                    if (strlen((const char *) data) < 100) my_fprintf(g, "'%s'", data);
                }
            }
        }
    }
    FIO_CloseFile(f);
    FIO_CloseFile(g);
    beep();
    redraw();
}

static void prop_toggle_i(void* priv, int unused) {prop_i = prop_i < 5 ? prop_i + 1 : prop_i == 5 ? 0xE : prop_i == 0xE ? 0x80 : 0; }
static void prop_toggle_j(void* priv, int unused) {prop_j = MOD(prop_j + 1, 0x10); }
static void prop_toggle_k(void* priv, int dir) {if (dir < 0) prop_toggle_j(priv, dir); prop_k = MOD(prop_k + 1, 0x51); }
#endif

#ifdef CONFIG_KILL_FLICKER
void menu_kill_flicker()
{
    gui_stop_menu();
    canon_gui_disable_front_buffer();
}
#endif


extern MENU_UPDATE_FUNC(tasks_print);
extern MENU_UPDATE_FUNC(batt_display);
extern MENU_SELECT_FUNC(tasks_toggle_flags);

extern int show_cpu_usage_flag;

static int gui_events_show = 0;
static MENU_SELECT_FUNC(gui_events_toggle);

static struct menu_entry debug_menus[] = {
    MENU_PLACEHOLDER("File Manager"),
#ifdef CONFIG_HEXDUMP
    {
        .name = "Memory Browser",
        .priv = &hexdump_enabled,
        .max = 1,
        .help = "Display memory contents in real-time (hexdump).",
        .children =  (struct menu_entry[]) {
            {
                .name = "HexDump",
                .priv = &hexdump_addr,
                .max = 0x20000000,
                .unit = UNIT_HEX,
                .icon_type = IT_PERCENT,
                .help = "Address to be analyzed. Press Q to select the digit to edit."
            },
            {
                .name = "Pointer dereference",
                .select = hexdump_deref,
                .help = "Changes address to *(int*)addr [SET] or goes back [PLAY]."
            },
            {
                .name = "Val hex32",
                .update = hexdump_print_value_hex,
                .select = hexdump_toggle_value_int32,
                .help = "Value as hex."
            },
            {
                .name = "Val int32",
                .update = hexdump_print_value_int32,
                .select = hexdump_toggle_value_int32,
                .help = "Value as int32."
            },
            {
                .name = "Val int16",
                .update = hexdump_print_value_int16,
                .select = hexdump_toggle_value_int16,
                .help = "Value as 2 x int16. Toggle: changes second value."
            },
            {
                .name = "Val int8",
                .update = hexdump_print_value_int8,
                .help = "Value as 4 x int8."
            },
            {
                .name = "Val string",
                .update = hexdump_print_value_str,
                .help = "Value as string."
            },
            MENU_EOL
        },
    },
#endif
    /*{
        .name        = "Flashlight",
        .select        = flashlight_lcd,
        .select_reverse = flashlight_frontled,
        .help = "Turn on the front LED [PLAY] or make display bright [SET]."
    },*/
    #ifdef FEATURE_SCREENSHOT
    {
        .name   = "Screenshot - 10s",
        .select = screenshot_start,
        .help   = "Screenshot after 10 seconds => VRAMx.PPM.",
        .help2  = "The screenshot will contain BMP and YUV overlays."
    },
    #endif
/*    {
        .name = "Menu screenshots",
        .select     = (void (*)(void*,int))run_in_separate_task,
        .priv = screenshots_for_menu,
        .help = "Take a screenshot for each ML menu.",
    }, */
#if CONFIG_DEBUGMSG
    #if 0
    {
        .name = "Draw palette",
        .select        = bmp_draw_palette,
        .help = "Display a test pattern to see the color palette."
    },
    #endif
    {
        .name = "Spy properties",
        .priv = &draw_prop,
        .max = 1,
        .help = "Show properties as they change."
    },
/*    {
        .name        = "Dialog test",
        .select        = dlg_test,
        .help = "Dialog templates (up/dn) and color palettes (left/right)"
    },*/
#endif
    {
        .name        = "Dump ROM and RAM",
        .priv        = dump_rom_task,
        .select      = run_in_separate_task,
    #if defined(CONFIG_DIGIC_45)
        .help = "ROM0.BIN:F0000000, ROM1.BIN:F8000000, RAM4.BIN"
    #elif defined(CONFIG_DIGIC_6)
        .help = "ROM0.BIN:      NA, ROM1.BIN:FE000000, RAM4.BIN"
    #elif defined(CONFIG_DIGIC_78)
        .help = "ROM0.BIN:E0000000, ROM1.BIN:F0000000, RAM4.BIN"
    #endif
    },
    {
        .name        = "Dump image buffers",
        .priv        = dump_img_task,
        .select      = run_in_separate_task,
        .help = "Dump all image buffers (LV, HD, RAW) from current video mode."
    },
#ifdef FEATURE_UNMOUNT_SD_CARD
    {
        .name        = "Unmount SD card",
        .priv        = unmount_sd_card,
        .select      = run_in_separate_task,
        .help        = "Run before uploading files to a Wi-Fi card, to avoid data corruption.",
        .help2       = "No further writes will be performed on your card from the camera.",
    },
#endif
#ifdef FEATURE_DONT_CLICK_ME
    {
        .name        = "Don't click me!",
        .priv =         run_test,
        .select        = run_in_separate_task,
        .help = "The camera may turn into a 1DX or it may explode."
    },
#endif
#ifdef FEATURE_BOOTFLAG_MENU
    {
        .name       = "Bootflag settings",
        .select     = menu_open_submenu,
        .help       = "Change camera bootflag status",
        .children =  (struct menu_entry[]) {
            {
                .name   = "Disable bootflag",
                .select = bootflag_disable,
                .help   = "Calls DisableBootDisk EvProc"
            },
            {
                .name   = "Enable bootflag",
                .select = bootflag_enable,
                .help   = "Calls EnableBootDisk EvProc"
            },
            MENU_EOL,
        },
    },
#endif
#ifdef CONFIG_DEBUG_INTERCEPT
    {
        .name        = "DM Log",
        .priv        = j_debug_intercept,
        .select      = run_in_separate_task,
        .help = "Log DebugMessages"
    },
    {
        .name        = "TryPostEvent Log",
        .priv        = j_tp_intercept,
        .select      = run_in_separate_task,
        .help = "Log TryPostEvents"
    },
#endif
#ifdef FEATURE_SHOW_TASKS
    {
        .name = "Show tasks",
        .select = menu_open_submenu,
        .help = "Displays the tasks started by Canon and Magic Lantern.",
        .children =  (struct menu_entry[]) {
            {
                .name = "Task list",
                .update = tasks_print,
                .select = tasks_toggle_flags,
                #ifdef CONFIG_VXWORKS
                .help = "Task info: name, priority, stack memory usage.",
                #else
                .help = "Task info: ID, name, priority, wait_id, mem, state.",
                #endif
            },
            MENU_EOL
        }
    },
#endif
#ifdef FEATURE_SHOW_CPU_USAGE
#ifdef CONFIG_TSKMON
    {
        .name = "Show CPU usage",
        .priv = &show_cpu_usage_flag,
        .max = 3,
        .choices = (const char *[]) {"OFF", "Percentage", "Busy tasks (ABS)", "Busy tasks (REL)"},
        .help = "Display total CPU usage (percentage).",
    },
#endif
#endif
#ifdef FEATURE_SHOW_GUI_EVENTS
    {
        .name   = "Show GUI events",
        .priv   = &gui_events_show,
        .select = gui_events_toggle,
        .max    = 1,
        .help   = "Display GUI events (button codes).",
    },
#endif
#ifdef FEATURE_GUIMODE_TEST
    {
        .name = "Test GUI modes (DANGEROUS!!!)",
        .select = run_in_separate_task,
        .priv = guimode_test,
        .help = "Cycle through all GUI modes and take screenshots.",
    },
#endif
    MENU_PLACEHOLDER("Free Memory"),
#ifdef FEATURE_SHOW_IMAGE_BUFFERS_INFO
    {
        .name = "Image buffers",
        .update = image_buf_display,
        .icon_type = IT_ALWAYS_ON,
        .help = "Display the image buffer sizes (LiveView and Craw).",
        //.essential = 0,
    },
#endif
#ifdef FEATURE_SHOW_SHUTTER_COUNT
    {
        .name = "Shutter Count",
        .update = shuttercount_display,
        //.essential = FOR_MOVIE | FOR_PHOTO,
        #ifdef CONFIG_DIGIC_VIII // Digic 8 and up
        .help = "Number of shutter actions. Open submenu to learn more.",
        .select = menu_open_submenu,
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Total Shutter",
                .update = totalshutter_display,
                .icon_type = IT_ALWAYS_ON,
                .help = "Number of mechanical shutter actions (incl. sensor cleaning)",
            },
            {
                .name = "Total Mirror",
                .update = totalmirror_display,
                .icon_type = IT_ALWAYS_ON,
                .help = "Number of mirror move actions (DSLR, 0 on mirrorless)",
            },
            {
                .name = "Total Shots",
                .update = totalshoot_display,
                .icon_type = IT_ALWAYS_ON,
                .help = "Number of photos made. Incl. silent (electronic) shots",
            },
            MENU_EOL,
        },
        #else // no submenu, classic style
        .help = "Number of pics taken + number of LiveView actuations",
        .icon_type = IT_ALWAYS_ON,
        #endif
    },
#endif

#ifdef FEATURE_SHOW_CMOS_TEMPERATURE
    {
        .name = "Internal Temp",
        .update = efictemp_display,
        .icon_type = IT_ALWAYS_ON,
	 #ifdef EFIC_CELSIUS
        .help = "EFIC chip temperature (somewhere on the mainboard).",
	 #else
	.help = "EFIC chip temperature (raw values).",
	.help2 = "http://www.magiclantern.fm/forum/index.php?topic=9673.0",
	 #endif
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },
#endif
    #if 0 // CONFIG_5D2
    {
        .name = "Ambient light",
        //~.display = ambient_display,
        .help = "Ambient light from the sensor under LCD, in raw units.",
        //.essential = FOR_MOVIE | FOR_PHOTO,
    },
    #endif
#ifdef CONFIG_BATTERY_INFO
    {
        .name = "Battery level",
        .update = batt_display,
        .help = "Battery remaining. Wait for 2% discharge before reading.",
        .icon_type = IT_ALWAYS_ON,
    },
#endif
#ifdef FEATURE_DEBUG_PROP_DISPLAY
    {
        .name = "PROP Display",
        .update = prop_display,
        .select = prop_toggle_k,
        // .select_reverse = prop_toggle_j,
        .select_Q = prop_toggle_i,
        .help = "Raw property display (read-only)",
    },
#endif
};

#if CONFIG_DEBUGMSG

static void * debug_token;

static void
debug_token_handler(
    void *            token,
    void *            arg1,
    void *            arg2,
    void *            arg3
)
{
    debug_token = token;
    DebugMsg( DM_MAGIC, 3, "token %08x arg=%08x %08x %08x",
        (unsigned) token,
        (unsigned) arg1,
        (unsigned) arg2,
        (unsigned) arg3
    );
}

//~ static int dbg_propn = 0;
#define MAXPROP 30
static unsigned dbg_props[MAXPROP] = {0};
static unsigned dbg_props_len[MAXPROP] = {0};
static unsigned dbg_props_a[MAXPROP] = {0};
static unsigned dbg_props_b[MAXPROP] = {0};
static unsigned dbg_props_c[MAXPROP] = {0};
static unsigned dbg_props_d[MAXPROP] = {0};
static unsigned dbg_props_e[MAXPROP] = {0};
static unsigned dbg_props_f[MAXPROP] = {0};
static void dbg_draw_props(int changed)
{
    dbg_last_changed_propindex = changed;
    int i;
    for (i = 0; i < dbg_propn; i++)
    {
    	int x =  80;
        unsigned property = dbg_props[i];
        unsigned len = dbg_props_len[i];
#ifdef CONFIG_VXWORKS
        uint32_t fnt = FONT_MONO_20;
        unsigned y =  15 + i * 20;
#else
        uint32_t fnt = FONT_MONO_12;
        int y =  15 + i * 12;
#endif
        if (i == changed) fnt = FONT(fnt, 5, COLOR_BG);
        char msg[100];
        snprintf(msg, sizeof(msg),
#ifdef CONFIG_VXWORKS
            "%08x %04x: %8lx %8lx %8lx %8lx",
#else
            "%08x %04x: %8lx %8lx %8lx %8lx %8lx %8lx",
#endif
            property,
            len,
            len > 0x00 ? dbg_props_a[i] : 0,
            len > 0x04 ? dbg_props_b[i] : 0,
            len > 0x08 ? dbg_props_c[i] : 0,
            len > 0x0c ? dbg_props_d[i] : 0
            #ifndef CONFIG_VXWORKS
           ,len > 0x10 ? dbg_props_e[i] : 0,
            len > 0x14 ? dbg_props_f[i] : 0
            #endif
        );
        bmp_puts(fnt, &x, &y, msg);
    }
}


static void *
debug_property_handler(
    unsigned        property,
    void *            UNUSED_ATTR( priv ),
    void *            buf,
    unsigned        len
)
{
    const uint32_t * const addr = buf;

    /*printf("Prop %08x: %2x: %08x %08x %08x %08x\n",
        property,
        len,
        len > 0x00 ? addr[0] : 0,
        len > 0x04 ? addr[1] : 0,
        len > 0x08 ? addr[2] : 0,
        len > 0x0c ? addr[3] : 0
    );*/

    if( !draw_prop )
        goto ack;

    // maybe the property is already in the array
    int i;
    for (i = 0; i < dbg_propn; i++)
    {
        if (dbg_props[i] == property)
        {
            dbg_props_len[i] = len;
            dbg_props_a[i] = addr[0];
            dbg_props_b[i] = addr[1];
            dbg_props_c[i] = addr[2];
            dbg_props_d[i] = addr[3];
            dbg_props_e[i] = addr[4];
            dbg_props_f[i] = addr[5];
            dbg_draw_props(i);
            goto ack; // return with cleanup
        }
    }
    // new property
    if (dbg_propn >= MAXPROP) dbg_propn = MAXPROP-1; // too much is bad :)
    dbg_props[dbg_propn] = property;
    dbg_props_len[dbg_propn] = len;
    dbg_props_a[dbg_propn] = addr[0];
    dbg_props_b[dbg_propn] = addr[1];
    dbg_props_c[dbg_propn] = addr[2];
    dbg_props_d[dbg_propn] = addr[3];
    dbg_props_e[dbg_propn] = addr[4];
    dbg_props_f[dbg_propn] = addr[5];
    dbg_propn++;
    dbg_draw_props(dbg_propn);

ack:
    return (void*)_prop_cleanup( debug_token, property );
}

#endif

#if defined(CONFIG_500D)
#define num_properties 2048
#elif defined(CONFIG_5DC)
#define num_properties 202
#else
#define num_properties 8192
#endif

void
debug_init( void )
{
#if CONFIG_DEBUGMSG
    draw_prop = 0;
    static unsigned* property_list = 0;
    if (!property_list) property_list = malloc(num_properties * sizeof(unsigned));
    if (!property_list) return;
    unsigned i, j, k;
    unsigned actual_num_properties = 0;

    unsigned is[] = {0x2, 0x80, 0xe, 0x5, 0x4, 0x1, 0x0};
    for( i=0 ; i<COUNT(is) ; i++ )
    {
        for( j=0 ; j<=0xA ; j++ )
        {
            for( k=0 ; k<0x50 ; k++ )
            {
                unsigned prop = 0
                    | (is[i] << 24)
                    | (j << 16)
                    | (k <<  0);

                property_list[ actual_num_properties++ ] = prop;

                if( actual_num_properties >= num_properties )
                    goto thats_all;
            }
        }
    }

thats_all:
    prop_register_slave(
        property_list,
        actual_num_properties,
        debug_property_handler,
        0,
        0
    );
#endif

}

CONFIG_INT( "debug.timed-dump",        timed_dump, 0 );

//~ CONFIG_INT( "debug.dump_prop", dump_prop, 0 );
//~ CONFIG_INT( "debug.dumpaddr", dump_addr, 0 );
//~ CONFIG_INT( "debug.dumplen", dump_len, 0 );

/*
struct bmp_file_t * logo = (void*) -1;
void load_logo()
{
    if (logo == (void*) -1)
        logo = bmp_load("ML/DOC/logo.bmp",0);
}
void show_logo()
{
    load_logo();
    if ((int)logo > 0)
    {
        kill_flicker(); msleep(100);
        bmp_draw_scaled_ex(logo, 360 - logo->width/2, 240 - logo->height/2, logo->width, logo->height, 0, 0);
    }
}*/

// initialization done AFTER reading the config file,
// but BEFORE starting ML tasks
void
debug_init_stuff( void )
{
    //~ set_pic_quality(PICQ_RAW);

    #ifdef CONFIG_WB_WORKAROUND
    if (is_movie_mode())
    {
        extern void restore_kelvin_wb(); /* movtweaks.c */
        restore_kelvin_wb();
    }
    #endif

    #ifdef CONFIG_5D3
    _card_tweaks();
    #endif
}

TASK_CREATE( "debug_task", debug_loop_task, 0, 0x1e, 0x2000 );


#ifdef CONFIG_INTERMEDIATE_ISO_INTERCEPT_SCROLLWHEEL
    #ifndef FEATURE_EXPO_ISO
    #error This requires FEATURE_EXPO_ISO.
    #endif

int iso_intercept = 1;

void iso_adj(int prev_iso, int sign)
{
    if (sign)
    {
        lens_info.raw_iso = prev_iso;
        iso_intercept = 0;
        iso_toggle(0, sign);
        if (lens_info.iso > 6400) lens_set_rawiso(0);
        iso_intercept = 1;
    }
}

int iso_adj_flag = 0;
int iso_adj_old = 0;
int iso_adj_sign = 0;

void iso_adj_task(void* unused)
{
    TASK_LOOP
    {
        msleep(20);
        if (iso_adj_flag)
        {
            iso_adj_flag = 0;
            iso_adj(iso_adj_old, iso_adj_sign);
            lens_display_set_dirty();
        }
    }
}

TASK_CREATE("iso_adj_task", iso_adj_task, 0, 0x1a, 0);

PROP_HANDLER(PROP_ISO)
{
    static unsigned int prev_iso = 0;
    if (!prev_iso) prev_iso = lens_info.raw_iso;

    if (iso_intercept && ISO_ADJUSTMENT_ACTIVE && lv && lv_disp_mode == 0 && is_movie_mode())
    {
        if ((prev_iso && buf[0] && prev_iso < buf[0]) || // 100 -> 200 => +
            (prev_iso >= 112 && buf[0] == 0)) // 3200+ -> auto => +
        {
            //~ bmp_printf(FONT_LARGE, 50, 50, "[%d] ISO+", k++);
            iso_adj_old = prev_iso;
            iso_adj_sign = 1;
            iso_adj_flag = 1;
        }
        else if ((prev_iso && buf[0] && prev_iso > buf[0]) || // 200 -> 100 => -
            (prev_iso <= 88 && buf[0] == 0)) // 400- -> auto => -
        {
            //~ bmp_printf(FONT_LARGE, 50, 50, "[%d] ISO-", k++);
            iso_adj_old = prev_iso;
            iso_adj_sign = -1;
            iso_adj_flag = 1;
        }
    }
    prev_iso = buf[0];
}

#endif

#ifdef CONFIG_RESTORE_AFTER_FORMAT

static int keep_ml_after_format = 1;

static void HijackFormatDialogBox()
{
    if (MEM(DIALOG_MnCardFormatBegin) == 0) return;
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    if (dialog && !streq(dialog->type, "DIALOG")) return;

    if (keep_ml_after_format)
        dialog_set_property_str(dialog, 4, "Format card, keep ML " FORMAT_BTN_NAME);
    else
        dialog_set_property_str(dialog, 4, "Format card, remove ML " FORMAT_BTN_NAME);
    dialog_redraw(dialog);
}

static void HijackCurrentDialogBox(int string_id, char* msg)
{
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    if (dialog && !streq(dialog->type, "DIALOG")) return;
    dialog_set_property_str(dialog, string_id, msg);
    dialog_redraw(dialog);
}

int handle_keep_ml_after_format_toggle(struct event * event)
{
    if (event->param == FORMAT_BTN && MENU_MODE && MEM(DIALOG_MnCardFormatBegin))
    {
        keep_ml_after_format = !keep_ml_after_format;
        fake_simple_button(MLEV_HIJACK_FORMAT_DIALOG_BOX);
        return 0;
    }
    
    return 1;
}

/**
 * for testing dialogs and string IDs
 */

static void HijackDialogBox()
{
    struct gui_task * current = gui_task_list.current;
    struct dialog * dialog = current->priv;
    if (dialog && !streq(dialog->type, "DIALOG")) return;
    int i;
    for (i = 0; i<255; i++) {
            char s[30];
            snprintf(s, sizeof(s), "%d", i);
            dialog_set_property_str(dialog, i, s);
    }
    dialog_redraw(dialog);
}

struct tmp_file {
    char name[50];
    void* buf;
    int size;
    int sig;
};

static struct tmp_file * tmp_files = 0;
static int tmp_file_index = 0;
static void* tmp_buffer = 0;
static void* tmp_buffer_ptr = 0;
#define TMP_MAX_BUF_SIZE 15000000

static int TmpMem_Init()
{
    ASSERT(!tmp_buffer);
    ASSERT(!tmp_files);
    static int retries = 0;
    tmp_file_index = 0;
    if (!tmp_files) tmp_files = malloc(200 * sizeof(struct tmp_file));
    if (!tmp_files)
    {
        retries++;
        HijackCurrentDialogBox(4,
            retries > 2 ? "Restart your camera (malloc error)." :
                          "Format: malloc error :("
            );
        beep();
        msleep(2000);
        return 0;
    }

    if (!tmp_buffer) tmp_buffer = (void*)fio_malloc(TMP_MAX_BUF_SIZE);
    if (!tmp_buffer)
    {
        retries++;
        HijackCurrentDialogBox(4,
            retries > 2 ? "Restart your camera (fio_malloc err)." :
                          "Format: fio_malloc error, retrying..."
        );
        beep();
        msleep(2000);
        free(tmp_files); tmp_files = 0;
        return 0;
    }

    retries = 0;
    tmp_buffer_ptr = tmp_buffer;

    return 1;
}

static void TmpMem_Done()
{
    free(tmp_files); tmp_files = 0;
    fio_free(tmp_buffer); tmp_buffer = 0;
}

static void TmpMem_UpdateSizeDisplay(int counting)
{
    int size = tmp_buffer_ptr - tmp_buffer;
    int size_mb = size * 10 / 1024 / 1024;

    char msg[100];
    snprintf(msg, sizeof(msg), "Format       (ML size: %s%d.%d MB%s)", counting ? "> " : "", size_mb/10, size_mb%10, counting ? "..." : "");
    HijackCurrentDialogBox(3, msg);
}

static void TmpMem_AddFile(char* filename)
{
    if (!tmp_buffer) return;
    if (!tmp_buffer_ptr) return;

    int filesize = FIO_GetFileSize_direct(filename);
    if (filesize == -1) return;
    if (tmp_file_index >= 200) return;
    if (tmp_buffer_ptr + filesize + 10 >= tmp_buffer + TMP_MAX_BUF_SIZE) return;
    
    /* don't add the same file twice */
    for (int i = 0; i < tmp_file_index; i++)
        if (streq(tmp_files[i].name, filename))
            return;

    read_file(filename, tmp_buffer_ptr, filesize);
    snprintf(tmp_files[tmp_file_index].name, 50, "%s", filename);
    tmp_files[tmp_file_index].buf = tmp_buffer_ptr;
    tmp_files[tmp_file_index].size = filesize;
    tmp_files[tmp_file_index].sig = compute_signature(tmp_buffer_ptr, filesize/4);
    tmp_file_index++;
    tmp_buffer_ptr += ALIGN32SUP(filesize);

    /* no not update on every file, else it takes too long (90% of time updating display) */
    static int aux = 0;
    if(should_run_polling_action(500, &aux))
    {
        char msg[100];

        snprintf(msg, sizeof(msg), "Reading %s...", filename, tmp_buffer_ptr);
        HijackCurrentDialogBox(4, msg);
        TmpMem_UpdateSizeDisplay(1);
    }
}

static void CopyMLDirectoryToRAM_BeforeFormat(char* dir, int (*is_valid_filename)(char*), int recursive_levels)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( dir, &file );
    if( IS_ERROR(dirent) )
        return;

    do {
        if (file.name[0] == '.' || file.name[0] == '_') continue;
        if (file.mode & ATTR_DIRECTORY)
        {
            if (recursive_levels > 0)
            {
                char new_dir[0x80];
                snprintf(new_dir, sizeof(new_dir), "%s%s/", dir, file.name);
                CopyMLDirectoryToRAM_BeforeFormat(new_dir, is_valid_filename, recursive_levels-1);
            }
            continue; // is a directory
        }
        
        if (is_valid_filename && !is_valid_filename(file.name))
        {
            continue;
        }

        char fn[0x80];
        snprintf(fn, sizeof(fn), "%s%s", dir, file.name);
        TmpMem_AddFile(fn);

    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_FindClose(dirent);
}

static int is_valid_fir_filename(char* filename)
{
    int n = strlen(filename);
    if ((n > 4) && (streq(filename + n - 4, ".FIR") || streq(filename + n - 4, ".fir")))
        return 1;
    return 0;
}

static int is_valid_log_filename(char* filename)
{
    int n = strlen(filename);
    if ((n > 4) && (streq(filename + n - 4, ".LOG") || streq(filename + n - 4, ".log")))
        return 1;
    return 0;
}

static void CopyMLFilesToRAM_BeforeFormat()
{
    /* this is the most important file, read it first */
    TmpMem_AddFile("AUTOEXEC.BIN");
    
    /* some important subdirectories from ML/ */
    CopyMLDirectoryToRAM_BeforeFormat("ML/FONTS/", 0, 0);
    CopyMLDirectoryToRAM_BeforeFormat("ML/MODULES/", 0, 0);
    CopyMLDirectoryToRAM_BeforeFormat("ML/SETTINGS/", 0, 1);

    /* FIR files from root dir */
    CopyMLDirectoryToRAM_BeforeFormat("", is_valid_fir_filename, 0);
    
    /* everything else from ML dir */
    CopyMLDirectoryToRAM_BeforeFormat("ML/", 0, 2);
    
    /* and, if we still have free space, also keep the LOG files from root dir */
    CopyMLDirectoryToRAM_BeforeFormat("", is_valid_log_filename, 0);
    
    /* restore Toshiba FlashAir files, if any */
    /* (normally, formatting this card from camera disables wifi operation) */
    /* (not sure which of those are strictly needed) */
    CopyMLDirectoryToRAM_BeforeFormat("B:/SD_WLAN/", 0, 0);
    CopyMLDirectoryToRAM_BeforeFormat("B:/GUPIXINF/", 0, 1);
    TmpMem_AddFile("B:/DCIM/100__TSB/FA000001.JPG");

    TmpMem_UpdateSizeDisplay(0);
}

// check if autoexec.bin is present on the card
static int check_autoexec()
{
    return is_file("AUTOEXEC.BIN");
}

static void CopyMLFilesBack_AfterFormat()
{
    int i;
    char msg[100];
    int aux = 0;
    for (i = 0; i < tmp_file_index; i++)
    {
        if(should_run_polling_action(500, &aux))
        {
            snprintf(msg, sizeof(msg), "Restoring %s...", tmp_files[i].name);
            HijackCurrentDialogBox(FORMAT_STR_LOC, msg);
        }
        dump_seg(tmp_files[i].buf, tmp_files[i].size, tmp_files[i].name);
        int sig = compute_signature(tmp_files[i].buf, tmp_files[i].size/4);
        if (sig != tmp_files[i].sig)
        {
            snprintf(msg, sizeof(msg), "Could not restore %s :(", tmp_files[i].name);
            HijackCurrentDialogBox(FORMAT_STR_LOC, msg);
            msleep(2000);
            FIO_RemoveFile(tmp_files[i].name);
            if (i <= 1) return;
            //else: if it copies AUTOEXEC.BIN and fonts, ignore the error, it's safe to run
        }
    }

    /* make sure we don't enable bootflag when there is no autoexec.bin (anymore) */
    if(check_autoexec())
    {
        HijackCurrentDialogBox(FORMAT_STR_LOC, "Writing bootflags...");
        
        extern int bootflag_write_bootblock(void);
        if (!bootflag_write_bootblock())
        {
            beep_times(3);
            NotifyBox(5000, "Bootflags not written, use EosCard");
        }
    }

    HijackCurrentDialogBox(FORMAT_STR_LOC, "Magic Lantern restored :)");
    msleep(2000);
}

static void restart_after_format()
{
    /* restart the camera after formatting */
    HijackCurrentDialogBox(FORMAT_STR_LOC, "Restarting camera...");
    msleep(1000);
    
    int reboot = 0;
    prop_request_change(PROP_REBOOT, &reboot, 4);
}

static void HijackFormatDialogBox_main()
{
    if (!MENU_MODE) return;
    if (MEM(DIALOG_MnCardFormatBegin) == 0) return;
    // at this point, Format dialog box is active
    
    #ifdef CONFIG_DUAL_SLOT
    int ml_on_cf = (get_ml_card()->drive_letter[0] == 'A');
    if (ml_on_cf != FORMATTING_CF_CARD)
    {
        /* we are not formatting the ML card, no need to restore anything */
        return;
    }
    #endif

    // make sure we have something to restore :)
    if (!check_autoexec()) return;

    gui_uilock(UILOCK_EVERYTHING);
    
    while (!TmpMem_Init())  /* may fail because of not enough memory */
        msleep(100);

    // before user attempts to do something, copy ML files to RAM
    CopyMLFilesToRAM_BeforeFormat();
    gui_uilock(UILOCK_NONE);

    // all files copied, we can change the message in the format box and let the user know what's going on
    fake_simple_button(MLEV_HIJACK_FORMAT_DIALOG_BOX);

    // waiting to exit the format dialog somehow
    while (MEM(DIALOG_MnCardFormatBegin))
        msleep(200);

    // and maybe to finish formatting the card
    while (MEM(DIALOG_MnCardFormatExecute))
        msleep(50);

    // card was formatted (autoexec no longer there) => restore ML
    if (keep_ml_after_format && !check_autoexec())
    {
        gui_uilock(UILOCK_EVERYTHING);
        CopyMLFilesBack_AfterFormat();
        TmpMem_Done();
        restart_after_format();
        /* needed? */
        gui_uilock(UILOCK_NONE);
    }
    else
    {
        TmpMem_Done();
    }
}
#endif

void debug_menu_init()
{
    #ifdef FEATURE_LV_DISPLAY_PRESETS
    extern struct menu_entry livev_cfg_menus[];
    menu_add( "Prefs", livev_cfg_menus,  1);
    #endif

    crop_factor_menu_init();
    customize_menu_init();
    menu_add( "Debug", debug_menus, COUNT(debug_menus) );
    
    #ifdef FEATURE_SHOW_FREE_MEMORY
    mem_menu_init();
    #endif
    
    movie_tweak_menu_init();
}

static MENU_SELECT_FUNC(gui_events_toggle)
{
    gui_events_show = !gui_events_show;

    if (gui_events_show) {
        console_show();
    } else {
        console_hide();
    }
}

void spy_event(struct event * event)
{
    if (gui_events_show)
    {
        printf("Event param=%8x *obj=%8x/%8x/%8x arg=%8x\n",
            event->param,
            event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj)) : 0,
            event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj + 4)) : 0,
            event->obj ? ((int)event->obj & 0xf0000000 ? (int)event->obj : *(int*)(event->obj + 8)) : 0,
            event->arg
        );
    }
}

#ifdef CONFIG_5DC
static int halfshutter_pressed;
bool get_halfshutter_pressed() { return halfshutter_pressed; }
#else
bool get_halfshutter_pressed() { return HALFSHUTTER_PRESSED && !dofpreview; }
#endif

static int zoom_in_pressed = 0;
static int zoom_out_pressed = 0;
int get_zoom_out_pressed() { return zoom_out_pressed; }

int handle_buttons_being_held(struct event * event)
{
    // keep track of buttons being pressed
    #ifdef CONFIG_5DC
    if (event->param == BGMT_PRESS_HALFSHUTTER) halfshutter_pressed = 1;
    if (event->param == BGMT_UNPRESS_HALFSHUTTER) halfshutter_pressed = 0;
    #endif
    #ifdef BGMT_UNPRESS_ZOOM_IN
    if (event->param == BGMT_PRESS_ZOOM_IN) {zoom_in_pressed = 1; zoom_out_pressed = 0; }
    if (event->param == BGMT_UNPRESS_ZOOM_IN) {zoom_in_pressed = 0; zoom_out_pressed = 0; }
    #endif
    #ifdef BGMT_PRESS_ZOOM_OUT
    if (event->param == BGMT_PRESS_ZOOM_OUT) { zoom_out_pressed = 1; zoom_in_pressed = 0; }
    if (event->param == BGMT_UNPRESS_ZOOM_OUT) { zoom_out_pressed = 0; zoom_in_pressed = 0; }
    #endif
    
    (void)zoom_in_pressed; /* silence warning */

    return 1;
}

void turn_on_display(void)
{
    #if defined(CONFIG_DIGIC_45)
    call("TurnOnDisplay");
    #elif defined(CONFIG_NO_DISPLAY_CALLS)
    extern void _turn_on_display(void);
    _turn_on_display();
    #endif
}

void turn_off_display(void)
{
    #if defined(CONFIG_DIGIC_45)
    call("TurnOffDisplay");
    #elif defined(CONFIG_NO_DISPLAY_CALLS)
    extern void _turn_off_display(void);
    _turn_off_display();
    #endif
}

// those functions seem not to be thread safe
// calling them from gui_main_task seems to sync them with other Canon calls properly
int handle_tricky_canon_calls(struct event *event)
{
    // fake ML events are always negative numbers
    if (event->param >= 0)
        return 1;

    //~ static int k; k++;
    //~ bmp_printf(FONT_LARGE, 50, 50, "[%d] tricky call: %d ", k, event->param); msleep(1000);

    switch (event->param)
    {
        #ifdef CONFIG_RESTORE_AFTER_FORMAT
        case MLEV_HIJACK_FORMAT_DIALOG_BOX:
            HijackFormatDialogBox();
            break;
        #endif
        case MLEV_TURN_ON_DISPLAY:
            if (!DISPLAY_IS_ON)
            {
                turn_on_display();
            }
            break;
        case MLEV_TURN_OFF_DISPLAY:
            if (DISPLAY_IS_ON)
            {
                turn_off_display();
            }
            break;
        /*case MLEV_ChangeHDMIOutputSizeToVGA:
            ChangeHDMIOutputSizeToVGA();
            break;*/
        case MLEV_LCD_SENSOR_START:
            #ifdef CONFIG_LCD_SENSOR
            DispSensorStart();
            #endif
            break;
        case MLEV_REDRAW:
            _redraw_do();   /* todo: move in gui-common.c */
            break;
    }
    
    return 1;
}

// engio functions may fail and lock the camera
void EngDrvOut(uint32_t reg, uint32_t value)
{
    if (ml_shutdown_requested) return;
    if (!(MEM(0xC0400008) & 0x2)) return; // this routine requires LCLK enabled
    _EngDrvOut(reg, value);
}

void engio_write(uint32_t* reg_list)
{
    if (!(MEM(0xC0400008) & 0x2)) return; // this routine requires LCLK enabled
    _engio_write(reg_list);
}
