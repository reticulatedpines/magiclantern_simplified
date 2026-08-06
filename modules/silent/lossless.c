#include "dryos.h"
#include "module.h"
#include "lens.h"
#include "raw.h"
#include "edmac.h"
#include "timer.h"

struct TwoInTwoOutLosslessPath_args
{ 
  uint32_t * ResLockKey;
  uint32_t   ResLockKeySize;
  uint32_t * engio_cmd_1_prepare;
  uint32_t * engio_cmd_2_prepare;
  uint32_t * engio_cmd_pre_jpcore;
  uint32_t * engio_cmd_post_jpcore;
  uint32_t * engio_cmd_stop;
  uint32_t * engio_cmd_6_prepare;
  uint32_t * engio_cmd_7_prepare;
  uint32_t * engio_cmd_8_HIV_PPR;
  uint16_t * PPR_table;
  uint32_t * goes_to_obinteg_C0F0E000_size_0x188;
  uint32_t * goes_to_C0F21000_size_0x7E0;
  uint32_t * LuckyEnable_goes_to_C0F20000_size_0x1000;
  uint16_t   LuckyEnable;
  uint16_t   xRes;
  uint16_t   yRes;
  uint16_t   off_0x3E;
  uint32_t   SamplePrecision;
  uint32_t   off_0x44;
  uint32_t   RD1_Channel;
  uint32_t   RD1_StartFlags;
  uint32_t   RD1_Flags;
  uint32_t   RD1_Connection;
  void *     RD1_Address;
  uint32_t   RD1_Offset;
  uint32_t   RD2_Channel;
  uint32_t   RD2_StartFlags;
  uint32_t   RD2_Flags;
  uint32_t   RD2_Connection;
  void *     RD2_Address;
  uint32_t   RD2_Offset;
  uint32_t   WR1_Channel;
  uint32_t   WR1_StartFlags;
  uint32_t   WR1_Flags;
  uint32_t   WR1_Connection;
  struct memSuite * WR1_MemSuite;
  uint32_t   off_0x8C;
  uint32_t   WR2_Channel;
  uint32_t   WR2_StartFlags;
  uint32_t   WR2_Flags;
  uint32_t   WR2_Connection;
  void *     WR2_Address;
  uint32_t   WR2_Offset;
};

static struct TwoInTwoOutLosslessPath_args TTL_Args;
static struct LockEntry * TTL_ResLock = 0;
static struct LockEntry * TTL_ResLock_shared = 0;   /* DIGIC 4; see lossless_init */
static struct semaphore * lossless_sem = 0;
static int verbose = 0;

static void LosslessCompleteCBR()
{
//    DryosDebugMsg(0, 0, "LosslessCompleteCBR\n");
    give_semaphore(lossless_sem);
}

/* pack two 16-bit values into a 32-bit one */
#define PACK32(lo,hi) (((uint32_t)(lo) & 0xFFFF) | ((uint32_t)(hi) << 16))

static void (*TTL_SetArgs)(int unused, void * TTL_Args, int PictureSize) = NULL;
static int  (*TTL_Prepare)(void * ResLock, void * TTL_Args) = NULL;
static void (*TTL_RegisterCBR)(int id, void * cbr, void * cbr_arg) = NULL;
static void (*TTL_SetFlags)(int PictureType) = NULL;
static void (*TTL_Start)(void * TTL_Args) = NULL;
static void (*TTL_Stop)(void * TTL_Args) = NULL;
static void (*TTL_Finish)(void * ResLock, void * TTL_Args, uint32_t * output_size) = NULL;

/* DIGIC 4 only; both stay NULL on every other model. Measured on a 600D, Canon's
 * ProcessTwoInTwoOutJpegPath differs from the DIGIC 5 one in three ways:
 *
 *  - Start is split in two. TTL_Start configures JPCORE and runs Canon's engio
 *    lists, reprogramming RD1 from the args struct; only TTL_StartJpCore starts
 *    the encoder and the input EDMAC. Our register overrides have to go between
 *    the two, or Canon's full-res values win.
 *
 *  - the pipeline ahead of the encoder holds back input, so it has to be fed past
 *    the end of the frame to push the last real rows through (D4_GEO_PIPELINE_ROWS).
 *
 *  - Completion is unreliable: JPCORE's 0x400 interrupt often never fires. Keep a
 *    stall detector as the completion path: when the output pointer stops moving
 *    with an EOI at the tip, call the completion CBR ourselves. It pops the output
 *    EDMAC FIFO (flushing EOI) and that is what dispatches the CBR we registered
 *    as id 4. */
static void (*TTL_StartJpCore)(void * TTL_Args) = NULL;
static void (*TTL_JpCoreCompleteCBR)(void * TTL_Args, int size, int, int) = NULL;

/* Read-only half of LockEngineResources: walks our resource ids and returns 0 if
 * all are free, 0x1f if any is held. TTL_Prepare goes on to TakeSemaphore(sem, 0),
 * which on DryOS means no timeout, so contention otherwise parks the task forever. */
static int (*TTL_ResourcesBusy)(void * ResLock) = NULL;

#define TTL_IS_DIGIC4       (TTL_StartJpCore != NULL)

#define TTL_D4_TICK_US      500     /* how often the completion detector looks */
#define TTL_D4_EOI_QUIET_US 500     /* unmoved this long with EOI = finished */
#define TTL_D4_EOI_WINDOW   64

/* DIGIC 4 PACK16/SNDPAS geometry. Measured on 600D LiveView Mem1->JPCORE:
 * SNDPAS advances width+20 samples per row regardless of the pitch register, so
 * RD1 must feed width+20 to keep rows aligned. That needs an even EDMAC row
 * length (width+20)*14/8, which holds only for width = 4 (mod 8). The encoder
 * also holds ~21 rows in the pipeline; 24 surplus RD1 rows flush them. */
#define D4_GEO_PIPELINE_ROWS      24
#define D4_GEO_PITCH_PAD          20
#define D4_GEO_MAX_OVERREAD_ROWS  64

struct d4_geometry
{
    int width;
    int height;
    int pitch;              /* SNDPAS row advance = width + pad */
    int feed_delta;         /* RD1 supplies width+feed_delta samples per row */
    int flush_rows;
};

static void d4_geometry_for_width(struct d4_geometry *out, int width, int height, int src_width)
{
    int pitch = width + D4_GEO_PITCH_PAD;
    int feed = width + D4_GEO_PITCH_PAD;

    /* Odd EDMAC row length is refused; over-long feed drives off1b negative. Fall
     * back to the stock read: the frame shears, but it encodes. */
    if (width <= 0 || (width & 7) != 4 || feed > src_width)
    {
        feed = width;
    }

    out->width = width;
    out->height = height;
    out->pitch = pitch;
    out->feed_delta = feed - width;
    out->flush_rows = D4_GEO_PIPELINE_ROWS;
}

/* Scan the last `window` bytes for FF D9. Entropy-coded data cannot contain a
 * bare FF (stuffed as FF 00), so this marker only appears where the frame ends. */
static unsigned int d4_lj92_eoi_end(const unsigned char *buf, unsigned int size, unsigned int window)
{
    unsigned int start, i;

    if (!buf || size < 2)
        return 0;
    if (window > size)
        window = size;

    start = size - window;
    for (i = size; i >= start + 2; i--)
    {
        if (buf[i - 2] == 0xFF && buf[i - 1] == 0xD9)
            return i;
    }
    return 0;
}

static int d4_lj92_has_eoi(const unsigned char *buf, unsigned int size, unsigned int window)
{
    return d4_lj92_eoi_end(buf, size, window) != 0;
}

int lossless_input_overread_rows(void)
{
    return TTL_IS_DIGIC4 ? D4_GEO_MAX_OVERREAD_ROWS : 0;
}

int lossless_encodable_width(int width)
{
    int w;

    if (!TTL_IS_DIGIC4 || width <= 0)
        return width;

    /* Nearest width = 4 (mod 8) at or below `width`, never below 4. */
    w = width - ((width - 4) & 7);
    if (w < 4)
        w += 8;
    return w;
}

static int TTL_wr1_pos(void)
{
    return edmac_get_pointer(TTL_Args.WR1_Channel)
         - edmac_get_address(TTL_Args.WR1_Channel);
}

/* One-shot timer chain (~30 ticks/frame). Each encode gets a generation token a
 * tick must still match to re-arm, so late timers from a previous frame cannot
 * signal the next one. A plain armed flag is not enough: clearing and re-setting
 * it inside one tick period lets the previous chain keep going. */
static int TTL_stall_gen = 0;
static int TTL_stall_pos = 0;
static uint32_t TTL_stall_time = 0;
static const unsigned char * TTL_stall_out = 0;

static void TTL_stall_cbr(int last_expiry, void * priv)
{
    if ((int)(intptr_t) priv != TTL_stall_gen)
        return;

    uint32_t now = get_us_clock();
    int pos = TTL_wr1_pos();

    if (pos != TTL_stall_pos)
    {
        TTL_stall_pos = pos;
        TTL_stall_time = now;
    }
    else if (pos && now - TTL_stall_time >= TTL_D4_EOI_QUIET_US
             && d4_lj92_eoi_end(TTL_stall_out, pos, TTL_D4_EOI_WINDOW))
    {
        TTL_JpCoreCompleteCBR(&TTL_Args, pos, 0, 0);
        return;
    }

    /* Quiet without EOI means starved, not done. Bound hangs with lossless_sem. */
    SetHPTimerNextTick(last_expiry, TTL_D4_TICK_US, TTL_stall_cbr, TTL_stall_cbr, priv);
}

/* edmac-memcpy.c */
extern uint32_t edmac_write_chan;
extern uint32_t edmac_read_chan;

static void decompress_init();

int lossless_init()
{
    if (is_camera("5D3", "1.1.3"))
    {
        /* ProcessTwoInTwoOutLosslessPath, 5D3 1.1.3 */
        TTL_SetArgs     = (void *) 0xFF32330C;  /* fills TTL_Args struct; PictureSize(Mem1ToRaw) */
        TTL_Prepare     = (void *) 0xFF3D4680;  /* called right after ProcessTwoInTwoOutLosslessPath(R) Start; */
                                                /* calls [TTL] GetPathResources and sets up the encoder for RAW/SRAW/MRAW */
        TTL_RegisterCBR = (void *) 0xFF3D3774;  /* RegisterTwoInTwoOutLosslessPathCompleteCBR */
        TTL_SetFlags    = (void *) 0xFF32B418;  /* called next, with PictureType as arguments */
        TTL_Start       = (void *) 0xFF3D46F0;  /* called next; starts the EDmac transfers */
        TTL_Stop        = (void *) 0xFF3D4728;  /* called right after sssStopMem1ToRawPath */
        TTL_Finish      = (void *) 0xFF3D4760;  /* called next; calls UnlockEngineResources and returns output size from JpCoreCompleteCBR */
    }

    if (is_camera("5D3", "1.2.3"))
    {
        /* ProcessTwoInTwoOutLosslessPath, 5D3 1.2.3 */
        TTL_SetArgs     = (void *) 0xFF327DE8;  /* fills TTL_Args struct; PictureSize(Mem1ToRaw) */
        TTL_Prepare     = (void *) 0xFF3DD574;  /* called right after ProcessTwoInTwoOutLosslessPath(R) Start; */
                                                /* calls [TTL] GetPathResources and sets up the encoder for RAW/SRAW/MRAW */
        TTL_RegisterCBR = (void *) 0xFF3DC668;  /* RegisterTwoInTwoOutLosslessPathCompleteCBR */
        TTL_SetFlags    = (void *) 0xFF32FF2C;  /* called next, with PictureType as arguments */
        TTL_Start       = (void *) 0xFF3DD5E4;  /* called next; starts the EDmac transfers */
        TTL_Stop        = (void *) 0xFF3DD61C;  /* called right after sssStopMem1ToRawPath */
        TTL_Finish      = (void *) 0xFF3DD654;  /* called next; calls UnlockEngineResources and returns output size from JpCoreCompleteCBR */
    }

    if (is_camera("700D", "1.1.5"))
    {
        /* ProcessTwoInTwoOutJpegath, 700D 1.1.5 */
        TTL_SetArgs     = (void *) 0xFF35F510;  /* fills TTJ_Args struct; PictureSize(Mem1ToRaw) */
        TTL_Prepare     = (void *) 0xFF424BA4;  /* called right after ProcessTwoInTwoOutJpegath(R) Start(%d); */
                                                /* calls [TTJ] GetPathResources and sets up the encoder for RAW */
        TTL_RegisterCBR = (void *) 0xFF423B88;  /* RegisterTwoInTwoOutJpegPathCompleteCBR */
        TTL_SetFlags    = (void *) 0xFF36B2D8;  /* alternate StartTwoInTwoOutJpegPath http://www.magiclantern.fm/forum/index.php?topic=18443.msg188721#msg188721 */
        TTL_Start       = (void *) 0xFF424C4C;  /* called next; starts the EDmac transfers */
        TTL_Stop        = (void *) 0xFF423DD4;  /* called right after sssStopMem1ToRawPath */
        TTL_Finish      = (void *) 0xFF424CBC;  /* called next; calls UnlockEngineResources and returns output size from JpCoreCompleteCBR */
    }

    if (is_camera("650D", "1.0.4"))
    {
        /* ProcessTwoInTwoOutJpegath, 650D 1.0.4 */
        TTL_SetArgs     = (void *) 0xFF35C9C0;  /* fills TTJ_Args struct; PictureSize(Mem1ToRaw) */
        TTL_Prepare     = (void *) 0xFF4210BC;  /* called right after ProcessTwoInTwoOutJpegath(R) Start(%d); */
                                                /* calls [TTJ] GetPathResources and sets up the encoder for RAW */
        TTL_RegisterCBR = (void *) 0xFF4200A0;  /* RegisterTwoInTwoOutJpegPathCompleteCBR */
        TTL_SetFlags    = (void *) 0xFF368788;  /* alternate StartTwoInTwoOutJpegPath http://www.magiclantern.fm/forum/index.php?topic=18443.msg188721#msg188721 */
        TTL_Start       = (void *) 0xFF421164;  /* called next; starts the EDmac transfers */
        TTL_Stop        = (void *) 0xFF4202EC;  /* called right after sssStopMem1ToRawPath */
        TTL_Finish      = (void *) 0xFF4211D4;  /* called next; calls UnlockEngineResources and returns output size from JpCoreCompleteCBR */
    }

    if (is_camera("EOSM", "2.0.2"))
    {
        /* ProcessTwoInTwoOutJpegath, EOSM 2.0.2 */
        TTL_SetArgs     = (void *) 0xFF361498;  /* fills TTJ_Args struct; PictureSize(Mem1ToRaw) */
        TTL_Prepare     = (void *) 0xFF429210;  /* called right after ProcessTwoInTwoOutJpegath(R) Start(%d); */
                                                /* calls [TTJ] GetPathResources and sets up the encoder for RAW */
        TTL_RegisterCBR = (void *) 0xFF4281F4;  /* RegisterTwoInTwoOutJpegPathCompleteCBR */
        TTL_SetFlags    = (void *) 0xFF36D124;  /* called next, with PictureType as arguments */
        TTL_Start       = (void *) 0xFF4292B8;  /* called next; starts the EDmac transfers */
        TTL_Stop        = (void *) 0xFF428440;  /* called right after sssStopMem1ToRawPath */
        TTL_Finish      = (void *) 0xFF429328;  /* called next; calls UnlockEngineResources and returns output size from JpCoreCompleteCBR */
    }
    if (is_camera("100D", "1.0.1"))
    {
        /* ProcessTwoInTwoOutJpegath, 100D 1.0.1 */
        TTL_SetArgs     = (void *) 0xFF3647D0;  /* fills TTJ_Args struct; PictureSize(Mem1ToRaw) */
        TTL_Prepare     = (void *) 0xFF42Bf8C;  /* called right after ProcessTwoInTwoOutJpegath(R) Start(%d); */
                                                /* calls [TTJ] GetPathResources and sets up the encoder for RAW */
        TTL_RegisterCBR = (void *) 0xFF42AF70;  /* RegisterTwoInTwoOutJpegPathCompleteCBR */
        TTL_SetFlags    = (void *) 0xFF363148;  /* called next, with PictureType as arguments */ 
        TTL_Start       = (void *) 0xFF42c034;  /* called next; starts the EDmac transfers */
        TTL_Stop        = (void *) 0xFF42B1BC;  /* called right after sssStopMem1ToRawPath */
        TTL_Finish      = (void *) 0xFF42C0A4;  /* called next; calls UnlockEngineResources and returns output size from JpCoreCompleteCBR */
    }

    if (is_camera("6D", "1.1.6"))
    {
        /* ProcessTwoInTwoOutLosslessPath, 6D 1.1.6 */
        TTL_SetArgs     = (void *) 0xFF3491C8;  /* fills TTL_Args struct; PictureSize(Mem1ToRaw) */
        TTL_Prepare     = (void *) 0xFF4129BC;  /* called right after ProcessTwoInTwoOutLosslessPath(R) Start; */
                                                /* calls [TTL] GetPathResources and sets up the encoder for RAW/SRAW/MRAW */
        TTL_RegisterCBR = (void *) 0xFF411A44;  /* RegisterTwoInTwoOutLosslessPathCompleteCBR */
        TTL_SetFlags    = (void *) 0xFF359C78;  /* called next, with PictureType as arguments */
        TTL_Start       = (void *) 0xFF412A2C;  /* called next; starts the EDmac transfers */
        TTL_Stop        = (void *) 0xFF412A64;  /* called right after sssStopMem1ToRawPath */
        TTL_Finish      = (void *) 0xFF412A9C;  /* called next; calls UnlockEngineResources and returns output size from JpCoreCompleteCBR */
    }

    if (is_camera("70D", "1.1.2"))
    {
        /* ProcessTwoInTwoOutLosslessPath, 70D 1.1.2 */
        TTL_SetArgs     = (void *) 0xFF362174;  /* fills TTL_Args struct; PictureSize(Mem1ToRaw) */
        TTL_Prepare     = (void *) 0xFF438404;  /* called right after ProcessTwoInTwoOutLosslessPath(R) Start; */
                                                /* calls [TTL] GetPathResources and sets up the encoder for RAW/SRAW/MRAW */
        TTL_RegisterCBR = (void *) 0xFF43748C;  /* RegisterTwoInTwoOutLosslessPathCompleteCBR */
        TTL_SetFlags    = (void *) 0xFF372AE8;  /* called next, with PictureType as arguments */
        TTL_Start       = (void *) 0xFF438474;  /* called next; starts the EDmac transfers */
        TTL_Stop        = (void *) 0xFF4384AC;  /* called right after sssStopMem1ToRawPath */
        TTL_Finish      = (void *) 0xFF4384E4;  /* called next; calls UnlockEngineResources and returns output size from JpCoreCompleteCBR */
    }

    if (is_camera("600D", "1.0.2"))
    {
        /* ProcessTwoInTwoOutJpegPath, 600D 1.0.2 (DIGIC 4)
         *
         * Same "TTJ" path as 650D/700D/EOSM/100D, and its JPCORE does support
         * lossless: with SamplePrecision 0xE the path calls
         * JPCORE_SetEncodeLosslessType(1). Canon's own caller is
         * ProcessTwoInTwoOutJpegPath at 0xFF2FE0DC; sdfExecuteMem1ToRawPath
         * (0xFF094CB4) drives it.
         */
        TTL_SetArgs     = (void *) 0xFF2155E0;  /* SetTwoInTwoOutJpegPathArgs; PictureSize 0 selects RAW */
        TTL_Prepare     = (void *) 0xFF2FE090;  /* wraps LockEngineResources; unlike D5 it does not set up the encoder */
        TTL_RegisterCBR = (void *) 0xFF2FD424;  /* RegisterTwoInTwoOutJpegPathCompleteCBR; ids 0, 1, 4, 5 */
        TTL_SetFlags    = NULL;                 /* no equivalent; the picture type comes from SamplePrecision */
        TTL_Start       = (void *) 0xFF2FDA5C;  /* configures JPCORE + EDMACs and runs the engio lists */
        TTL_StartJpCore = (void *) 0xFF2FD784;  /* starts the encoder, then the input EDMAC */
        TTL_Stop        = (void *) 0xFF2FD670;
        TTL_Finish      = (void *) 0xFF2FD518;  /* UnLockEngineResources; the extra args are ignored */
        TTL_JpCoreCompleteCBR = (void *) 0xFF2FD9C8;
        TTL_ResourcesBusy = (void *) 0xFF1E4D40;
    }

    lossless_sem = create_named_semaphore(NULL, SEM_CREATE_LOCKED);

    if (is_camera("600D", "*"))
    {
        /* Canon's own list for this path, from ROM at 0xFF5FFE34 (17 entries).
         * Write EDMAC index 4 (WR2) is left out - we never use it. */
        uint32_t resources[] = {
            0x00000 | edmac_channel_to_index(edmac_write_chan),
            0x10000 | edmac_channel_to_index(edmac_read_chan),
            0x30000,    /* Read connection 0 (uncompressed input) */
            0x20005,    /* Write connection 5 (compressed output) */
            0x20016,    /* Write connection 22 (for WR2 - not used) */
            0x50003,
            0x5000d,
            0x5000f,
            0x5001a,
            0x80000,
            0x90000,
            0xa0000,
            0x160000,
            0x130003,
            0x130004,
            0x130005,
        };

        /* Same list without the five entries LiveView keeps. Canon can name the
         * full set because it only runs this path with the mirror down; we cannot,
         * because LockEngineResources has no timeout. Leaving out 0x5000d/0f/1a and
         * 0x130004 costs no clock - sibling entries keep those classes powered.
         * 0x80000 is the only class-8 entry, so this list depends on the display
         * holding class 8 up, which is exactly when it gets used. */
        uint32_t resources_shared[] = {
            0x00000 | edmac_channel_to_index(edmac_write_chan),
            0x10000 | edmac_channel_to_index(edmac_read_chan),
            0x30000, 0x20005, 0x20016,
            0x50003, 0x90000, 0xa0000, 0x160000,
            0x130003, 0x130005,
        };

        TTL_ResLock = CreateResLockEntry(resources, COUNT(resources));
        TTL_ResLock_shared = CreateResLockEntry(resources_shared, COUNT(resources_shared));
    }
    else if (is_camera("700D", "*") || is_camera("650D", "*") || is_camera("EOSM", "*") || is_camera("100D", "*"))
    {
        uint32_t resources[] = {
            0x00000 | edmac_channel_to_index(edmac_write_chan),
            0x10000 | edmac_channel_to_index(edmac_read_chan),
            0x20005,
            0x20016,
            0x30002,
            0x50034,
            0x5002d,
            0x50010,
            0x90001,
            0x90000,
            0xa0000,
            0x160000,
            0x260000,
            0x260001,
            0x260002,
            0x260003,
        };

        TTL_ResLock = CreateResLockEntry(resources, COUNT(resources));
    }
    else if (is_camera("5D3", "*") || is_camera("6D", "*"))
    {
        uint32_t resources[] = {
            0x00000 | edmac_channel_to_index(edmac_write_chan),
            0x10000 | edmac_channel_to_index(edmac_read_chan),
            0x30001,    /* Read connection 1 (uncompressed input) */
            0x2002d,    /* Write connection 45 (compressed output) */
          //0x20016,    /* Write connection 22 (for WR2 - not used) */
            0x50034,
            0x5002d,
            0x50010,
            0x90001,
            0x230000,
            0x160000,
            0x260000,
            0x260001,
            0x260002,
            0x260003,
        };
        
        TTL_ResLock = CreateResLockEntry(resources, COUNT(resources));
    }
    else if (is_camera("70D", "*"))
    {
        uint32_t resources[] = {
            0x00000 | edmac_channel_to_index(edmac_write_chan),
            0x10000 | edmac_channel_to_index(edmac_read_chan),
            0x30001,    /* Read connection 1 (uncompressed input) */
            0x2002d,    /* Write connection 45 (compressed output) */
          //0x20016,    /* Write connection 22 (for WR2 - not used) */
            0x50034,
          //0x5002d,    /* workaround, TTL_Prepare stucks otherwise if called in LV */
            0x50010,
            0x90001,
            0x230000,
            0x160000,
            0x260000,
            0x260001,
            0x260002,
            0x260003,
        };
        
        TTL_ResLock = CreateResLockEntry(resources, COUNT(resources));
    }
    
    /* optionally initialize the decompression routines */
    decompress_init();

    /* return 1 if everything looks alright */
    return TTL_Start && lossless_sem && TTL_ResLock;
}

static uint32_t start_time = 0;

/* PACK16 (C0F085xx) and SNDPAS (C0F0D0xx) registers LiveView owns while the
 * display is up. Canon's RAW prepare list overwrites them; the stop list only
 * soft-resets the block bases and never puts these back. Save from the engio
 * shadow before Start, EngDrvOut them back after Stop while the resource lock
 * still holds the clocks. DIGIC 4 only. */
static const uint32_t TTL_d4_lv_regs[] = {
    0xC0F08544, 0xC0F08548, 0xC0F08550, 0xC0F08554, 0xC0F08558,
    0xC0F08560, 0xC0F0856C, 0xC0F08570,
    0xC0F0D008, 0xC0F0D014, 0xC0F0D018, 0xC0F0D038,
};
static uint32_t TTL_d4_lv_saved[COUNT(TTL_d4_lv_regs)];

static void TTL_d4_lv_save(void)
{
    for (int i = 0; i < COUNT(TTL_d4_lv_regs); i++)
        TTL_d4_lv_saved[i] = shamem_read(TTL_d4_lv_regs[i]);
}

static void TTL_d4_lv_restore(void)
{
    for (int i = 0; i < COUNT(TTL_d4_lv_regs); i++)
        EngDrvOut(TTL_d4_lv_regs[i], TTL_d4_lv_saved[i]);
}

/* ROM prepare list is ~97 pairs; clone enough for that plus the terminator. */
#define TTL_D4_PREPARE_MAX_WORDS  256
static uint32_t TTL_d4_prepare_clone[TTL_D4_PREPARE_MAX_WORDS];
static uint32_t * TTL_d4_prepare_orig;

static int TTL_d4_patch_prepare_list(const struct d4_geometry * geo)
{
    uint32_t * src = TTL_Args.engio_cmd_1_prepare;
    int i = 0;

    if (!src || TTL_d4_prepare_orig)
        return 0;

    while (i < TTL_D4_PREPARE_MAX_WORDS - 2 && src[i] != 0xFFFFFFFFu)
    {
        if ((src[i] & 0xFFF00000) != 0xC0F00000)
            return 0;

        TTL_d4_prepare_clone[i] = src[i];
        TTL_d4_prepare_clone[i + 1] = src[i + 1];

        switch (src[i])
        {
            case 0xC0F0858C:
                TTL_d4_prepare_clone[i + 1] = (unsigned int) geo->width;
                break;
            case 0xC0F08594:
                TTL_d4_prepare_clone[i + 1] = (unsigned int) (geo->pitch - 2);
                break;
            case 0xC0F0D01C:
            case 0xC0F0D024:
                TTL_d4_prepare_clone[i + 1] = (unsigned int) geo->pitch;
                break;
            case 0xC0F0D020:
                TTL_d4_prepare_clone[i + 1] = (unsigned int) (geo->pitch + 3);
                break;
            default:
                break;
        }

        i += 2;
    }

    if (i >= TTL_D4_PREPARE_MAX_WORDS - 1)
        return 0;

    TTL_d4_prepare_clone[i] = 0xFFFFFFFFu;
    TTL_d4_prepare_orig = src;
    TTL_Args.engio_cmd_1_prepare = TTL_d4_prepare_clone;
    return 1;
}

static void TTL_d4_restore_prepare_list(void)
{
    if (TTL_d4_prepare_orig)
    {
        TTL_Args.engio_cmd_1_prepare = TTL_d4_prepare_orig;
        TTL_d4_prepare_orig = 0;
    }
}

static void TTL_d4_apply_geometry(const struct d4_geometry * geo)
{
    EngDrvOut(0xC0F0858C, (unsigned int) geo->width);
    EngDrvOut(0xC0F08594, (unsigned int) (geo->pitch - 2));
    EngDrvOut(0xC0F0D01C, (unsigned int) geo->pitch);
    EngDrvOut(0xC0F0D020, (unsigned int) (geo->pitch + 3));
    EngDrvOut(0xC0F0D024, (unsigned int) geo->pitch);
}

/* OBWB per-Bayer gain. Unity is 0x400 (Q10). The field is 11 bits; values past
 * 0x7FF alias to gain 0 and collapse the frame to the pedestal. The encoder
 * input stage subtracts the optical black, applies this gain and re-pedestals
 * at 512 - that is also how reduced bit depths are reached (attenuate here
 * instead of via SHAD_GAIN, which never reaches the DIGIC 4 raw slurp). */
#define TTL_D4_OBWB_UNITY     0x400
#define TTL_D4_OBWB_PEDESTAL  512

static const uint32_t TTL_d4_obwb_gain_regs[] = {
    0xC0F0E084,     /* phase (0,0) */
    0xC0F0E074,     /* phase (0,1) */
    0xC0F0E064,     /* phase (1,0) */
    0xC0F0E054,     /* phase (1,1) */
};

static int TTL_d4_target_bpp = 14;

void lossless_d4_set_target_bpp(int bpp)
{
    TTL_d4_target_bpp = COERCE(bpp, 8, 14);
}

static uint32_t TTL_d4_obwb_gain(void)
{
    return TTL_D4_OBWB_UNITY >> (14 - TTL_d4_target_bpp);
}

int lossless_d4_map_level(int level, int black_in)
{
    int mapped;

    if (!TTL_IS_DIGIC4)
        return level;

    mapped = (level - black_in) * (int) TTL_d4_obwb_gain()
             / TTL_D4_OBWB_UNITY + TTL_D4_OBWB_PEDESTAL;
    return COERCE(mapped, 0, 16383);
}

static void TTL_d4_obwb_apply(void)
{
    uint32_t value = TTL_d4_obwb_gain();
    int i;

    for (i = 0; i < (int) COUNT(TTL_d4_obwb_gain_regs); i++)
        EngDrvOut(TTL_d4_obwb_gain_regs[i], value);
}

/* returns output size if successful, negative on error:
 * -1 not initialised, -2 encoder timeout, -3 output is not a JPEG,
 * -4 engine resources busy, -5 Prepare failed,
 * -7 positive size without a terminal EOI (incomplete LJ92) */
static int lossless_compress_raw_rectangle_once(
    struct memSuite * dst_suite, void * src,
    int src_width, int src_x, int src_y,
    int width, int height
)
{
    struct d4_geometry geo;

    if (!TTL_ResLock || !lossless_sem || !TTL_Start)
        return -1;

    d4_geometry_for_width(&geo, width, height, src_width);
    width = geo.width;

    /* setup photo quality (valid values: 0=RAW, 1=MRAW, 2=SRAW, 14, 15) */
    TTL_SetArgs(0, &TTL_Args, 0);

    /* trick the encoder so it configures slice width = image width */
    /* we'll have two slices on top of each other; this will give
     * valid lossless DNG as well, if we prepend a header :)
     */
    TTL_Args.xRes = width;
    TTL_Args.yRes = height;

    /* Default EDMAC channels may be used in LiveView;
     * use the ones from edmac_memcpy instead */
    TTL_Args.RD1_Channel = edmac_read_chan;
    TTL_Args.WR1_Channel = edmac_write_chan;

    /* set starting point (top-left corner) */
    /* we need to skip a multiple of 8 pixels horizontally for raw_pixblock alignment
     * and an even number of pixels vertically, to preserve the Bayer pattern
     */
    uint32_t src_adjusted = ((uint32_t)src & 0x1FFFFFFF)
        + ((src_x/8*8) + (src_y/2*2) * src_width) * 14/8;

    /* set buffers */
    TTL_Args.WR1_MemSuite = dst_suite;
    TTL_Args.WR2_Address  = 0;
    TTL_Args.RD1_Address  = (void *) src_adjusted;

    /* to check whether the compression was successful */
    /* note: in dummy mode, dst_suite is NULL - don't check this case */
    /* Every use of WR1_Address is a CPU access to a buffer the WR1 EDMAC writes.
     * MLV Lite's slots come off the memory suite without UNCACHEABLE(), so take
     * the uncacheable alias here - otherwise a dirty cache line can be written
     * back over the finished JPEG, and the SOI/EOI checks can read stale data. */
    void * WR1_Address = (TTL_Args.WR1_MemSuite)
        ? UNCACHEABLE(GetMemoryAddressOfMemoryChunk(GetFirstChunkFromSuite(TTL_Args.WR1_MemSuite)))
        : 0;

    if (WR1_Address)
    {
        MEM(WR1_Address) = 0;
    }

    /* Prefer Canon's full resource list (locking also powers the blocks up);
     * fall back to the LiveView-safe shared list when the full one is held.
     * Short same-frame retries absorb transient LV ownership. */
    struct LockEntry * lock = TTL_ResLock;
    int lock_retries = 0;

    if (TTL_ResourcesBusy)
    {
        const int max_retries = TTL_IS_DIGIC4 ? 8 : 0;
        while (1)
        {
            lock = TTL_ResLock;
            if (!TTL_ResourcesBusy(lock))
                break;

            lock = TTL_ResLock_shared;
            if (lock && !TTL_ResourcesBusy(lock))
                break;

            if (lock_retries >= max_retries)
                return -4;

            lock_retries++;
            msleep(1);
        }
    }

    /* Patch Canon's prepare list before Start latches companions that post-Start
     * EngDrvOut cannot move. Reinforced after Start by TTL_d4_apply_geometry. */
    if (TTL_IS_DIGIC4)
        TTL_d4_patch_prepare_list(&geo);

    int prepared = TTL_Prepare(lock, &TTL_Args);

    /* On DIGIC 4 the status is LockEngineResources' own; odd means the engine
     * clocks were never switched on. Nothing to Finish either - Canon does not
     * call it on this path. */
    if (TTL_IS_DIGIC4 && (prepared & 1))
    {
        TTL_d4_restore_prepare_list();
        return -5;
    }

    if (TTL_IS_DIGIC4)
    {
        TTL_d4_lv_save();
        TTL_Start(&TTL_Args);
        TTL_d4_restore_prepare_list();
        TTL_d4_apply_geometry(&geo);
        TTL_d4_obwb_apply();
    }

    if (is_camera("6D", "*") ||
        is_camera("650D", "*"))
    {
        /* not sure what exactly these do, but they seem to be required to get correct image
         * taken from register log diffs: http://www.magiclantern.fm/forum/index.php?topic=18443.msg197987#msg197987 */
        EngDrvOut(0xC0F37610, 0);       /* 0x11 on 650D, 0 on all other D5 (not sure if needed) */
        EngDrvOut(0xC0F37628, 0x71000); /* 72000 on 650D, 71000 on all other D5 */
        EngDrvOut(0xC0F3762C, 0x71000); /* 72000 on 650D, 71000 on all other D5 */
        EngDrvOut(0xC0F37630, 0x71000); /* 72000 on 650D, 71000 on all other D5 */
        EngDrvOut(0xC0F37634, 0x71000); /* 72000 on 6D and 650D, 71000 on all other D5 */
        EngDrvOut(0xC0F3763C, 0);       /* 0x1000000 on 6D and 650D, 0 on all other D5 */
        EngDrvOut(0xC0F37640, 0);       /* 0x2000000 on 6D and 650D, 0 on all other D5 */
        EngDrvOut(0xC0F37644, 0);       /* 0x4000000 on 6D and 650D, 0 on all other D5 */
        EngDrvOut(0xC0F37648, 0);       /* 0x8000000 on 6D and 650D, 0 on all other D5 */
    }

    if (is_camera("70D", "*"))
    {
        /* 70D is different */
        // EngDrvOut(0xC0F373F4, 0x00000000);  /* alternative fixing method; 0x7FFF7FFF on 70D */
        EngDrvOut(0xC0F373B4, 0);              /* 0x11 on 70D */
        EngDrvOut(0xC0F37300, PACK32(width    - 1,  height/2  - 1));  /* 0xE7B0ADF on 70D */
        EngDrvOut(0xC0F373E8, PACK32(width    - 1,  height/2  - 1));  /* 0xE7B0ADF on 70D */
    }
    else if (TTL_IS_DIGIC4)
    {
        /* no 0xC0F375B4 on this generation; the slice geometry lives in 0xC0F120xx.
         * Canon's RAW list (0xFF5FFE78 on the 600D) writes 3 slices of 1728x3456 for the
         * 5184x3456 sensor frame; transpose that the way the DIGIC 5 branch does and leave
         * the slice count alone. */
        EngDrvOut(0xC0F12010, PACK32(width - 1,  height/3  - 1));
    }
    else if (is_camera("DIGIC", "5"))
    {
        /* all other D5 models use this register instead */
        /* the hardware encoder (and other image processing modules that might be used)
         * expect slice width and slice height for these registers
         * Canon configuration: slice width = image width / 2, slice height = image height
         * our configuration:   slice width = image width, slice height = image height / 2 */
        EngDrvOut(0xC0F375B4, PACK32(width    - 1,  height/2  - 1));  /* 0xF6D0B8F on 5D3 */
    }

    if (is_camera("DIGIC", "5") || TTL_IS_DIGIC4)
    {
        /* required for full-res silent pictures? */
        /* this one doesn't seem to refer to slice size, but to total image size */
        /* sht_mirror shows 0xC0F13000 - 0xC0F13064 as RABBIT */
        EngDrvOut(0xC0F13068, PACK32(width    - 1,  height    - 1));  /* 0xF6D171F on 5D3 */
    }

    /* there are a few registers possibly related to RD2/WR2 - we don't use them */
    /* 5D3: [0xC0F375B4] <- 0xF6D0B8F, [0xC0F12020] <- 0x18A0127  =>  slice width / 10, slice height / 10 */
    /* 6D:  [0xC0F375B4] <- 0xE7B0ADF, [0xC0F12020] <- 0x13400E7  =>  slice width / 12, slice height / 12 */
    /* 70D: [0xC0F37300] <- 0xE7B0ADF, [0xC0F12020] <- 0x13400E7  =>  slice width / 12, slice height / 12 */
    /* 650D, 700D, 100D, EOSM, EOSM2:
     *      [0xC0F375B4] <- 0xDC70A4F, [0xC0F12020] <- 0x1B80149  =>  slice width / 8, slice height / 8 */
    if (0)
    {
        /* values for 5D3 - just for future reference */
        EngDrvOut(0xC0F12010,        width    - 1                 );  /* 0xB8F     */
        EngDrvOut(0xC0F12014, PACK32(width    - 1,  height/2  - 1));  /* 0xF6D0B8F */
        EngDrvOut(0xC0F1201C,        width/10 - 1                 );  /* 0x127     */
        EngDrvOut(0xC0F12020, PACK32(width/10 - 1,  height/20 - 1));  /* 0x18A0127 */
    }

    /* need to read the image data in 2 slices
     * default configuration is 2 vertical slices;
     * however, using 2 horizontal slices makes it easy
     * to just slap a DNG header, resulting in valid output.
     * 
     * => the input EDMAC will simply read the image as usual.
     */
    int feed_width = width + geo.feed_delta;

    struct edmac_info RD1_info = {
        .xb     = feed_width * 14/8,
        .yb     = height + (TTL_IS_DIGIC4 ? geo.flush_rows : 0) - 1,
        .off1b  = src_width * 14/8 - feed_width * 14/8,
    };

    SetEDmac(TTL_Args.RD1_Channel, TTL_Args.RD1_Address, &RD1_info, TTL_Args.RD1_Flags);

    if (verbose >= 2)
    {
        const char * WR1_SizeFmt = format_memory_size(GetSizeOfMemoryChunk(GetFirstChunkFromSuite(dst_suite)));
        printf("[TTL] %dx%d %dbpp\n", TTL_Args.xRes, TTL_Args.yRes, TTL_Args.SamplePrecision);
        printf(" WR1: %x EDMAC#%d<%d> (%x %s)\n",  WR1_Address,  TTL_Args.WR1_Channel, TTL_Args.WR1_Connection, TTL_Args.WR1_MemSuite, WR1_SizeFmt);
        printf(" WR2: %x EDMAC#%d<%d>\n", TTL_Args.WR2_Address,  TTL_Args.WR2_Channel, TTL_Args.WR2_Connection);
        printf(" RD1: %x EDMAC#%d<%d>\n", TTL_Args.RD1_Address,  TTL_Args.RD1_Channel, TTL_Args.RD1_Connection);
        printf(" RD2: %x EDMAC#%d<%d>\n", TTL_Args.RD2_Address,  TTL_Args.RD1_Channel, TTL_Args.RD2_Connection);
    }

    /* register our CBR, to be called when finished */
    TTL_RegisterCBR(4, LosslessCompleteCBR, 0);
    
    if (TTL_SetFlags)
    {
        /* this changes a few registers that appear to be bit fields */
        TTL_SetFlags(0x10000);
    }

    /* time the operation */
    start_time = get_us_clock();

    if (TTL_IS_DIGIC4)
    {
        /* Arm before the encoder runs so the first tick cannot mistake an
         * output that has not started moving for one that has stopped. */
        TTL_stall_pos = 0;
        TTL_stall_time = start_time;
        TTL_stall_out = (const unsigned char *) WR1_Address;
        SetHPTimerAfterNow(TTL_D4_TICK_US, TTL_stall_cbr, TTL_stall_cbr,
                           (void *)(intptr_t) ++TTL_stall_gen);
    }

    /* this starts the EDmac channels; on DIGIC 4 the configuring half already ran */
    (TTL_IS_DIGIC4 ? TTL_StartJpCore : TTL_Start)(&TTL_Args);

    /* wait until finished. DIGIC 4 encodes in ~15 ms and the caller runs once
     * per LiveView frame even when idle, so a stuck encode must not cost a second. */
    int err = take_semaphore(lossless_sem, TTL_IS_DIGIC4 ? 100 : 1000);

    /* retire the detector; on timeout it is still ticking */
    TTL_stall_gen++;

    if (TTL_IS_DIGIC4 && err)
    {
        /* detector may have completed just as we gave up; drain that or it
         * would satisfy the next frame's wait before its encoder wrote anything */
        take_semaphore(lossless_sem, 1);
    }

    /* DIGIC 4 reports size nowhere: Finish only unlocks, and the completion CBR
     * publishes the pointer from just before the pop. Read WR1 while the channel
     * is still programmed. */
    uint32_t output_size = (TTL_IS_DIGIC4 && !err) ? TTL_wr1_pos() : 0;

    if (verbose >= 2)
    {
        uint32_t stop_time = get_us_clock();
        printf("[TTL] Elapsed time: %d us\n", (int)(stop_time - start_time));
    }

    /* stop processing; this will report output size (DIGIC 4 leaves it alone) */
    TTL_Stop(&TTL_Args);

    if (TTL_IS_DIGIC4)
        TTL_d4_lv_restore();

    TTL_Finish(lock, &TTL_Args, &output_size);

    /* Prepare-list pointer is restored after Start on the success path; keep the
     * restore here so early returns after a patched Prepare cannot leak it. */
    if (TTL_IS_DIGIC4)
        TTL_d4_restore_prepare_list();

    if (verbose >= 1)
    {
        /* compute input size (uncompressed); the flush rows are not part of the picture */
        uint32_t input_size = RD1_info.xb * height;

        int ratio_x100 = output_size * 10000.0 / input_size;
        printf("[TTL] Output size : %s (%s%d.%02d%%)\n", format_memory_size(output_size), FMT_FIXEDPOINT2(ratio_x100));
    }

    if (err)
        return -2;

    /* do we have valid JPEG data in the output buffer? */
    if (WR1_Address && MEM(WR1_Address) != 0xC4FFD8FF)
        return -3;

    /* Positive size without a terminal EOI is an incomplete frame. */
    if (TTL_IS_DIGIC4 && WR1_Address && output_size > 0
        && !d4_lj92_has_eoi((const unsigned char *) WR1_Address, output_size,
                            TTL_D4_EOI_WINDOW))
        return -7;

    /* Zero size with SOI in place is the same aborted encode as -3; MLV Lite
     * only rejects negatives, so a 0 would become a 0-byte VIDF. */
    if (TTL_IS_DIGIC4 && WR1_Address && output_size <= 0)
        return -3;

    return output_size;
}

int lossless_compress_raw_rectangle(
    struct memSuite * dst_suite, void * src,
    int src_width, int src_x, int src_y,
    int width, int height
)
{
    int res = lossless_compress_raw_rectangle_once(
        dst_suite, src, src_width, src_x, src_y, width, height);

    /* Transient aborts (SOI written, size 0) are usually LiveView holding the
     * resources for one frame, not a bad image. Retry once: double buffering
     * leaves the source intact for ~80 ms and one encode is ~15 ms. A systematic
     * failure fails again and stops the clip as before. */
    if (res == -3 && TTL_IS_DIGIC4)
    {
        res = lossless_compress_raw_rectangle_once(
            dst_suite, src, src_width, src_x, src_y, width, height);
    }

    return res;
}

int lossless_compress_raw(struct raw_info * raw_info, struct memSuite * output_memsuite)
{
    return lossless_compress_raw_rectangle(
        output_memsuite, raw_info->buffer,
        raw_info->width, 0, 0,
        raw_info->width, raw_info->height
    );
}

/* decompression stuff wizardry goes here */
struct DecodeLossless_args
{
    void *address;
    int depth_type;
    int is_not_16bpp;
    int x_2;
    int x_1;
    int x_mul;
    int y_2;
    int ysize_raw;
    int y_mul;
};

static void (*Setup_DecodeLosslessRawPath) (struct DecodeLossless_args *args, void (*read_cbr)(int *), void (*done_cbr)(int *), int *cbr_ctx, int *a5) = NULL;
static void (*Start_DecodeLosslessPath) (struct memSuite *a1) = NULL;
static void (*Cleanup_DecodeLosslessPath) (void) = NULL;
static struct semaphore *decompress_sem = NULL;

static void decompress_init()
{
    /* now check for the needed decompression functions */
    if (is_camera("5D3", "1.1.3"))
    {
        Setup_DecodeLosslessRawPath = (void *) 0xFF3CB010;
        Start_DecodeLosslessPath    = (void *) 0xFF3CB0D8;
        Cleanup_DecodeLosslessPath  = (void *) 0xFF3CB23C;
    }

    if (is_camera("5D3", "1.2.3"))
    {
        Setup_DecodeLosslessRawPath = (void *) 0xff3d3f04;
        Start_DecodeLosslessPath    = (void *) 0xff3d3fcc;
        Cleanup_DecodeLosslessPath  = (void *) 0xff3d4130;
    }
    
    if (is_camera("700D", "1.1.5"))
    {
        Setup_DecodeLosslessRawPath = (void *) 0xFF4294DC;
        Start_DecodeLosslessPath    = (void *) 0xFF4295A4;
        Cleanup_DecodeLosslessPath  = (void *) 0xFF429708;
    }

    if (is_camera("650D", "1.0.4"))
    {
        Setup_DecodeLosslessRawPath = (void *) 0xFF4259F4;
        Start_DecodeLosslessPath    = (void *) 0xFF425ABC;
        Cleanup_DecodeLosslessPath  = (void *) 0xFF425C20;
    }

    if (is_camera("EOSM", "2.0.2"))
    {
        Setup_DecodeLosslessRawPath = (void *) 0xFF42DBD0;
        Start_DecodeLosslessPath    = (void *) 0xFF42DC98;
        Cleanup_DecodeLosslessPath  = (void *) 0xFF42DDFC;
    }

    if (is_camera("100D", "1.0.1"))
    {
        Setup_DecodeLosslessRawPath = (void *) 0xFF42F4C8;
        Start_DecodeLosslessPath    = (void *) 0xFF42F590;
        Cleanup_DecodeLosslessPath  = (void *) 0xFF42F6F4;
    }

    if (is_camera("6D", "1.1.6"))
    {
        Setup_DecodeLosslessRawPath = (void *) 0xFF409218;
        Start_DecodeLosslessPath    = (void *) 0xFF4092E0;
        Cleanup_DecodeLosslessPath  = (void *) 0xFF409444;
    }

    if (is_camera("70D", "1.1.2"))
    {
        Setup_DecodeLosslessRawPath = (void *) 0xFF4309A4;
        Start_DecodeLosslessPath    = (void *) 0xFF430A6C;
        Cleanup_DecodeLosslessPath  = (void *) 0xFF430BD0;
    }

    /* all functions known? having the semaphore is an indicator we can decompress */
    if (Setup_DecodeLosslessRawPath && Start_DecodeLosslessPath && Cleanup_DecodeLosslessPath)
    {
        decompress_sem = create_named_semaphore("decompress_sem", SEM_CREATE_LOCKED);
    }
}

/* this one is called when decompression is done */
static void DecodeLossless_DoneCBR(int *done)
{
    give_semaphore(decompress_sem);
}

/* the read cbr is not used */
static void DecodeLossless_ReadCBR(int *done)
{
}

int lossless_decompress_raw(
    struct memSuite * src, void * dst,
    int width, int height,
    int output_bpp
)
{
    if (!decompress_sem)
    {
        return -1;
    }

    struct DecodeLossless_args decode_opts;

    decode_opts.address = dst;
    decode_opts.depth_type = 4;
    decode_opts.is_not_16bpp = (output_bpp == 16) ? 0 : 1;
    decode_opts.x_1 = width;
    decode_opts.x_2 = 0;
    decode_opts.x_mul = 0;
    decode_opts.ysize_raw = height;
    decode_opts.y_2 = 0;
    decode_opts.y_mul = 0;
    
    /* we dont use that one */
    int done = 0;
    Setup_DecodeLosslessRawPath(&decode_opts, DecodeLossless_ReadCBR, DecodeLossless_DoneCBR, &done, &done);
    Start_DecodeLosslessPath(src);
    
    /* wait for decompression to finish */
    take_semaphore(decompress_sem, 0);
    
    /* clean up */
    Cleanup_DecodeLosslessPath();

    return 0;
}
