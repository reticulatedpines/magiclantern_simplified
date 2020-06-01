/** \file
 * Minimal ML - for debugging
 */

#include "dryos.h"
#include "vram.h"
#include "bmp.h"
#include "lens.h"
#include "font_direct.h"
#include "raw.h"

/** These are called when new tasks are created */
static int my_init_task(int a, int b, int c, int d);

/** This just goes into the bss */
#define RELOCSIZE 0x3000 // look in HIJACK macros for the highest address, and subtract ROMBASEADDR
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

extern void _prop_request_change(unsigned property, const void* addr, size_t len);

static void run_test()
{
    /* clear the screen - hopefully nobody will overwrite us */
    clrscr();

    /* make sure we've got some sane exposure settings */
    int iso = ISO_100;
    int shutter = SHUTTER_1_50;
    _prop_request_change(PROP_ISO, &iso, 4);
    _prop_request_change(PROP_SHUTTER, &shutter, 4);

    /* capture a full-res silent picture */
    /* (on real camera, you won't see anything, unless you start in LV PLAY mode */
    void* job = (void*) call("FA_CreateTestImage");
    call("FA_CaptureTestImage", job);
    call("FA_DeleteTestImage", job);

    /* fake a few things, to make the raw backend happy */
    gui_state = GUISTATE_QR;
    pic_quality = PICQ_RAW;
    lens_info.raw_iso = ISO_100;

    if (!raw_update_params())
    {
        font_draw(50,  75, COLOR_RED, 3, "RAW ERROR.");
    }
    else
    {
        raw_preview_fast();
    }
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
    
    /* wait for display to initialize */
    while (!bmp_vram_info[1].vram2)
    {
        msleep(100);
    }

    msleep(1000);

#ifdef CONFIG_QEMU
    /* for running in QEMU: go to PLAY mode and take the test picture from there
     * ideally, we should run from LiveView, but we don't have it emulated.
     * 
     * The emulation usually starts with the main Canon screen,
     * however, some models have it off by default (e.g. 550D),
     * others have sensor cleaning animations (5D2, 50D, 600D),
     * and generally it's hard to draw over this screen without trickery. */
    SetGUIRequestMode(GUIMODE_PLAY);
    msleep(1000);

    /* some cameras don't initialize the YUV buffer right away - but we need it! */
    if (!YUV422_LV_BUFFER_DISPLAY_ADDR)
    {
        /* let's hope this works... */
        extern void * _AllocateMemory(size_t);
        int size = 720 * 480 * 2;
        void * buf = _AllocateMemory(720 * 480 * 2);
        while (!buf);   /* lock up on error */
        memset(buf, 0, size);
        MEM(0xC0F140E0) = YUV422_LV_BUFFER_DISPLAY_ADDR = (uint32_t) buf;
        qprintf("Allocated YUV buffer: %X\n", YUV422_LV_BUFFER_DISPLAY_ADDR);
    }
#else
    /* for running on real camera: wait for user to enter LiveView,
     * then switch to PLAY mode (otherwise you'll capture a dark frame) */
    for (int i = 0; i < 5; i++)
    {
        uint8_t* bmp = bmp_vram_info[1].vram2;
        memset(bmp + 950*40, COLOR_BLACK, 960*70);

        font_draw(50, 50, COLOR_WHITE, 3, "Please enter LiveView,");
        font_draw(50, 75, COLOR_WHITE, 3, "then switch to PLAY mode.");

        msleep(1000);
    }
#endif

    task_create("run_test", 0x1e, 0x4000, run_test, 0 );

    return 0;
}

/* used by font_draw */
void disp_set_pixel(int x, int y, int c)
{
    uint8_t* bmp = bmp_vram_info[1].vram2;
    bmp[x + y * 960] = c;
}

void clrscr()
{
    uint8_t* bmp = bmp_vram_info[1].vram2;
    memset(bmp, 0, 960*480);
}


/* dummy stubs to include raw.c */

int get_ms_clock()
{
    static int ms = 0;
    ms += 10;
    return ms;
}

int is_pure_play_photo_mode() { return 0; }
int is_pure_play_movie_mode() { return 0; }
int is_play_mode() { return 0; }
int digic_zoom_overlay_enabled() { return 0; }
int display_filter_enabled() { return 0; }
void display_filter_get_buffers(uint32_t** a, uint32_t** b) { }
int bmp_printf(uint32_t fontspec, int x, int y, const char *fmt, ... ) { return 0; }
int printf(const char * fmt, ...) { return 0; }
void * bmp_lock;
void bmp_mute_flag_reset() {}
void redraw() {}
void afframe_set_dirty() {}
void ml_assert_handler(char* msg, char* file, int line, const char * func) {}
struct font font_med;
struct lens_info lens_info;
int focus_box_get_raw_crop_offset(int* dx, int* dy) { return 0; }
void edmac_raw_slurp() {}
int should_run_polling_action(int ms, int* last) { return 1; }
int fps_get_current_x1000() { return 30000; }
int wait_lv_frames(int n) { return 0; } 
void EngDrvOut(uint32_t reg, uint32_t val) { MEM(reg) = val; }
void EngDrvOutLV(uint32_t reg, uint32_t val) { };
int get_expsim() { return 0; }
int module_exec_cbr(unsigned int type) { return 0; }
int display_idle() { return 1; }

extern void* _AllocateMemory(size_t size);
extern void  _FreeMemory(void* ptr);

void * __mem_malloc( size_t len, unsigned int flags, const char *file, unsigned int line)
{
    return _AllocateMemory(len);
}
void __mem_free( void * buf)
{
    return _FreeMemory(buf);
}

char* get_current_task_name()
{
    return current_task->name;
}

int raw2iso(int raw_iso)
{
    int iso = (int) roundf(100.0f * powf(2.0f, (raw_iso - 72.0f)/8.0f));
    return iso;
}

const char * format_memory_size(uint64_t size)
{
    static char buf[32];
    snprintf(buf, sizeof(buf), "%d", size);
    return buf;
}
