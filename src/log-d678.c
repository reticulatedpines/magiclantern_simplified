/* DIGIC 6/7/8 logging experiments
 * based on dm-spy-experiments  code */

#include "dryos.h"
#include "tasks.h"
#include "log-d678.h"
#include "io_trace.h"

/* fixme */
extern __attribute__((long_call)) void DryosDebugMsg(int,int,const char *,...);
extern void dump_file(char* name, uint32_t addr, uint32_t size);
extern int GetMemoryInformation(int *, int *);

/* custom logging buffer */
static char * buf;
static int buf_size = 0;
static int len = 0;

#ifndef LOG_EARLY_STARTUP                  /* we've got malloc */
extern void * _AllocateMemory(size_t);
extern void _FreeMemory(void *);
#endif

/* override Canon's DebugMsg to save all messages */
/* DIGIC 7/8: this runs on both CPU cores */
static void DUMP_ASM my_DebugMsg(int class, int level, char* fmt, ...)
{
    uintptr_t lr = read_lr();

    if (!buf)
        return;
    if (buf_size - len < 100)
        return;

#if 0
    if ((class != 0 || level != 15) && level < 3)
    {
        /* skip "unimportant" messages */
        return;
    }
#endif

#if defined(CONFIG_DIGIC_78X)
    static volatile uint32_t lock;
    uint32_t old = cli_spin_lock(&lock);
#else
    uint32_t old = cli();
#endif

    /* are the interrupts actually disabled? */
    if (!(read_cpsr() & 80))
    {
        qprintf("Interrupts not disabled!\n");
        while(1);
    }

#if defined(CONFIG_DIGIC_78X)
    len += snprintf( buf+len, buf_size-len, "[%d] ", get_cpu_id());
#endif

#ifdef CONFIG_MMIO_TRACE
    uint32_t us_timer = io_trace_get_timer();
#else
    uint32_t us_timer = MEM(0xD400000C);
#endif

    const char *task_name = get_current_task_name();
    
    /* Canon's vsnprintf doesn't know %20s */
    char task_name_padded[11] = "           ";
    int spaces = 10 - strlen(task_name);
    if (spaces < 0)
        spaces = 0;
    snprintf(task_name_padded + spaces, 11 - spaces, "%s", task_name);
    len += snprintf( buf+len, buf_size-len, "%d.%06d  %s:%08x:%02x:%02x: ", us_timer/1000000, us_timer%1000000, task_name_padded, lr-4, class, level );

    va_list ap;
    va_start( ap, fmt );
    len += vsnprintf( buf+len, buf_size-len-1, fmt, ap );
    va_end( ap );

    len += snprintf( buf+len, buf_size-len, "\n" );

#if defined(CONFIG_DIGIC_78X)
    sei_spin_unlock(&lock, old);
#else
    sei(old);
#endif

}


#ifdef CONFIG_ISR_LOGGING
/* comment out one of these to see anything in QEMU */
/* on real hardware, more interrupts are expected */
static char* isr_names[0x200] = {
    [0x0D] = "Omar",
    [0x0E] = "UTimerDriver",
    [0x19] = "OCH_SPx",
    [0x1B] = "dryos_timer",
    [0x1C] = "Omar",
    [0x1E] = "UTimerDriver",
    [0x28] = "HPTimer",
    [0x29] = "OCHxEPx",
    [0x2A] = "MREQ",
    [0x2D] = "Omar",
    [0x2E] = "UTimerDriver",
    [0x35] = "SlowMossy",
    [0x39] = "OCH_SPx",
    [0x3C] = "Omar",
    [0x3E] = "UTimerDriver",
    [0x41] = "WRDMAC1",
    [0x49] = "OCHxEPx",
    [0x4A] = "MREQ2_ICU",
    [0x4D] = "Omar",
    [0x4E] = "UTimerDriver",
    [0x59] = "OCH_SPx",
    [0x5A] = "LENSIF_SEL",
    [0x5C] = "Omar",
    [0x5E] = "UTimerDriver",
    [0x69] = "OCHxEPx",
    [0x6D] = "Omar",
    [0x6E] = "UTimerDriver",
    [0x79] = "OCH_SPx",
    [0x7A] = "XINT_7",
    [0x7C] = "Omar",
    [0x7E] = "UTimerDriver",
    [0x89] = "OCHxEPx",
    [0x8A] = "INT_LM",
    [0x98] = "CAMIF_0",
    [0x99] = "OCH_SPx",
    [0x9C] = "Omar",
    [0xA8] = "CAMIF_1",
    [0xA9] = "OCHxEPx",
    [0xB9] = "OCH_SPx",
    [0xBC] = "Omar",
    [0xBE] = "sd_dma",
    [0xC9] = "OCHxEPx",
    [0xCA] = "INT_LM",
    [0xCD] = "Omar",
    [0xCE] = "SerialFlash",
    [0xD9] = "ICAPCHx",
    [0xDC] = "Omar",
    [0xDE] = "SerialFlash",
    [0xE9] = "ICAPCHx",
    [0xEE] = "sd_driver",
    [0xF9] = "ICAPCHx",
    [0xFC] = "Omar",
    [0xFE] = "SerialFlash",
    [0x102] = "RDDMAC13",
    [0x109] = "ICAPCHx",
    [0x10C] = "BltDmac",
    [0x10E] = "SerialFlash",
    [0x119] = "ICAPCHx",
    [0x129] = "ICAPCHx",
    [0x12A] = "mreq",
    [0x139] = "ICAPCHx",
    [0x13A] = "CAPREADY",
    [0x13E] = "xdmac",
    [0x145] = "MZRM",
    [0x147] = "SIO3",
    [0x149] = "ICAPCHx",
    [0x14E] = "xdmac",
    [0x159] = "ICAPCHx",
    [0x15D] = "uart_rx",
    [0x15E] = "xdmac",
    [0x169] = "ICAPCHx",
    [0x16D] = "uart_tx",
    [0x16E] = "xdmac",
    [0x179] = "ICAPCHx",
    [0x189] = "ICAPCHx",
    [0x18B] = "WdtInt",
};
#endif

static void mpu_decode(const char * in, char * out, int max_len);
static char mpu_msg[768];   /* max message size: 0xFF * 3 */

/* DIGIC 7/8: this runs on both CPU cores */
static void pre_isr_log(uint32_t isr)
{
#ifdef CONFIG_ISR_LOGGING
    extern const uint32_t isr_table_handler[];
    extern const uint32_t isr_table_param[];
    const char * name = isr_names[isr & 0x1FF];
    uint32_t handler = isr_table_handler[2 * isr];
    uint32_t arg     = isr_table_param  [2 * isr];
    DryosDebugMsg(0, 15, ">>> INT-%03Xh %s %X(%X)", isr, name ? name : "", handler, arg);
#endif

    /* DIGIC 7: MPU messages arrive on the first core only,
     * but some of these interrupts are also triggered on second core */
    if ((get_cpu_id() == 0) &&
        (isr == 0x2A || isr == 0x12A || isr == 0x147 || isr == 0x1B))
    {
        /* SIO3/MREQ; also check on timer interrupt */
        #ifdef CONFIG_7D2
        extern const char * const mpu_send_ring_buffer[20];
        #else
        extern const char * const mpu_send_ring_buffer[50];
        #endif
        extern const int mpu_send_ring_buffer_tail;
        static int last_tail = 0;
        while (last_tail != mpu_send_ring_buffer_tail)
        {
            const char * last_message = &mpu_send_ring_buffer[last_tail][4];
            mpu_decode(last_message, mpu_msg, sizeof(mpu_msg));
            //qprintf("[%d] mpu_send(%s)%s\n", last_tail, msg, last_message[-2] == 1 ? "" : " ?!?");
            DryosDebugMsg(0, 15, "[%d] *** mpu_send(%s)%s", last_tail, mpu_msg, last_message[-2] == 1 ? "" : " ?!?");
            INC_MOD(last_tail, COUNT(mpu_send_ring_buffer));
        }
    }
}

/* DIGIC 7/8: this runs on both CPU cores */
static void post_isr_log(uint32_t isr)
{
#ifdef CONFIG_ISR_LOGGING
    const char * name = isr_names[isr & 0x1FF];
    DryosDebugMsg(0, 15, "<<< INT-%03Xh %s", isr, name ? name : "");
#endif

    /* CPU check is here just in case, for future models */
    if ((isr == 0x147) && (get_cpu_id() == 0))
    {
        /* expecting at most one message fully received at the end of this interrupt */
        extern const char * const mpu_recv_ring_buffer[80];
        extern const int mpu_recv_ring_buffer_tail;
        static int last_tail = 0;

        if (last_tail != mpu_recv_ring_buffer_tail)
        {
            const char * last_message = &mpu_recv_ring_buffer[last_tail][4];
            mpu_decode(last_message, mpu_msg, sizeof(mpu_msg));
            //qprintf("[%d] mpu_recv(%s)\n", last_tail, msg);
            DryosDebugMsg(0, 15, "[%d] *** mpu_recv(%s)", last_tail, mpu_msg);
            last_tail = mpu_recv_ring_buffer_tail;
        }
    }
}

extern void (*pre_isr_hook)();
extern void (*post_isr_hook)();

static void mpu_decode(const char * in, char * out, int max_len)
{
    int len = 0;
    int size = (unsigned char) in[0];

    /* print each byte as hex */
    for (const char * c = in; c < in + size; c++)
    {
        len += snprintf(out+len, max_len-len, "%02x ", *c);
    }
    
    /* trim the last space */
    if (len) out[len-1] = 0;
}

#if 0
extern int (*mpu_recv_cbr)(char * buf, int size);
extern int __attribute__((long_call)) mpu_recv(char * buf);

static int mpu_recv_log(char * buf, int size_unused)
{
    int size = buf[-1];
    mpu_decode(buf, mpu_msg, sizeof(mpu_msg));
    DryosDebugMsg(0, 15, "*** mpu_recv(%02x %s)", size, mpu_msg);

    /* call the original */
    return mpu_recv(buf);
}
#endif

int GetFreeMemForAllocateMemory()
{
    int a,b;
    GetMemoryInformation(&a,&b);
    return b;
}

void log_start()
{
    /* allocate memory for our logging buffer */
#ifndef LOG_EARLY_STARTUP
    #ifdef CONFIG_200D
    buf = (char *) 0x56500000;
    buf_size = 4 * 1024 * 1024;
    #else
    buf_size = 2 * 1024 * 1024;
    qprintf("Free memory: %X\n", GetFreeMemForAllocateMemory());
    buf = _AllocateMemory(buf_size);
    #endif
#else
    #ifdef CONFIG_80D
    /* some hardcoded address, likely unused by Canon firmware during startup and light workloads
     * caveat: heavier workloads like burst pictures are likely to allocate memory from here,
     * overwriting our logs (or our logs overwriting Canon's data)
     * https://www.magiclantern.fm/forum/index.php?topic=17360.msg211065#msg211065 */
    buf = (void *) 0x30000000;          /* try 12B00000, 15600000, 18100000, 28000000, 2AB00000, 2D600000, 30000000, 32B00000 */
    buf_size = 32 * 1024 * 1024;        /* actually over 40, but we don't really need that much */
    #endif
    #ifdef CONFIG_5D4
    /* https://www.magiclantern.fm/forum/index.php?topic=17695.msg212320#msg212320 */
    buf = (void *) 0x0B100000;          /* 42600000-42FFFFFF = 10MB, 4B100000-4BCFFFFF = 12MB, 5D100000-5D6FFFFF = 6MB, 60B00000-614FFFFF = 10MB, 7C500000-7D0FFFFF = 12MB; using cacheable versions */
    buf_size = 12 * 1024 * 1024;
    #endif
    /* other models will require different addresses; to be found experimentally */
    #ifdef CONFIG_200D
    buf = (char *) 0x56500000;
    buf_size = 4 * 1024 * 1024;
    #endif
#endif
    qprintf("Logging buffer: %X - %X\n", buf, buf + buf_size - 1);
    qprintf("Free memory: %X\n", GetFreeMemForAllocateMemory());
    while (!buf);

    /* override Canon's DebugMsg (requires RAM address) */
    uint32_t old_int = cli();
    uint32_t DebugMsg_addr = (uint32_t) &DryosDebugMsg & ~1;
    qprintf("Replacing %X DebugMsg with %X...\n", DebugMsg_addr, &my_DebugMsg);
    MEM(DebugMsg_addr)     = 0xC004F8DF;    /* ldr.w  r12, [pc, #4] */
    MEM(DebugMsg_addr + 4) = 0xBF004760;    /* bx r12; nop */
    if (DebugMsg_addr & 2) {
        MEM(DebugMsg_addr + 6) = (uint32_t) &my_DebugMsg;
    } else {
        MEM(DebugMsg_addr + 8) = (uint32_t) &my_DebugMsg;
    }
    sync_caches();
    sei(old_int);

    /* install hooks before and after each hardware interrupt */
    pre_isr_hook = &pre_isr_log;
    post_isr_hook = &post_isr_log;

#if 0
    /* wait for InitializeIntercom to complete
     * then install our own hook quickly
     * this assumes Canon's init_task is already running */
    while (!mpu_recv_cbr)
    {
        msleep(10);
    }
    mpu_recv_cbr = &mpu_recv_log;
#endif

    sync_caches();

#ifdef CONFIG_MMIO_TRACE
    io_trace_prepare();
    io_trace_install();
#endif

    //dm_set_store_level(255, 1);
    DryosDebugMsg(0, 15, "Logging started.");
    //DryosDebugMsg(0, 15, "Free memory: %d bytes.", GetFreeMemForAllocateMemory());

}

void log_finish()
{
#ifdef CONFIG_MMIO_TRACE
    io_trace_uninstall();
    io_trace_dump();
#endif

    //dm_set_store_level(255, 15);
    DryosDebugMsg(0, 15, "Logging finished.");
    DryosDebugMsg(0, 15, "Free memory: %d bytes.", GetFreeMemForAllocateMemory());

    qprintf("Saving log %X size %X...\n", buf, len);
    sync_caches();
    dump_file("DEBUGMSG.LOG", (uint32_t) buf, len);

    pre_isr_hook = 0;
    post_isr_hook = 0;
    sync_caches();
}
