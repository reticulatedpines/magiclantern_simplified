#include "hw/hw.h"
#include "hw/loader.h"
#include "sysemu/sysemu.h"
#include "hw/devices.h"
#include "hw/boards.h"
#include "exec/address-spaces.h"
#include "exec/memory-internal.h"
#include "exec/ram_addr.h"
#include "hw/sysbus.h"
#include "qemu/thread.h"
#include "ui/console.h"
#include "ui/pixel_ops.h"
#include "hw/display/framebuffer.h"
#include "hw/sd/sd.h"
#include "sysemu/char.h"
#include <hw/ide/internal.h>
#include "eos.h"

#include "hw/eos/model_list.h"
#include "hw/eos/eos_ml_helpers.h"
#include "hw/eos/mpu.h"
#include "hw/eos/serial_flash.h"
#include "hw/eos/eos_utils.h"
#include "eos_bufcon_100D.h"

#define IGNORE_CONNECT_POLL

static void edmac_test_format_size(void);

/* Machine class */

typedef struct {
    MachineClass parent;
    struct eos_model_desc * model;
} EosMachineClass;

#define EOS_DESC_BASE    "Canon EOS"
#define TYPE_EOS_MACHINE "eos"
#define EOS_MACHINE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(EosMachineClass, obj, TYPE_EOS_MACHINE)
#define EOS_MACHINE_CLASS(klass) \
    OBJECT_CLASS_CHECK(EosMachineClass, klass, TYPE_EOS_MACHINE)

static void eos_init_common(MachineState *machine);

static void eos_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = EOS_DESC_BASE;
    mc->init = eos_init_common;
}
static const TypeInfo canon_eos_info = {
    .name = TYPE_EOS_MACHINE,
    .parent = TYPE_MACHINE,
    .abstract = true,
//  .instance_size = sizeof(MachineState), // Could probably be used for something
//  .instance_init = vexpress_instance_init,
    .class_size = sizeof(EosMachineClass),
    .class_init = eos_class_init,
};


static void eos_cam_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    EosMachineClass *emc = EOS_MACHINE_CLASS(oc);
    struct eos_model_desc * model = (struct eos_model_desc*) data;
    emc->model = model;

    /* Create description from name */
    int desc_size = sizeof(EOS_DESC_BASE) + strlen(model->name) + 1;
    char * desc = (char*)malloc(desc_size * sizeof(char));
    if (desc) {
        snprintf(desc, desc_size, EOS_DESC_BASE " %s", model->name);
    }
    mc->desc = desc;
}

static void eos_cam_class_finalize(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    if (mc->desc) {
        free((char*)mc->desc);
        mc->desc = NULL;
    }
}


static void eos_machine_init(void)
{
    /* Register base type */
    type_register_static(&canon_eos_info);

    /* Base info for camera models */
    char name[32]; // "XXXX-machine"
    TypeInfo info = {
        .name = name,
        .class_init = eos_cam_class_init,
        .class_finalize = eos_cam_class_finalize,
        .parent = TYPE_EOS_MACHINE,
    };
    
    /* Loop over all models listed in model_list.c */
    
    /* fill in the defaults from generic entries */
    /* note: generic entries don't have a name */
    for (const struct eos_model_desc * generic = eos_model_list; generic->digic_version; generic++)
    {
        if (!generic->name)
        {
            for (struct eos_model_desc * model = eos_model_list; model->digic_version; model++)
            {
                if (model->name && model->digic_version == generic->digic_version)
                {
                    /* copy settings from generic to model */
                    for (int i = 0; i < COUNT(model->params); i++)
                    {
                        if (model->params[i] == 0)
                        {
                            // printf("%s: params[%d] = %x\n", model->name, i, generic->params[i]);
                            model->params[i] = generic->params[i];
                        }
                    }
                }
            }
        }
    }

    /* then register every supported camera model */
    for (struct eos_model_desc * model = eos_model_list; model->digic_version; model++)
    {
        if (model->name)
        {
            snprintf(name, 32, "%s" TYPE_MACHINE_SUFFIX, model->name);
            info.class_data = (void*)model;
            type_register(&info);
        }
    }
}

machine_init(eos_machine_init);



EOSRegionHandler eos_handlers[] =
{
    { "RAM Trace",    0x00000000, 0x10000000, eos_handle_ram, 0 },
    { "FlashControl", 0xC0000000, 0xC0001FFF, eos_handle_flashctrl, 0 },
    { "ROM0",         0xF8000000, 0xFFFFFFFF, eos_handle_rom, 0 },
    { "ROM1",         0xF0000000, 0xF7FFFFFF, eos_handle_rom, 1 },
    { "Interrupt",    0xC0200000, 0xC02000FF, eos_handle_intengine_vx, 0 }, /* mostly used on D2/3, but also 60D */
    { "Interrupt",    0xC0201000, 0xC0201FFF, eos_handle_intengine, 0 },    /* <= D5 */
    { "Interrupt",    0xD4011000, 0xD4011FFF, eos_handle_intengine, 1 },    /* D6; first core in D7 */
    { "Interrupt",    0xD5011000, 0xD5011FFF, eos_handle_intengine, 2 },    /* second core in D7 */
    { "Interrupt",    0xD02C0200, 0xD02C02FF, eos_handle_intengine, 3 },    /* 5D3 eeko */
    { "Interrupt",    0xC1000000, 0xC100FFFF, eos_handle_intengine_gic, 7 },/* D7 */
    { "Timers",       0xC0210000, 0xC0210FFF, eos_handle_timers, 0 },
    { "Timers",       0xD4000240, 0xD4000410, eos_handle_timers, 1 },
    { "Timers",       0xD02C1500, 0xD02C15FF, eos_handle_timers, 2 },
    { "Timer",        0xC0242014, 0xC0242014, eos_handle_digic_timer, 0 },
    { "Timer",        0xD400000C, 0xD400000C, eos_handle_digic_timer, 1 },  /* not sure */
    { "HPTimer",      0xC0243000, 0xC0243FFF, eos_handle_hptimer, 0 },
    { "GPIO",         0xC0220000, 0xC022FFFF, eos_handle_gpio, 0 },
    { "Basic",        0xC0100000, 0xC0100FFF, eos_handle_basic, 0 },
    { "Basic",        0xC0400000, 0xC0400FFF, eos_handle_basic, 1 },
    { "Basic",        0xC0720000, 0xC0720FFF, eos_handle_basic, 2 },
    { "SDIO0",        0xC0C00000, 0xC0C00FFF, eos_handle_sdio, 0 },
    { "SDIO1",        0xC0C10000, 0xC0C10FFF, eos_handle_sdio, 1 },
    { "SDIO2",        0xC0C20000, 0xC0C20FFF, eos_handle_sdio, 2 },
    { "SFIO4",        0xC0C40000, 0xC0C40FFF, eos_handle_sfio, 4 },
    { "SDIO6",        0xC8060000, 0xC8060FFF, eos_handle_sdio, 6 },
    { "CFDMA0",       0xC0500000, 0xC0500FFF, eos_handle_cfdma, 0 },
    { "SDDMA1",       0xC0510000, 0xC05100FF, eos_handle_sddma, 1 },
    { "SDDMA3",       0xC0530000, 0xC0530FFF, eos_handle_sddma, 3 },
    { "SDDMA6",       0xC8020000, 0xC80200FF, eos_handle_sddma, 6 },
    { "CFATA0",       0xC0600000, 0xC060FFFF, eos_handle_cfata, 0 },
    { "CFATA2",       0xC0620000, 0xC062FFFF, eos_handle_cfata, 2 },
    { "UART",         0xC0800000, 0xC08000FF, eos_handle_uart, 0 },
    { "UART",         0xC0810000, 0xC08100FF, eos_handle_uart, 1 },
    { "UART",         0xC0270000, 0xC0270000, eos_handle_uart, 2 },
    { "SIO0",         0xC0820000, 0xC08200FF, eos_handle_sio, 0 },
    { "SIO1",         0xC0820100, 0xC08201FF, eos_handle_sio, 1 },
    { "SIO2",         0xC0820200, 0xC08202FF, eos_handle_sio, 2 },
    { "SIO3",         0xC0820300, 0xC08203FF, eos_handle_sio3, 3 },
    { "SIO4",         0xC0820400, 0xC08204FF, eos_handle_sio_serialflash, 4 },
    { "SIO7",         0xC0820700, 0xC08207FF, eos_handle_sio_serialflash, 7 },
    { "MREQ",         0xC0203000, 0xC02030FF, eos_handle_mreq, 0 },
    { "DMA1",         0xC0A10000, 0xC0A100FF, eos_handle_dma, 1 },
    { "DMA2",         0xC0A20000, 0xC0A200FF, eos_handle_dma, 2 },
    { "DMA3",         0xC0A30000, 0xC0A300FF, eos_handle_dma, 3 },
    { "DMA4",         0xC0A40000, 0xC0A400FF, eos_handle_dma, 4 },
    { "DMA5",         0xC0A50000, 0xC0A500FF, eos_handle_dma, 5 },
    { "DMA6",         0xC0A60000, 0xC0A600FF, eos_handle_dma, 6 },
    { "DMA7",         0xC0A70000, 0xC0A700FF, eos_handle_dma, 7 },
    { "DMA8",         0xC0A80000, 0xC0A800FF, eos_handle_dma, 8 },
    { "CHSW",         0xC0F05000, 0xC0F05FFF, eos_handle_edmac_chsw, 0 },
    { "EDMAC",        0xC0F04000, 0xC0F04FFF, eos_handle_edmac, 0 },
    { "EDMAC",        0xC0F26000, 0xC0F26FFF, eos_handle_edmac, 1 },
    { "EDMAC",        0xC0F30000, 0xC0F30FFF, eos_handle_edmac, 2 },
    { "PREPRO",       0xC0F08000, 0xC0F08FFF, eos_handle_prepro, 0 },
    { "HEAD",         0xC0F07048, 0xC0F0705B, eos_handle_head, 1 },
    { "HEAD",         0xC0F0705C, 0xC0F0706F, eos_handle_head, 2 },
    { "HEAD",         0xC0F07134, 0xC0F07147, eos_handle_head, 3 },
    { "HEAD",         0xC0F07148, 0xC0F0715B, eos_handle_head, 4 },
    { "CARTRIDGE",    0xC0F24000, 0xC0F24FFF, eos_handle_cartridge, 0 },
    { "ASIF",         0xC0920000, 0xC0920FFF, eos_handle_asif, 4 },
    { "Display",      0xC0F14000, 0xC0F14FFF, eos_handle_display, 0 },
    { "Power",        0xC0F01000, 0xC0F010FF, eos_handle_power_control, 1 },
    { "ADC",          0xD9800000, 0xD9800068, eos_handle_adc, 0 },
    { "JP51",         0xC0E00000, 0xC0E0FFFF, eos_handle_jpcore, 0 },
    { "JP62",         0xC0E10000, 0xC0E1FFFF, eos_handle_jpcore, 1 },
    { "JP57",         0xC0E20000, 0xC0E2FFFF, eos_handle_jpcore, 2 },

    { "EEKO",         0xD02C2000, 0xD02C243F, eos_handle_eeko_comm, 0 },

    /* generic catch-all for everything unhandled from this range */
    { "ENGIO",        0xC0F00000, 0xC0FFFFFF, eos_handle_engio, 0 },
    
    { "DIGIC6",       0xC8000000, 0xDFFFFFFF, eos_handle_digic6, 0 },
    
    { "ML helpers",   0xCF123000, 0xCF123EFF, eos_handle_ml_helpers, 0 },
};

/* io range access */
static uint64_t eos_io_read(void *opaque, hwaddr addr, uint32_t size)
{
    addr += IO_MEM_START;

    uint32_t type = MODE_READ;

    return eos_handler ( opaque, addr, type, 0 );
}

static void eos_io_write(void *opaque, hwaddr addr, uint64_t val, uint32_t size)
{
    addr += IO_MEM_START;

    uint32_t type = MODE_WRITE;

    eos_handler ( opaque, addr, type, val );
}

static const MemoryRegionOps iomem_ops = {
    .read = eos_io_read,
    .write = eos_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

#ifdef TRACE_MEM_START
/* memory trace */
static uint8_t trace_mem[TRACE_MEM_LEN];

static uint64_t eos_mem_read(void * base_addr, hwaddr addr, uint32_t size)
{
    uint32_t ret = 0;
    
    switch(size)
    {
        case 1:
            ret = *(uint8_t*)(trace_mem + addr);
            break;
        case 2:
            ret = *(uint16_t*)(trace_mem + addr);
            break;
        case 4:
            ret = *(uint32_t*)(trace_mem + addr);
            break;
    }
    
    printf("MEM(0x%08x) => 0x%x\n", (uint32_t)addr + (uint32_t)(uintptr_t)base_addr, ret);
    return ret;
}

static void eos_mem_write(void * base_addr, hwaddr addr, uint64_t val, uint32_t size)
{
    printf("MEM(0x%08x) = 0x%x\n", (uint32_t)addr + (uint32_t)(uintptr_t)base_addr, (uint32_t)val);
    switch(size)
    {
        case 1:
            *(uint8_t*)(trace_mem + addr) = val;
            break;
        case 2:
            *(uint16_t*)(trace_mem + addr) = val;
            break;
        case 4:
            *(uint32_t*)(trace_mem + addr) = val;
            break;
    }
}

static const MemoryRegionOps mem_ops = {
    .read = eos_mem_read,
    .write = eos_mem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};
#endif



void eos_load_image(EOSState *s, const char * file_rel, int offset, int max_size, uint32_t addr, int swap_endian)
{
    /* all files are loaded from $QEMU_EOS_WORKDIR/CAM/ */
    char file[1024];
    snprintf(file, sizeof(file), "%s/%s/%s", s->workdir, s->model->name, file_rel);

    int size = get_image_size(file);
    if (size < 0)
    {
        fprintf(stderr, "%s: file not found '%s'\n", __func__, file);
        abort();
    }

    if (size < offset) {
        fprintf(stderr, "%s: file '%s': offset '%d' is too big\n", __func__,
            file, offset);
        abort();
    }

    fprintf(stderr, "[EOS] loading '%s'", file);

    uint8_t* buf = malloc(size);
    if (!buf)
    {
        fprintf(stderr, "%s: malloc error loading '%s'\n", __func__, file);
        abort();
    }

    if (load_image(file, buf) != size)
    {
        fprintf(stderr, "%s: error loading '%s'\n", __func__, file);
        abort();
    }

    size = size - offset;

    if ((max_size > 0) && (size > max_size)) {
        size = max_size;
    }
    
    fprintf(stderr, " to 0x%08X-0x%08X", addr, size + addr - 1);
    
    if (offset)
    {
        fprintf(stderr, " (offset 0x%X)", offset);
    }
    
    fprintf(stderr, "\n");
    
    if (swap_endian) {
        reverse_bytes_order(buf + offset, size);
    }

    MEM_WRITE_ROM(addr, buf + offset, size);

    free(buf);
}

static int cfdma_read_data(EOSState *s, CFState *cf);
static int cfdma_write_data(EOSState *s, CFState *cf);
static void cfdma_trigger_interrupt(EOSState *s);

static void *eos_interrupt_thread(void *parm)
{
    EOSState *s = (EOSState *)parm;

    while (1)
    {
        uint32_t pos;

        usleep(0x100);

        /* don't loop thread if cpu stopped in gdb */
        if (s->cpu0 && cpu_is_stopped(CPU(s->cpu0))) {
            continue;
        }

        if (s->cpu1 && cpu_is_stopped(CPU(s->cpu1))) {
            continue;
        }

        qemu_mutex_lock(&s->irq_lock);

        s->digic_timer += 0x100;
        s->digic_timer &= 0xFFF00;
        
        for (pos = 0; pos < COUNT(s->timer_enabled); pos++)
        {
            if (s->timer_enabled[pos])
            {
                s->timer_current_value[pos] += 0x100;

                if (s->timer_current_value[pos] > s->timer_reload_value[pos])
                {
                    s->timer_current_value[pos] = 0;
                }
            }
        }

        /* go through all interrupts and check if they are pending/scheduled */
        for(pos = INT_ENTRIES-1; pos > 0; pos--)
        {
            /* it is pending, so trigger int and set to 0 */
            if(s->irq_schedule[pos] == 1)
            {
                /* wait, its not enabled. keep it pending */
                if(s->irq_enabled[pos] && !s->irq_id)
                {
                    /* timer interrupt will re-fire periodically */
                    if(pos == TIMER_INTERRUPT)
                    {
                        if (qemu_loglevel_mask(CPU_LOG_INT)) {
                            //~ printf("[EOS] trigger int 0x%02X (delayed)\n", pos);    /* quiet */
                        }
                        s->irq_schedule[pos] = s->timer_reload_value[DRYOS_TIMER_ID] >> 8;
                    }
                    else
                    {
                        if (qemu_loglevel_mask(CPU_LOG_INT)) {
                            printf("[EOS] trigger int 0x%02X (delayed)\n", pos);
                        }
                        s->irq_schedule[pos] = 0;
                    }

                    s->irq_id = pos;
                    s->irq_enabled[s->irq_id] = 0;

                    /* fixme: CURRENT_CPU not valid here */
                    cpu_interrupt(CPU(s->cpu0), CPU_INTERRUPT_HARD);
                }
            }

            /* still counting down? */
            if(s->irq_schedule[pos] > 1)
            {
                s->irq_schedule[pos]--;
            }
        }

        /* also check all HPTimers */
        /* note: we can trigger multiple HPTimers on a single interrupt */
        int trigger_hptimers[64] = {0};
        int hptimer_interrupts[COUNT(s->HPTimers)] = {
            0x18, 0x1A, 0x1C, 0x1E, 0, 0,
            HPTIMER_INTERRUPT, HPTIMER_INTERRUPT, HPTIMER_INTERRUPT, HPTIMER_INTERRUPT,
            HPTIMER_INTERRUPT, HPTIMER_INTERRUPT, HPTIMER_INTERRUPT, HPTIMER_INTERRUPT,
        };
        
        for (pos = 0; pos < COUNT(s->HPTimers); pos++)
        {
            if (s->HPTimers[pos].active && s->HPTimers[pos].output_compare == s->digic_timer)
            {
                if (qemu_loglevel_mask(EOS_LOG_IO)) {
                    printf("[HPTimer] Firing HPTimer #%d\n", pos);
                }
                s->HPTimers[pos].triggered = 1;
                int interrupt = hptimer_interrupts[pos];
                assert(interrupt > 0);
                assert(interrupt < COUNT(trigger_hptimers));
                trigger_hptimers[hptimer_interrupts[pos]] = 1;
            }
        }
        
        qemu_mutex_unlock(&s->irq_lock);

        for (int i = 1; i < COUNT(trigger_hptimers); i++)
        {
            if (trigger_hptimers[i])
            {
                eos_trigger_int(s, i, 0);
            }
        }
        
        qemu_mutex_lock(&s->cf.lock);
        
        if (s->cf.dma_read_request)
        {
            s->cf.dma_read_request = cfdma_read_data(s, &s->cf);
        }

        if (s->cf.dma_write_request)
        {
            s->cf.dma_write_request = cfdma_write_data(s, &s->cf);
        }
        
        if (s->cf.pending_interrupt && s->cf.interrupt_enabled == 1)
        {
            cfdma_trigger_interrupt(s);
            s->cf.pending_interrupt = 0;
        }
        
        qemu_mutex_unlock(&s->cf.lock);
    }

    return NULL;
}




/** FRAMEBUFFER & DISPLAY (move to separate file?) **/




// precompute some parts of YUV to RGB computations
static int yuv2rgb_RV[256];
static int yuv2rgb_GU[256];
static int yuv2rgb_GV[256];
static int yuv2rgb_BU[256];

/** http://www.martinreddy.net/gfx/faqs/colorconv.faq
 * BT 601:
 * R'= Y' + 0.000*U' + 1.403*V'
 * G'= Y' - 0.344*U' - 0.714*V'
 * B'= Y' + 1.773*U' + 0.000*V'
 * 
 * BT 709:
 * R'= Y' + 0.0000*Cb + 1.5701*Cr
 * G'= Y' - 0.1870*Cb - 0.4664*Cr
 * B'= Y' - 1.8556*Cb + 0.0000*Cr
 */

static void precompute_yuv2rgb(int rec709)
{
    int u, v;
    if (rec709)
    {
        /*
        *R = *Y + 1608 * V / 1024;
        *G = *Y -  191 * U / 1024 - 478 * V / 1024;
        *B = *Y + 1900 * U / 1024;
        */
        for (u = 0; u < 256; u++)
        {
            int8_t U = u;
            yuv2rgb_GU[u] = (-191 * U) >> 10;
            yuv2rgb_BU[u] = (1900 * U) >> 10;
        }

        for (v = 0; v < 256; v++)
        {
            int8_t V = v;
            yuv2rgb_RV[v] = (1608 * V) >> 10;
            yuv2rgb_GV[v] = (-478 * V) >> 10;
        }
    }
    else // REC 601
    {
        /*
        *R = *Y + ((1437 * V) >> 10);
        *G = *Y -  ((352 * U) >> 10) - ((731 * V) >> 10);
        *B = *Y + ((1812 * U) >> 10);
        */
        for (u = 0; u < 256; u++)
        {
            int8_t U = u;
            yuv2rgb_GU[u] = (-352 * U) >> 10;
            yuv2rgb_BU[u] = (1812 * U) >> 10;
        }

        for (v = 0; v < 256; v++)
        {
            int8_t V = v;
            yuv2rgb_RV[v] = (1437 * V) >> 10;
            yuv2rgb_GV[v] = (-731 * V) >> 10;
        }
    }
}

#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))

static void yuv2rgb(int Y, int U, int V, int* R, int* G, int* B)
{
    const int v_and_ff = V & 0xFF;
    const int u_and_ff = U & 0xFF;
    int v = Y + yuv2rgb_RV[v_and_ff];
    *R = COERCE(v, 0, 255);
    v = Y + yuv2rgb_GU[u_and_ff] + yuv2rgb_GV[v_and_ff];
    *G = COERCE(v, 0, 255);
    v = Y + yuv2rgb_BU[u_and_ff];
    *B = COERCE(v, 0, 255);
}

#define UYVY_GET_Y1(uyvy) (((uyvy) >>  8) & 0xFF)
#define UYVY_GET_Y2(uyvy) (((uyvy) >> 24) & 0xFF)
#define UYVY_GET_U(uyvy)  (((uyvy)      ) & 0xFF)
#define UYVY_GET_V(uyvy)  (((uyvy) >> 16) & 0xFF)


/* todo: supoort other bith depths */

typedef void (*drawfn_bmp_yuv)(void *, uint8_t *, const uint8_t *, const uint8_t*, int, int, int);

static void draw_line8_32(void *opaque,
                uint8_t *d, const uint8_t *s, int width, int deststep)
{
    uint8_t v, r, g, b;
    EOSState* ws = (EOSState*) opaque;
    
    do {
        v = ldub_p((void *) s);
        if (v)
        {
            r = ws->disp.palette_8bit[v].R;
            g = ws->disp.palette_8bit[v].G;
            b = ws->disp.palette_8bit[v].B;
            ((uint32_t *) d)[0] = rgb_to_pixel32(r, g, b);
        }
        else
        {
            r = g = b = rand();
            ((uint32_t *) d)[0] = rgb_to_pixel32(r, g, b);
        }
        s ++;
        d += 4;
    } while (-- width != 0);
}

static void draw_line4_32(void *opaque,
                uint8_t *d, const uint8_t *s, int width, int deststep)
{
    uint8_t v, r, g, b;
    EOSState* ws = (EOSState*) opaque;
    
    do {
        v = ldub_p((void *) s);
        v = ((uintptr_t)d/4 % 2) ? (v >> 4) & 0xF : v & 0xF;
        
        r = ws->disp.palette_4bit[v].R;
        g = ws->disp.palette_4bit[v].G;
        b = ws->disp.palette_4bit[v].B;
        ((uint32_t *) d)[0] = rgb_to_pixel32(r, g, b);

        if ((uintptr_t)d/4 % 2) s ++;
        d += 4;
    } while (-- width != 0);
}

static void draw_line8_32_bmp_yuv(void *opaque,
                uint8_t *d, const uint8_t *bmp, const uint8_t *yuv, int width, int deststep, int yuvstep)
{
    uint8_t v, r, g, b;
    EOSState* ws = (EOSState*) opaque;
    
    do {
        v = ldub_p((void *) bmp);
        if (v)
        {
            r = ws->disp.palette_8bit[v].R;
            g = ws->disp.palette_8bit[v].G;
            b = ws->disp.palette_8bit[v].B;
            ((uint32_t *) d)[0] = rgb_to_pixel32(r, g, b);
        }
        else
        {
            uint32_t uyvy =  ldl_p((void*)((uintptr_t)yuv & ~3));
            int Y = (uintptr_t)yuv & 3 ? UYVY_GET_Y2(uyvy) : UYVY_GET_Y1(uyvy);
            int U = UYVY_GET_U(uyvy);
            int V = UYVY_GET_V(uyvy);
            int R, G, B;
            yuv2rgb(Y, U, V, &R, &G, &B);
            ((uint32_t *) d)[0] = rgb_to_pixel32(R, G, B);
        }
        bmp ++;
        yuv += yuvstep;
        d += 4;
    } while (-- width != 0);
}

/* similar to QEMU's framebuffer_update_display, but with two image planes */
/* main plane is BMP (8-bit, same size as output), secondary plane is YUV (scaled to match the BMP one) */
static void framebuffer_update_display_bmp_yuv(
    DisplaySurface *ds,
    MemoryRegion *address_space,
    hwaddr base_bmp,
    hwaddr base_yuv,
    int cols, /* Width in pixels.  */
    int rows_bmp, /* Height in pixels.  */
    int rows_yuv,
    int src_width_bmp, /* Length of source line, in bytes.  */
    int src_width_yuv,
    int dest_row_pitch, /* Bytes between adjacent horizontal output pixels.  */
    int dest_col_pitch, /* Bytes between adjacent vertical output pixels.  */
    int invalidate, /* nonzero to redraw the whole image.  */
    drawfn_bmp_yuv fn,
    void *opaque,
    int *first_row, /* Input and output.  */
    int *last_row /* Output only */)
{
    hwaddr src_len_bmp;
    hwaddr src_len_yuv;
    uint8_t *dest;
    uint8_t *src_bmp;
    uint8_t *src_yuv;
    uint8_t *src_base_bmp;
    uint8_t *src_base_yuv;
    int first, last = 0;
    int dirty;
    int i;
    ram_addr_t addr_bmp;
    ram_addr_t addr_yuv;
    ram_addr_t addr_base_yuv;
    MemoryRegionSection mem_section_bmp;
    MemoryRegionSection mem_section_yuv;
    MemoryRegion *mem_bmp;
    MemoryRegion *mem_yuv;

    i = *first_row;
    *first_row = -1;
    src_len_bmp = src_width_bmp * rows_bmp;
    src_len_yuv = src_width_yuv * rows_yuv;

    mem_section_bmp = memory_region_find(address_space, base_bmp, src_len_bmp);
    mem_section_yuv = memory_region_find(address_space, base_yuv, src_len_yuv);
    mem_bmp = mem_section_bmp.mr;
    mem_yuv = mem_section_yuv.mr;
    if (int128_get64(mem_section_bmp.size) != src_len_bmp ||
            !memory_region_is_ram(mem_section_bmp.mr)) {
        goto out;
    }
    assert(mem_bmp);
    assert(mem_section_bmp.offset_within_address_space == base_bmp);

    if (int128_get64(mem_section_yuv.size) != src_len_yuv ||
            !memory_region_is_ram(mem_section_yuv.mr)) {
        goto out;
    }
    assert(mem_yuv);
    assert(mem_section_yuv.offset_within_address_space == base_yuv);

    memory_region_sync_dirty_bitmap(mem_bmp);
    memory_region_sync_dirty_bitmap(mem_yuv);
    src_base_bmp = cpu_physical_memory_map(base_bmp, &src_len_bmp, 0);
    src_base_yuv = cpu_physical_memory_map(base_yuv, &src_len_yuv, 0);
    /* If we can't map the framebuffer then bail.  We could try harder,
       but it's not really worth it as dirty flag tracking will probably
       already have failed above.  */
    if (!src_base_bmp)
        goto out;
    if (!src_base_yuv)
        goto out;
    if (src_len_bmp != src_width_bmp * rows_bmp) {
        cpu_physical_memory_unmap(src_base_bmp, src_len_bmp, 0, 0);
        goto out;
    }
    if (src_len_yuv != src_width_yuv * rows_yuv) {
        cpu_physical_memory_unmap(src_base_yuv, src_len_yuv, 0, 0);
        goto out;
    }
    src_bmp = src_base_bmp;
    src_yuv = src_base_yuv;
    dest = surface_data(ds);
    if (dest_col_pitch < 0)
        dest -= dest_col_pitch * (cols - 1);
    if (dest_row_pitch < 0) {
        dest -= dest_row_pitch * (rows_bmp - 1);
    }
    first = -1;
    addr_bmp = mem_section_bmp.offset_within_region;
    addr_yuv = mem_section_yuv.offset_within_region;
    addr_base_yuv = addr_yuv;

    int j = i * rows_yuv / rows_bmp;
    addr_bmp += i * src_width_bmp;
    src_bmp += i * src_width_bmp;
    addr_yuv = addr_base_yuv + j * src_width_yuv;
    src_yuv = src_base_yuv + j * src_width_yuv;
    dest += i * dest_row_pitch;
    
    /* fixme: only works for integer factors */
    int src_yuv_pitch = src_width_yuv / cols;

    for (; i < rows_bmp; i++) {
        dirty = memory_region_get_dirty(mem_bmp, addr_bmp, src_width_bmp,
                                             DIRTY_MEMORY_VGA);
        dirty |= memory_region_get_dirty(mem_yuv, addr_yuv, src_width_yuv,
                                             DIRTY_MEMORY_VGA);
        if (dirty || invalidate) {
            fn(opaque, dest, src_bmp, src_yuv, cols, dest_col_pitch, src_yuv_pitch);
            if (first == -1)
                first = i;
            last = i;
        }

        int j = i * rows_yuv / rows_bmp;
        addr_bmp += src_width_bmp;
        src_bmp += src_width_bmp;
        addr_yuv = addr_base_yuv + j * src_width_yuv;
        src_yuv = src_base_yuv + j * src_width_yuv;
        dest += dest_row_pitch;
    }
    cpu_physical_memory_unmap(src_base_bmp, src_len_bmp, 0, 0);
    cpu_physical_memory_unmap(src_base_yuv, src_len_yuv, 0, 0);
    if (first < 0) {
        goto out;
    }
    memory_region_reset_dirty(mem_bmp, mem_section_bmp.offset_within_region, src_len_bmp,
                              DIRTY_MEMORY_VGA);
    memory_region_reset_dirty(mem_yuv, mem_section_yuv.offset_within_region, src_len_yuv,
                              DIRTY_MEMORY_VGA);
    *first_row = first;
    *last_row = last;
out:
    memory_region_unref(mem_bmp);
    memory_region_unref(mem_yuv);
}

static void eos_update_display(void *parm)
{
    EOSState *s = (EOSState *)parm;

    DisplaySurface *surface = qemu_console_surface(s->disp.con);
    
    /* these numbers need double-checking */
    /*                  LCD    HDMI-1080   HDMI-480    SD-PAL      SD-NTSC */
    int widths[]      = {   720,   960,        720,        720,        720     };
    int heights[]     = {   480,   540,        480,        576,        480     };
    int yuv_widths[]  = {   720,  1920,        720,        540,        540     };
    int yuv_heights[] = {   480,  1080,        480,        572,        480     };
    
    int width       = widths     [s->disp.type];
    int height      = heights    [s->disp.type];
    int yuv_width   = yuv_widths [s->disp.type];
    int yuv_height  = yuv_heights[s->disp.type];
    
    if (s->model->digic_version < 4)
    {
        /* for VxWorks bootloader */
        height /= 2;
    }
    
    if (s->disp.width && s->disp.height)
    {
        /* did we manage to get them from registers? override the above stuff */
        width = s->disp.width;
        height = s->disp.height;
    }

    if (width != surface_width(surface) || height != surface_height(surface))
    {
        qemu_console_resize(s->disp.con, width, height);
        surface = qemu_console_surface(s->disp.con);
        s->disp.invalidate = 1;
    }
    
    int first, last;
    
    first = 0;
    int linesize = surface_stride(surface);
    
    if (s->disp.is_4bit)
    {
        /* bootloader config, 4 bpp */
        uint64_t size = height * linesize;
        MemoryRegionSection section = memory_region_find(
            s->system_mem,
            s->disp.bmp_vram ? s->disp.bmp_vram : 0x08000000,
            size
        );
        framebuffer_update_display(
            surface,
            &section,
            width, height,
            s->disp.bmp_pitch, linesize, 0, 1,
            draw_line4_32, s,
            &first, &last
        );
    }
    else if (s->disp.img_vram)
    {
        framebuffer_update_display_bmp_yuv(
            surface,
            s->system_mem,
            s->disp.bmp_vram,
            s->disp.img_vram,
            width, height, yuv_height,
            s->disp.bmp_pitch, yuv_width*2, linesize, 0, s->disp.invalidate,
            draw_line8_32_bmp_yuv, s,
            &first, &last
        );
    }
    else
    {
        uint64_t size = height * linesize;
        MemoryRegionSection section = memory_region_find(
            s->system_mem,
            s->disp.bmp_vram ? s->disp.bmp_vram : 0x08000000,
            size
        );
        framebuffer_update_display(
            surface,
            &section,
            width, height,
            s->disp.bmp_pitch, linesize, 0, 1,
            draw_line8_32, s,
            &first, &last
        );
    }

    if (s->card_led)
    {
        /* draw the LED at the bottom-right corner of the screen */
        int x_led = width - 8;
        int y_led = height - 8;
        uint8_t * dest = surface_data(surface);
        for (int dy = -5; dy <= 5; dy++)
        {
            for (int dx = -5; dx <= 5; dx++)
            {
                int r2 = dx*dx + dy*dy;
                if (r2 < 5*5)
                {
                    ((uint32_t *) dest)[x_led+dx + width*(y_led+dy)] =
                        (r2 >= 4*4)         ? rgb_to_pixel32(0, 0, 0)       :
                        (s->card_led == 1)  ? rgb_to_pixel32(255, 0, 0)     :
                                              rgb_to_pixel32(64, 64, 64) ;
                }
            }
        }
    }

    if (first >= 0) {
        dpy_gfx_update(s->disp.con, 0, first, width, last - first + 1);
    }
    
    s->disp.invalidate = 0;
}

static void eos_invalidate_display(void *parm)
{
    EOSState *s = (EOSState *)parm;
    s->disp.invalidate = 1;
}

static const GraphicHwOps eos_display_ops = {
    .invalidate  = eos_invalidate_display,
    .gfx_update  = eos_update_display,
};

static void eos_key_event(void *parm, int keycode)
{
    /* keys sent to guest machine */
    EOSState *s = (EOSState *)parm;
    mpu_send_keypress(s, keycode);
    //s->keyb.buf[(s->keyb.tail++) & 15] = keycode;
}

/**
 * UART code taken from hw/char/digic-uart.c
 * (sorry, couldn't figure out how to reuse it...)
 */

enum {
    ST_RX_RDY = (1 << 0),
    ST_TX_RDY = (1 << 1),
};

static int eos_uart_can_rx(void *opaque)
{
    DigicUartState *s = opaque;

    return !(s->reg_st & ST_RX_RDY);
}

static void eos_uart_rx(void *opaque, const uint8_t *buf, int size)
{
    DigicUartState *s = opaque;

    assert(eos_uart_can_rx(opaque));

    s->reg_st |= ST_RX_RDY;
    s->reg_rx = *buf;
    
    EOSState *es = (EOSState *)(opaque - offsetof(EOSState, uart));
    if (strcmp(es->model->name, "5D3eeko") == 0)
    {
        /* fixme: hardcoded for Eeko */
        eos_trigger_int(es, 0x39, 0);
    }
}

static void eos_uart_event(void *opaque, int event)
{
}

static void eos_uart_reset(DigicUartState *s)
{
    s->reg_rx = 0;
    s->reg_st = ST_TX_RDY;
}

/** EOS CPU SETUP **/


static EOSState *eos_init_cpu(struct eos_model_desc * model)
{
    EOSState *s = g_new(EOSState, 1);
    memset(s, 0, sizeof(*s));

    s->model = model;

    s->workdir = getenv("QEMU_EOS_WORKDIR");
    if (!s->workdir) s->workdir = ".";

    const char* cpu_name = 
        (s->model->digic_version <= 5) ? "arm946eos" :
        (s->model->digic_version == 7) ? "cortex-a9" :
        (s->model->digic_version >= 6) ? "arm-digic6-eos" :
                                         "arm946";
    
    s->cpu0 = cpu_arm_init(cpu_name);
    assert(s->cpu0);

    if (s->model->digic_version == 7)
    {
        s->cpu1 = cpu_arm_init(cpu_name);
        assert(s->cpu1);
        CPU(s->cpu1)->halted = 1;
    }

    s->verbosity = 0xFFFFFFFF;
    s->tio_rxbyte = 0x100;

    s->system_mem = get_system_memory();

    if (ATCM_SIZE)
    {
        memory_region_init_ram(&s->tcm_code, NULL, "eos.tcm_code", ATCM_SIZE, &error_abort);
        memory_region_add_subregion(s->system_mem, ATCM_ADDR, &s->tcm_code);
    }

    if (BTCM_SIZE)
    {
        memory_region_init_ram(&s->tcm_data, NULL, "eos.tcm_data", BTCM_SIZE, &error_abort);
        memory_region_add_subregion(s->system_mem, BTCM_ADDR, &s->tcm_data);
    }

    /* set up RAM, cached and uncached */
    /* main RAM starts at 0 */
    /* the ATCM overlaps the RAM (so far all models);
     * the BTCM may or may not overlap the uncached RAM (model-dependent) */
    assert(ATCM_ADDR == 0);
    
    if (BTCM_ADDR == CACHING_BIT)
    {
        /* not sure what to do if both TCMs overlap the RAM,
         * when they have different sizes */
        assert(ATCM_SIZE == BTCM_SIZE);
    }
    
    memory_region_init_ram(&s->ram, NULL, "eos.ram", RAM_SIZE - ATCM_SIZE, &error_abort);
    memory_region_add_subregion(s->system_mem, 0 + ATCM_SIZE, &s->ram);
    memory_region_init_alias(&s->ram_uncached, NULL, "eos.ram_uncached", &s->ram, 0x00000000, RAM_SIZE - ATCM_SIZE);
    memory_region_add_subregion(s->system_mem, CACHING_BIT + ATCM_SIZE, &s->ram_uncached);
    
    if (ATCM_SIZE && (BTCM_ADDR != CACHING_BIT))
    {
        /* I believe there's a small section of RAM visible only as uncacheable (to be tested) */
        memory_region_init_ram(&s->ram_uncached0, NULL, "eos.ram_uncached0", ATCM_SIZE, &error_abort);
        memory_region_add_subregion(s->system_mem, CACHING_BIT, &s->ram_uncached0);
    }
    
    if (s->model->ram_extra_addr)
    {
        memory_region_init_ram(&s->ram_extra, NULL, "eos.ram_extra", s->model->ram_extra_size, &error_abort);
        memory_region_add_subregion(s->system_mem, s->model->ram_extra_addr, &s->ram_extra);
    }

    /* set up ROM0 */
    if (ROM0_SIZE)
    {
        memory_region_init_ram(&s->rom0, NULL, "eos.rom0", ROM0_SIZE, &error_abort);
        memory_region_add_subregion(s->system_mem, ROM0_ADDR, &s->rom0);

        for(uint64_t offset = ROM0_ADDR + ROM0_SIZE; offset < ROM1_ADDR; offset += ROM0_SIZE)
        {
            char name[32];
            MemoryRegion *image = g_new(MemoryRegion, 1);
            sprintf(name, "eos.rom0_mirror_%02X", (uint32_t)offset >> 24);

            memory_region_init_alias(image, NULL, name, &s->rom0, 0x00000000, ROM0_SIZE);
            memory_region_add_subregion(s->system_mem, offset, image);
        }
    }

    if (ROM1_SIZE)
    {
        /* set up ROM1 */
        memory_region_init_ram(&s->rom1, NULL, "eos.rom1", ROM1_SIZE, &error_abort);
        memory_region_add_subregion(s->system_mem, ROM1_ADDR, &s->rom1);

        for(uint64_t offset = ROM1_ADDR + ROM1_SIZE; offset < 0x100000000; offset += ROM1_SIZE)
        {
            char name[32];
            MemoryRegion *image = g_new(MemoryRegion, 1);
            sprintf(name, "eos.rom1_mirror_%02X", (uint32_t)offset >> 24);

            memory_region_init_alias(image, NULL, name, &s->rom1, 0x00000000, ROM1_SIZE);
            memory_region_add_subregion(s->system_mem, offset, image);
        }
    }

    //memory_region_init_ram(&s->rom1, "eos.rom", 0x10000000, &error_abort);
    //memory_region_add_subregion(s->system_mem, 0xF0000000, &s->rom1);

    /* set up io space */
    memory_region_init_io(&s->iomem, NULL, &iomem_ops, s, "eos.iomem", s->model->io_mem_size);
    memory_region_add_subregion(s->system_mem, IO_MEM_START, &s->iomem);

#ifdef TRACE_MEM_START
    /* optional memory access logging */
    memory_region_init_io(&s->tracemem, NULL, &mem_ops, (void*)TRACE_MEM_START, "eos.tracemem", TRACE_MEM_LEN);
    memory_region_add_subregion(s->system_mem, TRACE_MEM_START, &s->tracemem);

    memory_region_init_io(&s->tracemem_uncached, NULL, &mem_ops, (void*)(TRACE_MEM_START | CACHING_BIT), "eos.tracemem_u", TRACE_MEM_LEN);
    memory_region_add_subregion(s->system_mem, TRACE_MEM_START | CACHING_BIT, &s->tracemem_uncached);
#endif

    /*ROMState *rom0 = eos_rom_register(0xF8000000, NULL, "ROM1", ROM1_SIZE,
                                NULL,
                                0x100, 0x100, 32,
                                0, 0, 0, 0, 0);

                                */

    vmstate_register_ram_global(&s->ram);

    s->rtc.transfer_format = 0xFF;

    qemu_mutex_init(&s->irq_lock);
    qemu_thread_create(&s->interrupt_thread_id, "eos_interrupt", eos_interrupt_thread, s, QEMU_THREAD_JOINABLE);

    /* init display */
    precompute_yuv2rgb(1);
    s->disp.con = graphic_console_init(NULL, 0, &eos_display_ops, s);
    s->disp.bmp_pitch = 960; /* fixme: get it from registers */
    
    qemu_add_kbd_event_handler(eos_key_event, s);

    return s;
}

static void patch_7D2(EOSState *s)
{
    int is_7d2m = (eos_get_mem_w(s, 0xFE106062) == 0x0F31EE19);

    if (is_7d2m)
    {
        uint32_t nop = 0x8000F3AF;
        uint32_t ret = 0x00004770;
        uint32_t one = 1;

        printf("Patching 0x%X (enabling TIO on 7D2M)\n", 0xFEC4DCBC);
        MEM_WRITE_ROM(0xFEC4DCBC, (uint8_t*) &one, 4);
        
        MEM_WRITE_ROM(0xFE0A3024, (uint8_t*) &nop, 4);
        printf("Patching 0x%X (idk, it fails)\n", 0xFE0A3024);
        
        MEM_WRITE_ROM(0xFE102B5A, (uint8_t*) &ret, 4);
        printf("Patching 0x%X (PROPAD_CreateFROMPropertyHandle)\n", 0xFE102B5A);
    }
    else
    {
        printf("This ROM doesn't look like a 7D2M\n");
    }
}

static void patch_EOSM3(EOSState *s)
{
    printf("Patching 0xFCC637A8 (enabling TIO)\n");
    uint32_t one = 1;
    MEM_WRITE_ROM(0xFCC637A8, (uint8_t*) &one, 4);

    /* fixme: timer issue? some interrupt that needs triggered? */
    printf("Patching 0xFC1F0116 (usleep)\n");
    uint32_t bx_lr = 0x4770;
    MEM_WRITE_ROM(0xFC1F0116, (uint8_t*) &bx_lr, 2);

    printf("Patching 0xFC0F45B8 (InitExDrivers, locks up)\n");
    MEM_WRITE_ROM(0xFC0F45B8, (uint8_t*) &bx_lr, 2);

    printf("Patching 0xFC1F455C (DcdcDrv, assert i2c)\n");
    MEM_WRITE_ROM(0xFC1F455C, (uint8_t*) &bx_lr, 2);

    printf("Patching 0xFC4FE848 (JpCore, assert)\n");
    MEM_WRITE_ROM(0xFC4FE848, (uint8_t*) &bx_lr, 2);

    printf("Patching 0xFC284B20 and 0xFC284B80 (Hdmi_comm, assert)\n");
    MEM_WRITE_ROM(0xFC284B20, (uint8_t*) &bx_lr, 2);
    MEM_WRITE_ROM(0xFC284B80, (uint8_t*) &bx_lr, 2);

    printf("Patching 0xFC10C1A4 and 0xFC10C2B2 (DefMarkManLeo, assert)\n");
    MEM_WRITE_ROM(0xFC10C1A4, (uint8_t*) &bx_lr, 2);
    MEM_WRITE_ROM(0xFC10C2B2, (uint8_t*) &bx_lr, 2);

    printf("Patching 0xFC2A0F38 (SoundTsk, assert)\n");
    MEM_WRITE_ROM(0xFC2A0F38, (uint8_t*) &bx_lr, 2);

    printf("Patching 0xFC1847E4 (MechaCPUFirmTransfer, assert)\n");
    MEM_WRITE_ROM(0xFC1847E4, (uint8_t*) &bx_lr, 2);
}

static void patch_EOSM10(EOSState *s)
{
    printf("Patching 0xFCE642A8 (enabling TIO)\n");
    uint32_t one = 1;
    MEM_WRITE_ROM(0xFCE642A8, (uint8_t*) &one, 4);
    
    /* fixme: timer issue? some interrupt that needs triggered? */
    printf("Patching 0xFE1ED4D6 (usleep)\n");
    uint32_t bx_lr = 0x4770;
    MEM_WRITE_ROM(0xFE1ED4D6, (uint8_t*) &bx_lr, 2);
}

static void patch_EOSM5(EOSState *s)
{
    /* 0x4060, in the block copied from 0xE001B2E4 to 0x4000 */
    printf("Patching 0xE001B2E4+0x60 (enabling TIO on DryOs #1)\n");
    uint32_t one = 1;
    MEM_WRITE_ROM(0xE001B2E4+0x60, (uint8_t*) &one, 4);

    /* 0x8098, in the block copied from 0xE115CF88 to 0x8000 */
    printf("Patching 0xE115CF88+0x98 (enabling TIO on DryOs #2)\n");
    MEM_WRITE_ROM(0xE115CF88+0x98, (uint8_t*) &one, 4);
}

static void eos_init_common(MachineState *machine)
{
    EOSState *s = eos_init_cpu(EOS_MACHINE_GET_CLASS(machine)->model);

    /* populate ROM0 */
    if (ROM0_SIZE)
    {
        eos_load_image(s, "ROM0.BIN", 0, ROM0_SIZE, ROM0_ADDR, 0);
    }
    
    /* populate ROM1 */
    if (ROM1_SIZE)
    {
        eos_load_image(s, "ROM1.BIN", 0, ROM1_SIZE, ROM1_ADDR, 0);
    }

    /* init SD card */
    DriveInfo *di;
    /* FIXME use a qdev drive property instead of drive_get_next() */
    di = drive_get_next(IF_SD);
    s->sd.card = sd_init(di ? blk_by_legacy_dinfo(di) : NULL, false);
    if (!s->sd.card) {
        printf("SD init failed\n");
        exit(1);
    }
    
    /* init CF card */
    DriveInfo *dj;
    dj = drive_get_next(IF_IDE);
    if (!dj) {
        printf("CF init failed\n");
        exit(1);
    }

    ide_bus_new(&s->cf.bus, sizeof(s->cf.bus), NULL, 0, 2);
    ide_init2(&s->cf.bus, s->interrupt);
    ide_create_drive(&s->cf.bus, 0, dj);
    s->cf.bus.ifs[0].drive_kind = IDE_CFATA;
    qemu_mutex_init(&s->cf.lock);


    /* nkls: init SF */
    if (s->model->serial_flash_size)
    {
        char sf_filename[1024];
        snprintf(sf_filename, sizeof(sf_filename), "%s/%s/SFDATA.BIN", s->workdir, s->model->name);
        s->sf = serial_flash_init(sf_filename, s->model->serial_flash_size);
    }
    
    /* init UART */
    /* FIXME use a qdev chardev prop instead of qemu_char_get_next_serial() */
    s->uart.chr = qemu_char_get_next_serial();
    if (s->uart.chr) {
        qemu_chr_add_handlers(s->uart.chr, eos_uart_can_rx, eos_uart_rx, eos_uart_event, &s->uart);
    }
    eos_uart_reset(&s->uart);


    /* init MPU */
    mpu_spells_init(s);

    edmac_test_format_size();

    if ((strcmp(s->model->name, "7D2M") == 0) ||
        (strcmp(s->model->name, "7D2S") == 0))
    {
        /* 7D2 experiments */
        patch_7D2(s);
    }
    
    if (strcmp(s->model->name, "EOSM3") == 0)
    {
        patch_EOSM3(s);
    }

    if (strcmp(s->model->name, "EOSM10") == 0)
    {
        patch_EOSM10(s);
    }

    if (strcmp(s->model->name, "EOSM5") == 0)
    {
        patch_EOSM5(s);
    }

    if (s->model->digic_version == 6)
    {
        /* fixme: initial PC should probably be set in cpu.c */
        /* note: DIGIC 4 and 5 start execution at FFFF0000 (hivecs) */
        s->cpu0->env.regs[15] = eos_get_mem_w(s, 0xFC000000);
        printf("Start address: 0x%08X\n", s->cpu0->env.regs[15]);
    }

    if (s->model->digic_version == 7)
    {
        /* fixme: what configures this address as startup? */
        s->cpu0->env.regs[15] = 0xE0000000;
        s->cpu1->env.regs[15] = 0xE0000000;
        printf("Start address: 0x%08X\n", s->cpu0->env.regs[15]);
    }

    if (strcmp(s->model->name, "5D3eeko") == 0)
    {
        /* see EekoBltDmac calls (5D3 1.1.3)
         * EekoBltDmac(0x0, 0xd0288000, 0xff99541c, 0x6b8c,  0xff508e78, 0x0), from ff508f30
         * EekoBltDmac(0x0, 0x1e80000,  0xff99c164, 0x10e8,  0xff508e78, 0x0), from ff508fd0
         * EekoBltDmac(0x0, 0x1e00000,  0xff8bf888, 0x4ef14, 0xff217de8, 0x0), from ff217e34
         * EekoBltDmac(0x0, 0xd0280000, 0xff99bfa8, 0x1bc,   0xff508e78, 0x0), from ff508fd0
         */
        
        /* all dumps must be made before starting the Eeko core, but after the above copy calls
         * 5D3 1.1.3: 0xFF508F78 (right before writing 7 to C022320C) */
        eos_load_image(s, "D0288000.DMP", 0, 0, 0,          0); /* 0x008000 bytes */
        eos_load_image(s, "D0280000.DMP", 0, 0, 0x40000000, 0); /* 0x004000 bytes */
        eos_load_image(s, "1E00000.DMP",  0, 0, 0x1E00000,  0); /* 0x120000 bytes (overlaps 2 regions) */
        eos_load_image(s, "1F20000.DMP",  0, 0, 0x1F20000,  0); /* 0x020000 bytes (non-shareable device) */
        s->cpu0->env.regs[15] = 0;
        s->cpu0->env.thumb = 1;
    }

    /* hijack machine option "firmware" to pass command-line parameters */
    /* fixme: better way to expose machine-specific options? */
    QemuOpts *machine_opts = qemu_get_machine_opts();
    const char *options = qemu_opt_get(machine_opts, "firmware");
    
    if (options)
    {
        /* fixme: reinventing the wheel */
        if (strstr(options, "boot=1") || strstr(options, "boot=0"))
        {
            /* change the boot flag */
            uint32_t flag = strstr(options, "boot=1") ? 0xFFFFFFFF : 0;
            printf("Setting BOOTDISK flag to %X\n", flag);
            MEM_WRITE_ROM(s->model->bootflags_addr + 4, (uint8_t*) &flag, 4);
        }
    }
}

void eos_set_mem_w ( EOSState *s, uint32_t addr, uint32_t val )
{
    cpu_physical_memory_write(addr, &val, sizeof(val));
}

void eos_set_mem_h ( EOSState *s, uint32_t addr, uint16_t val )
{
    cpu_physical_memory_write(addr, &val, sizeof(val));
}

void eos_set_mem_b ( EOSState *s, uint32_t addr, uint8_t val )
{
    cpu_physical_memory_write(addr, &val, sizeof(val));
}

uint32_t eos_get_mem_w ( EOSState *s, uint32_t addr )
{
    uint32_t buf;

    cpu_physical_memory_read(addr, &buf, sizeof(buf));

    return buf;
}

uint16_t eos_get_mem_h ( EOSState *s, uint32_t addr )
{
    uint16_t buf;

    cpu_physical_memory_read(addr, &buf, sizeof(buf));

    return buf;
}

uint8_t eos_get_mem_b ( EOSState *s, uint32_t addr )
{
    uint8_t buf;

    cpu_physical_memory_read(addr, &buf, sizeof(buf));

    return buf;
}

static char* get_current_task_name(EOSState *s)
{
    if (!s->model->current_task_addr)
    {
        return 0;
    }
    
    uint32_t current_task_ptr;
    uint32_t current_task[0x50/4];
    static char task_name[100];
    cpu_physical_memory_read(s->model->current_task_addr, &current_task_ptr, 4);
    if (current_task_ptr)
    {
        int off = s->model->current_task_name_offs;
        cpu_physical_memory_read(current_task_ptr, current_task, sizeof(current_task));
        cpu_physical_memory_read(current_task[off], task_name, sizeof(task_name));
        return task_name;
    }
    
    return 0;
}

void io_log(const char * module_name, EOSState *s, unsigned int address, unsigned char type, unsigned int in_value, unsigned int out_value, const char * msg, intptr_t msg_arg1, intptr_t msg_arg2)
{
    /* log I/O when "-d io" is specified on the command line */
    if (!qemu_loglevel_mask(EOS_LOG_IO)) {
        return;
    }

    /* on multicore machines, print CPU index for each message */
    char cpu_name[] = "[CPU0] ";
    if (CPU_NEXT(first_cpu)) {
        cpu_name[4] = '0' + current_cpu->cpu_index;
    } else {
        cpu_name[0] = 0;
    }
    
    unsigned int pc = CURRENT_CPU->env.regs[15];
    unsigned int lr = CURRENT_CPU->env.regs[14];
    if (!module_name) module_name = "???";
    if (!msg) msg = "???";
    
    char* task_name = get_current_task_name(s);
    
    char mod_name[50];
    char mod_name_and_pc[50];
    snprintf(mod_name, sizeof(mod_name), "[%s]", module_name);

    if (task_name)
    {
        /* trim task name or pad with spaces for alignment */
        /* note: task_name size is 100 chars, in get_current_task_name */
        task_name[MAX(5, 15 - (int)strlen(mod_name))] = 0;
        char spaces[] = "           ";
        spaces[MAX(0, 15 - (int)strlen(mod_name) - (int)strlen(task_name))] = 0;
        snprintf(mod_name_and_pc, sizeof(mod_name_and_pc), "%s%s at %s:%08X:%08X", mod_name, spaces, task_name, pc, lr);
    }
    else
    {
        snprintf(mod_name_and_pc, sizeof(mod_name_and_pc), "%-10s at 0x%08X:%08X", mod_name, pc, lr);
    }
    
    /* description may have two optional integer arguments */
    char desc[200];
    snprintf(desc, sizeof(desc), msg, msg_arg1, msg_arg2);
    
    printf("%s%-28s [0x%08X] %s 0x%-8X%s%s\n",
        cpu_name,
        mod_name_and_pc,
        address,
        type & MODE_WRITE ? "<-" : "->",
        type & MODE_WRITE ? in_value : out_value,
        strlen(msg) ? ": " : "",
        desc
    );
}






/** HANDLES **/




unsigned int eos_default_handle ( EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int data = 0;

    if ( type & MODE_WRITE )
        eos_set_mem_w ( s, address, value );
    else
        data = eos_get_mem_w ( s, address );

    /* do not log ram/flash access */
    if(((address & 0xF0000000) == 0) || ((address & 0xF0000000) == 0xF0000000) || ((address & 0xF0000000) == 0x40000000))
    {
        return data;
    }

    if ( type & MODE_WRITE )
    {
        if(s->verbosity & 1)
        {
            io_log("MEM", s, address, type, value, 0, "", 0, 0);
        }
    }
    else
    {
        static int mod = 0;
        mod++;
        mod %= 2;

        if(mod)
        {
            data = ~data;
        }
        if(s->verbosity & 1)
        {
            io_log("MEM", s, address, type, 0, data, "", 0, 0);
        }
    }
    return data;
}

EOSRegionHandler *eos_find_handler( unsigned int address)
{
    int pos = 0;
    for(pos = 0; pos < sizeof(eos_handlers) / sizeof(eos_handlers[0]); pos++)
    {
        if(eos_handlers[pos].start <= address && eos_handlers[pos].end >= address)
        {
            return &eos_handlers[pos];
        }
    }

    return NULL;
}

unsigned int eos_handler ( EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    EOSRegionHandler *handler = eos_find_handler(address);

    if(handler)
    {
        return handler->handle(handler->parm, s, address, type, value);
    }
    else
    {
        static uint32_t last_addr = 0;
        static uint32_t repeats = 0;

        if(address != last_addr || repeats < 5)
        {
            if(address == last_addr)
            {
                repeats++;
            }
            else
            {
                last_addr = address;
                repeats = 0;
            }

            io_log("*unk*", s, address, type, value, 0, 0, 0, 0);
        }
    }
    return 0;
}

unsigned int eos_trigger_int(EOSState *s, unsigned int id, unsigned int delay)
{
    qemu_mutex_lock(&s->irq_lock);

    if(!delay && s->irq_enabled[id] && !s->irq_id)
    {
        if (qemu_loglevel_mask(CPU_LOG_INT)) {
            printf("[EOS] trigger int 0x%02X\n", id);
        }
        s->irq_id = id;
        s->irq_enabled[s->irq_id] = 0;
        /* fixme: CURRENT_CPU not valid outside MMIO handlers */
        cpu_interrupt(CPU(s->cpu0), CPU_INTERRUPT_HARD);
    }
    else
    {
        if (qemu_loglevel_mask(CPU_LOG_INT)) {
            printf("[EOS] trigger int 0x%02X (delayed!)\n", id);
        }
        if(!s->irq_enabled[id])
        {
            delay = 1;
        }
        s->irq_schedule[id] = MAX(delay, 1);
    }

    qemu_mutex_unlock(&s->irq_lock);
    return 0;
}

/* this appears to be an older interface for the same interrupt controller */
unsigned int eos_handle_intengine_vx ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    int msg_arg1 = 0;
    int msg_arg2 = 0;
    unsigned int ret = 0;

    switch(address & 0xF)
    {
        case 0xC:
            {
                msg = "Reset interrupts %Xh (%Xh)";
                msg_arg1 = value;

                for (int i = 0; i < 32; i++)
                {
                    if (value & (1<<i))
                    {
                        msg_arg2 = ((address & 0xF0) >> 1) + i;
                        if (msg_arg2 < COUNT(s->irq_enabled))
                        {
                            s->irq_enabled[msg_arg2] = 0;
                        }
                        cpu_reset_interrupt(CPU(CURRENT_CPU), CPU_INTERRUPT_HARD);
                    }
                }
            }
            break;

        case 0x8:
            if(type & MODE_WRITE)
            {
                msg = "Enabled interrupts %Xh (%Xh)";
                msg_arg1 = value;

                for (int i = 0; i < 32; i++)
                {
                    if (value & (1<<i))
                    {
                        msg_arg2 = ((address & 0xF0) >> 1) + i;
                        if (msg_arg2 < COUNT(s->irq_enabled))
                        {
                            s->irq_enabled[msg_arg2] = 1;
                        }
                    }
                }
            }
            break;
    }

    if (qemu_loglevel_mask(CPU_LOG_INT))
    {
        io_log("INTvx", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    }
    return ret;
}

unsigned int eos_handle_intengine ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    int msg_arg1 = 0;
    int msg_arg2 = 0;
    unsigned int ret = 0;

    switch(address)
    {
        case 0xC0201000:    /* DIGIC 2,3 */
        case 0xC0201004:    /* DIGIC 4,5 (returns irq_id << 2) */
        case 0xD4011000:    /* DIGIC 6 */
        case 0xD02C0290:    /* 5D3 EEKO */
            if(type & MODE_WRITE)
            {
                msg = "Wrote int reason ???";
            }
            else
            {
                msg = "Requested int reason %x (INT %02Xh)";
                msg_arg1 = s->irq_id << 2;
                msg_arg2 = s->irq_id;
                ret = s->irq_id << ((address & 0xF) ? 2 : 0);
                
                if (s->model->digic_version > 3)
                {
                    /* 1000D doesn't like this... */
                    assert(ret);
                }

                /* this register resets on read (subsequent reads should report 0) */
                s->irq_id = 0;
                cpu_reset_interrupt(CPU(CURRENT_CPU), CPU_INTERRUPT_HARD);

                if(msg_arg2 == TIMER_INTERRUPT)
                {
                    /* timer interrupt, quiet */
                    return ret;
                }
            }
            break;

        case 0xC0201010:    /* DIGIC <= 5 */
        case 0xD4011010:    /* DIGIC 6 */
        case 0xD02C029C:    /* 5D3 EEKO */
            if(type & MODE_WRITE)
            {
                msg = "Enabled interrupt %02Xh";
                msg_arg1 = value;
                s->irq_enabled[value] = 1;

                /* we shouldn't reset s->irq_id here (we already reset it on read) */
                /* if we reset it here also, it will trigger interrupt 0 incorrectly (on race conditions) */

                if (value == TIMER_INTERRUPT)
                {
                    /* timer interrupt, quiet */
                    return 0;
                }
            }
            else
            {
                /* DIGIC 6: interrupt handler reads this register after writing */
                /* value seems unused */
                return 0;
            }
            break;

        case 0xC0201200:    /* DIGIC <= 5 */
        case 0xD4011200:    /* DIGIC 6 */
        case 0xD02C02CC:    /* 5D3 EEKO */
            if(type & MODE_WRITE)
            {
                if (value)
                {
                    msg = "Reset IRQ?";
                    s->irq_id = 0;
                }
            }
            else
            {
                msg = "Read after enabling interrupts";
            }
            break;
    }

    if (qemu_loglevel_mask(CPU_LOG_INT))
    {
        io_log("INT", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    }
    return ret;
}

/* Private memory region for Cortex A9, used in EOS M5 */
/* http://www.csc.lsu.edu/~whaley/teach/FHPO_F11/ARM/CortAProgGuide.pdf#G26.1058874 */
/* fixme: reuse QEMU implementation from intc/arm_gic.c */
unsigned int eos_handle_intengine_gic ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * module = "PRIV";
    const char * msg = 0;
    int msg_arg1 = 0;
    int msg_arg2 = 0;
    unsigned int ret = 0;

    static int enabled[32] = {0};
    static int target[1024] = {0};

    switch (address & 0xFFFF)
    {
        /* Snoop Control Unit (SCU) */
        case 0x0000 ... 0x00FF:
        {
            module = "SCU";
            break;
        }

        /* Interrupt Controller CPU Interface */
        case 0x0100 ... 0x01FF:
        {
            module = "GICC";
            switch (address & 0xFF)
            {
                case 0x0C:
                {
                    msg = "GICC_IAR";
                    ret = 0x20;
                    break;
                }
            }
            break;
        }

        /* Interrupt Controller Distributor */
        case 0x1000 ... 0x1FFF:
        {
            module = "GICD";
            switch (address & 0xFFF)
            {
                case 0x100 ... 0x17C:
                {
                    msg = "GICD_ISENABLER%d (1C0+%02Xh)";
                    int word = ((address & 0xFFF) - 0x100) / 4;
                    msg_arg1 = word;
                    msg_arg2 = word * 32;
                    assert(word < COUNT(enabled));
                    MMIO_VAR(enabled[word]);
                    break;
                }

                case 0x180 ... 0x1FC:
                {
                    msg = "GICD_ICENABLER%d (1C0+%02Xh)";
                    int word = ((address & 0xFFF) - 0x180) / 4;
                    msg_arg1 = word;
                    msg_arg2 = word * 32;
                    assert(word < COUNT(enabled));
                    if(type & MODE_WRITE) {
                        enabled[word] &= ~value;
                    }
                    break;
                }

                case 0x800 ... 0x880:
                {
                    msg = "GICD_ITARGETSR%d (1C0+%02Xh)";
                    int id = ((address & 0xFFFF) - 0x1800);
                    msg_arg1 = id;
                    msg_arg2 = id;
                    MMIO_VAR(target[id]);
                    break;
                }
            }
            break;
        }
    }

    if (qemu_loglevel_mask(CPU_LOG_INT))
    {
        io_log(module, s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    }
    return ret;
}

unsigned int eos_handle_timers_ ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int pc = CURRENT_CPU->env.regs[15];

    if(type & MODE_WRITE)
    {
        printf("[Timer?] at [0x%08X] [0x%08X] -> [0x%08X]\r\n", pc, value, address);
    }
    else
    {
        return 0;
    }
    return 0;
}

unsigned int eos_handle_timers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;
    int msg_arg1 = 0;
    int msg_arg2 = 0;

    int timer_id = 
        (parm == 0) ? ((address & 0xF00) >> 8)     :    /* DIGIC 4/5 timers (0,1,2)*/
        (parm == 1) ? ((address & 0xFC0) >> 6) - 6 :    /* DIGIC 6 timers (3,4,5,6,7,8,9,10)*/
        (parm == 2) ? 11 :                              /* 5D3 Eeko DryOS timer */
                      0  ;
    
    msg_arg1 = timer_id;
    
    if (timer_id < COUNT(s->timer_enabled))
    {
        switch(address & 0x1F)
        {
            case 0x00:
                if(type & MODE_WRITE)
                {
                    if(value & 1)
                    {
                        if (timer_id == DRYOS_TIMER_ID)
                        {
                            msg = "Timer #%d: starting triggering";
                            eos_trigger_int(s, TIMER_INTERRUPT, s->timer_reload_value[timer_id] >> 8);   /* digic timer */
                        }
                        else
                        {
                            msg = "Timer #%d: starting";
                        }

                        s->timer_enabled[timer_id] = 1;
                    }
                    else
                    {
                        msg = "Timer #%d: stopped";
                        s->timer_enabled[timer_id] = 0;
                        s->timer_current_value[timer_id] = 0;
                    }
                }
                else
                {
                    msg = "Timer #%d: ready";
                }
                break;
            
            case 0x08:
                MMIO_VAR(s->timer_reload_value[timer_id]);

                if(type & MODE_WRITE)
                {
                    msg = "Timer #%d: will trigger after %d ms";
                    msg_arg2 = ((uint64_t)value + 1) / 1000;
                }
                break;
            
            case 0x0C:
                msg = "Timer #%d: current value";
                ret = s->timer_current_value[timer_id];
                break;
            
            case 0x10:
                if(type & MODE_WRITE)
                {
                    msg = "Timer #%d: interrupt enable?";
                }
                break;
        }
    }

    io_log("TIMER", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

unsigned int eos_handle_hptimer ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    /* not sure if it's really needed, but... just in case */
    qemu_mutex_lock(&s->irq_lock);

    const char * msg = 0;
    int msg_arg1 = 0;
    int msg_arg2 = 0;
    
    unsigned int ret = 0;
    int timer_id = (address & 0x0F0) >> 4;
    msg_arg1 = timer_id;

    switch(address & 0xF0F)
    {
        case 0x100:
            MMIO_VAR(s->HPTimers[timer_id].active);

            if(type & MODE_WRITE)
            {
                msg = value == 1 ? "HPTimer #%d: active" :
                      value == 0 ? "HPTimer #%d: inactive" :
                                   "???";
            }
            else
            {
                msg = "HPTimer #%d: status?";
            }
            break;

        case 0x104:
            if(type & MODE_WRITE)
            {
                /* upper rounding, to test for equality with digic_timer */
                int rounded = (value + 0x100) & 0xFFF00;
                int old = s->HPTimers[timer_id].output_compare;
                s->HPTimers[timer_id].output_compare = rounded;
                
                /* for some reason, the value set to output compare
                 * is sometimes a little behind digic_timer */
                int delay_since_last = ((int32_t)(value - old) << 12) >> 12;
                int actual_delay = ((int32_t)(rounded - s->digic_timer) << 12) >> 12;

                if (actual_delay < 0)
                {
                    /* workaround: when this happens, trigger right away */
                    s->HPTimers[timer_id].output_compare = s->digic_timer + 0x100;
                }
                
                if (old)
                {
                    msg = "HPTimer #%d: output compare (%d microseconds since last)";
                    msg_arg2 = delay_since_last;
                }
                else
                {
                    msg = "HPTimer #%d: output compare (delay %d microseconds)";
                    msg_arg2 = actual_delay;
                }
            }
            else
            {
                ret = s->HPTimers[timer_id].output_compare;
                msg = "HPTimer #%d: output compare";
            }
            break;

        case 0x200:
            msg = "HPTimer #%d: ?!";
            break;

        case 0x204:
            msg = "HPTimer #%d: ???";
            if(type & MODE_WRITE)
            {
                msg = "HPTimer #%d: reset trigger?";
                s->HPTimers[timer_id].triggered = 0;
            }
            break;
        
        case 0x300:
            if(type & MODE_WRITE)
            {
                msg = "?!";
            }
            else
            {
                ret = 0;
                int i;
                for (i = 0; i < 8; i++)
                    if (s->HPTimers[6+i].triggered)
                        ret |= 1 << (2*i+4);
                
                msg = "Which timer(s) triggered";
            }
            break;
    }

    io_log("HPTimer", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    qemu_mutex_unlock(&s->irq_lock);
    return ret;
}


// 100D Set_AVS
static
unsigned int avs_handle(EOSState *s, int address, int type, int val)
{
    // Actual values from a live 100D, possibly reads from an ADC and 
    // the voltage levels set by some voltage supply. If the wrong 
    // values are used there will be a divide-by-zero error in Canon
    // firmware, resulting in assert(0) @ Stub.c.
    const uint32_t avs_reply[][3] = {
        { 0x000C00, 0x200400, 0xE8D3 },
        { 0x000C00, 0x300000, 0x00AA },
        { 0x100800, 0x200400, 0xBC94 },
        { 0x100800, 0x300000, 0x0099 },
    };
    static int regA = 0, regB = 0;
    unsigned int ret = 0;
    const char * msg = "unknown";

    if (type & MODE_WRITE) {
        switch (address & 0xFFFF) {
            case 0xC288:
                msg = "reg A";
                regA = val;
                break;
            case 0xC28C:
                msg = "reg B";
                regB = val;
                break;
        }
    } else {
        switch (address & 0xFFFF) {
            case 0xF498:
                for (int i = 0; i < sizeof(avs_reply)/sizeof(avs_reply[0]); i++) {
                    if (regA == avs_reply[i][0] && regB == avs_reply[i][1]) {
                        ret = avs_reply[i][2];
                        msg = "pattern match!";
                        regA = 0; regB = 0;
                        break;
                    }
                }
                break;
        }
    }
    io_log("AVS", s, address, type, val, ret, msg, 0, 0);
    return ret;
}

static int eos_handle_card_led( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = "Card LED";
    unsigned int ret = 0;
    static int stored_value = 0;
    
    MMIO_VAR(stored_value);

    if (type & MODE_WRITE)
    {
        if (s->model->digic_version == 6)
        {
            s->card_led = 
                ((value & 0x0F000F) == 0x0D0002) ?  1 :
                ((value & 0x0F000F) == 0x0C0003) ? -1 :
                (value == 0x8A0075)              ? -1 : 0;
        }
        else
        {
            s->card_led = 
                (value == 0x46 || value == 0x138800
                               || value == 0x93D800) ?  1 :
                (value == 0x44 || value == 0x838C00 ||
                 value == 0x40 || value == 0x038C00
                               || value == 0x83DC00
                               || value == 0xE000000) ? -1 : 0;
        }
        
        /* this will trigger if somebody writes an invalid LED ON/OFF code */
        assert (s->card_led);
    }
    
    io_log("GPIO", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_gpio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 1;
    const char * msg = 0;
    const char * msg_lookup = 0;
    static int unk = 0;

    /* 0xC022009C/BC/C06C/D06C, depending on camera model */
    if (address == s->model->mpu_request_register)
    {
        return eos_handle_mpu(parm, s, address, type, value);
    }

    /* 0xC0220134/BC/6C/C188/C184, depending on model */
    if (address == s->model->card_led_address)
    {
        return eos_handle_card_led(parm, s, address, type, value);
    }

    switch (address & 0xFFFF)
    {
        case 0xCB6C: /* 5D3/6D expect this one to be 0x10 in bootloader (6D:FFFF0544) */
            msg = "5D3/6D expected to be 0x10";
            ret = 0x10;
            break;
        
        case 0xFA04:
            msg = "6D expected to be 0";
            ret = 0;
            break;

        case 0xF100:
            if(type & MODE_WRITE)
            {
            }
            else
            {
                ret = unk;
                unk++;
                unk %= 2;
            }
            break;

        case 0xF198:
            if(type & MODE_WRITE)
            {
            }
            else
            {
                ret = unk;
                unk++;
                unk %= 2;
            }
            break;

        /*
        0xC022F480 [32]  Other VSW Status
           0x40000 /VSW_OPEN Hi
           0x80000 /VSW_REVO Hi
        */
        case 0xF480:
            if(type & MODE_WRITE)
            {
            }
            else
            {
                if (strcmp(s->model->name, "5D2") == 0 ||
                    strcmp(s->model->name, "50D") == 0)
                {
                    ret = 0x6000;
                    msg = "VSW_STATUS 5D2/50D";
                }
                else
                {
                    ret = 0x40000 | 0x80000;
                    msg = "VSW_STATUS";
                }
            }
#ifdef IGNORE_CONNECT_POLL
            return ret;
#endif
            break;

        case 0xF48C:
            /* 6D: return -1 here to launch "System & Display Check & Adjustment program" */
            msg = "70D/6D SD detect?";
            ret = 0x10C;
            break;
        
        case 0x019C: /* 5D3: return 1 to launch "System & Display Check & Adjustment program" */
        case 0x0080: /* same for 1000D */
            msg = "System check";
            ret = 0;
            break;

        case 0x00DC:
            msg = "abort situation for FROMUTIL on 600D";
            ret = 0;
            break;

        case 0x00B0:
            msg = "FUNC SW OFF on 7D";
            ret = 0;
            break;
            
        case 0x0024:
            msg = "master woke up on 7D";
            ret = 0;
            break;
        
        case 0x0108:
            /* ERASE SW OFF on 600D */
            msg = "ERASE SW OFF";
            ret = 1;
#ifdef IGNORE_CONNECT_POLL
            return ret;
#endif
            break;

        case 0x010C:
            msg = "something from hotplug task on 60D";
            ret = 1;
#ifdef IGNORE_CONNECT_POLL
            return ret;
#endif
            break;

        case 0x012C:
            msg = "1000D display";
            ret = rand();
            break;
    
        case 0x0034:
            if (s->model->digic_version < 4)
            {
                msg = "400D init";
                ret = rand();
                break;
            }
            else
            {
                /* USB on 600D */
                msg = "600D USB CONNECT";
                ret = 1;
#ifdef IGNORE_CONNECT_POLL
                return ret;
#endif
                break;
            }

        case 0x014:
            /* /VSW_ON on 600D */
            msg = "/VSW_ON";
            ret = 0;
            break;

        case 0xC0D4:
            /* Serial flash on 100D */
            msg = "SPI";
            if (s->sf)
                serial_flash_set_CS(s->sf, (value & 0x100000) ? 1 : 0);
            if (value == 0x83DC00 || value == 0x93D800)
                return 0; // Quiet
            ret = 0;
            break;
        
        case 0x002C:
            /* Serial flash on 70D */
            msg = "SPI";
            if (s->sf)
                serial_flash_set_CS(s->sf, (value & 0x2) ? 1 : 0);
            if (value == 0x46 || value == 0x44)
                return 0; // Quiet
            ret = 0;
            break;

        case 0xC020: 
            /* CS for RTC on 100D */
            if(type & MODE_WRITE)
            {
                if((value & 0x0100000) == 0x100000)
                {
                    msg = "[RTC] CS set";
                    s->rtc.transfer_format = 0xFF;
                }
                else
                {
                    msg = "[RTC] CS reset";
                }
            }
            ret = 0;
            break;
//          eos_spi_rtc_handle(2,  (value & 0x100000) ? 1 : 0);
//          msg = (value & 0x100000) ? "[RTC] CS set" : "[RTC] CS reset";
//            if (value == 0x83DC00 || value == 0x93D800)
//                return 0; // Quiet
//          ret = 0;
//          break;

        case 0x0128:
            /* CS for RTC on 600D */
            if(type & MODE_WRITE)
            {
                if((value & 0x06) == 0x06)
                {
                    msg = "[RTC] CS set";
                    s->rtc.transfer_format = 0xFF;
                }
                else
                {
                    msg = "[RTC] CS reset";
                }
            }
            ret = 0;
            break;

        case 0x0098:
        {
            static int last_value = 1;
            MMIO_VAR(last_value);
            msg = (value & 0x02) ? "SRM_SetBusy" 
                                 : "SRM_ClearBusy" ;
            break;
        }

        case 0x0168:
            msg = "70D write protect";
            ret = 0;
            break;
        
        case 0x301C:    /* 40D, 5D2 */
        case 0x3020:    /* 5D3 */
            /* set low => CF present */
            msg = "CF detect";
            ret = 0;
            break;


        /* 100D */
        //case 0xC0DC: // [0xC022C0DC] <- 0x83DC00  : GPIO_12
        case 0xC0E0:   // [0xC022C0E0] <- 0xA3D400  : GPIO_13
            if ((type & MODE_WRITE) && value == 0xA3D400) {
                msg = "100D Serial flash DMA start?";
                ret = 0;
            }

            break;

        case 0x0070:    /* 600D, 60D */
        case 0x0164:
        case 0x0174:    /* 5D3 */
            msg = "VIDEO CONNECT";
            ret = 1;
#ifdef IGNORE_CONNECT_POLL
            return ret;
#endif
            break;

        case 0x00E8:    /* 600D, 60D */
        case 0x0160:
        case 0x016C:    /* 5D3 */
        case 0x0134:    /* EOSM */
            msg = "MIC CONNECT";
            ret = 1;
#ifdef IGNORE_CONNECT_POLL
            return ret;
#endif
            break;
        
        case 0x015C:
        case 0x017C:    /* 5D3 */
        case 0x0130:    /* EOSM */
            msg = "USB CONNECT";
            ret = 0;
#ifdef IGNORE_CONNECT_POLL
            return ret;
#endif
            break;
        
        case 0x0124:    /* 100D? */
        case 0x0150:    /* 5D3 */
            msg = "HDMI CONNECT";
            ret = 0;
#ifdef IGNORE_CONNECT_POLL
            return ret;
#endif
            break;

        case 0x0138:
            if (s->model->digic_version == 5) {
                msg = "VIDEO CONNECT";      /* EOSM; likely other D5 models */
                ret = 1;                    /* negative logic */
            } else {
                msg = "HDMI CONNECT";       /* 600D; likely other D4 models */
                ret = 0;
            }
#ifdef IGNORE_CONNECT_POLL
            return ret;
#endif
            break;

        case 0x320C:
            msg = "Eeko WakeUp";
            if (type & MODE_WRITE)
            {
                if (value == 7)
                {
                    eos_trigger_int(s, 0x111, 0);
                }
            }
            break;

        // 100D Set_AVS
        case 0xC288:
        case 0xC28C:
        case 0xF498:
            return avs_handle(s, address, type, value);
    }

    msg_lookup = get_bufcon_label(bufcon_label_100D, address);
    if (msg_lookup != NULL && msg != NULL)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "%s (%s)", msg_lookup, msg);
        io_log("GPIO", s, address, type, value, ret, tmp, 0, 0);
    }
    else
    {
        if (msg == NULL)
            msg = msg_lookup;
        io_log("GPIO", s, address, type, value, ret, msg, 0, 0);
    }
    return ret;
}

unsigned int eos_handle_ram ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    int ret = eos_default_handle ( s, address, type, value );

    /* not tested; appears unused */
    io_log("RAM", s, address, type, value, ret, 0, 0, 0);

    return ret;
}

unsigned int eos_handle_cartridge ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    io_log("Cartridge", s, address, type, value, 0, 0, 0, 0);
    return 0;
}

static void edmac_trigger_interrupt(EOSState* s, int channel)
{
    /* from register_interrupt calls */
    const int edmac_interrupts[] = {
        0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x6D, 0xC0, 0x00, /* write channels 0..6, one unused position */
        0x5D, 0x5E, 0x5F, 0x6E, 0xC1, 0xC8, 0x00, 0x00, /* read channels 0..5, two unused positions */
        0xF9, 0x83, 0x8A, 0xCA, 0xCB, 0xD2, 0xD3, 0x00, /* write channels 7..13, one unused position */
        0x8B, 0x92, 0xE2, 0x95, 0x96, 0x97, 0x00, 0x00, /* read channels 6..11, two unused positions */
        0xDA, 0xDB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* write channels 14..15, 6 unused positions */
        0x9D, 0x9E, 0x9F, 0xA5, 0x00, 0x00, 0x00, 0x00, /* read channels 12..15, 4 unused positions */
    };

#if 0
    for (int i = 0; i < COUNT(edmac_interrupts); i++)
    {
        int isr = edmac_interrupts[i];
        if (isr)
        {
            printf("    [0x%02X] = \"EDMAC#%d\",\n", isr, i);
        }
    }
    exit(1);
#endif

    assert(channel >= 0 && channel < COUNT(edmac_interrupts));
    assert(edmac_interrupts[channel]);
    
    eos_trigger_int(s, edmac_interrupts[channel], 0);
}

static int edmac_fix_off1(EOSState *s, int32_t off)
{
    /* the value is signed, but the number of bits is model-dependent */
    int off1_bits = (s->model->digic_version <= 4) ? 17 : 
                    (s->model->digic_version == 5) ? 19 : 0;
    assert(off1_bits);
    return off << (32-off1_bits) >> (32-off1_bits);
}

static char * edmac_format_size_3(
    int x, int off
)
{
    static char buf[32];
    snprintf(buf, sizeof(buf),
        off ? "%d, skip %d" : x ? "%d" : "",
        x, off
    );
    return buf;
}

static char * edmac_format_size_2(
    int y, int x,
    int off1, int off2
)
{
    static char buf[64]; buf[0] = 0;
    if (off1 == off2)
    {
        char * inner = edmac_format_size_3(x, off1);
        snprintf(buf, sizeof(buf),
            y == 0 ? "%s" : strchr(inner, ' ') ? "(%s) x %d" : "%sx%d",
            inner, y+1
        );
    }
    else
    {
        /* y may be executed never, once or many times */
        if (y)
        {
            char * inner1 = edmac_format_size_3(x, off1);
            snprintf(buf, sizeof(buf),
                y == 0 ? "%s" : strchr(inner1, ' ') ? "(%s) x %d" : "%sx%d",
                inner1, y
            );
        }
        
        /* y is executed once */
        char * inner2 = edmac_format_size_3(x, off2);
        STR_APPEND(buf, "%s%s", buf[0] && inner2[0] ? ", " : "", inner2);
    }
    return buf;
}

static char * edmac_format_size_1(
    int y, int xn, int xa, int xb,
    int off1a, int off1b, int off2, int off3
)
{
    static char buf[128]; buf[0] = 0;
    if (xa == xb && off1a == off1b && off2 == off3)
    {
        char * inner = edmac_format_size_2(y, xa, off1a, off2);
        snprintf(buf, sizeof(buf),
            xn == 0 ? "%s" : strchr(inner, ' ') ? "(%s) x %d" : "%sx%d",
            inner, xn+1
        );
    }
    else
    {
        /* xa may be executed never, once or many times */
        if (xn)
        {
            char * inner1 = edmac_format_size_2(y, xa, off1a, off2);
            snprintf(buf, sizeof(buf),
                xn == 1 ? "%s" : strchr(inner1, ' ') ? "(%s) x %d" : "%sx%d",
                inner1, xn
            );
        }
        
        /* xb is executed once */
        char * inner2 = edmac_format_size_2(y, xb, off1b, off3);
        STR_APPEND(buf, "%s%s",
            !(buf[0] && inner2[0]) ? "" :   /* no separator needed */
            strlen(buf) > 20 && strlen(inner2) > 20 ? ",\n  " : ", ",   /* newline for long strings */
            inner2
        );
    }
    return buf;
}

static char * edmac_format_size(
    int yn, int ya, int yb, int xn, int xa, int xb,
    int off1a, int off1b, int off2a, int off2b, int off3
)
{
#if 0
    const char * names[] = { "yn", "ya", "yb", "xn", "xa", "xb", "off1a", "off1b", "off2a", "off2b", "off3" };
    int values[] = { yn, ya, yb, xn, xa, xb, off1a, off1b, off2a, off2b, off3 };
    int len = 0;
    for (int i = 0; i < COUNT(values); i++)
        if (values[i])
            len += printf("%s=%d, ", names[i], values[i]);
    printf("\b\b: ");
    for (int i = 0; i < 45 - len; i++)
        printf(" ");
    if (len > 45)
        printf("\n  ");
#endif

    static char buf[256]; buf[0] = 0;
    
    if (ya == yb && off2a == off2b)
    {
        char * inner = edmac_format_size_1(ya, xn, xa, xb, off1a, off1b, off2a, off3);
        snprintf(buf, sizeof(buf),
            yn == 0 ? "%s" : strchr(inner, ' ') ? "(%s) x %d" : "%sx%d",
            inner, yn+1
        );
    }
    else
    {
        /* ya may be executed never, once or many times */
        if (yn)
        {
            char * inner1 = edmac_format_size_1(ya, xn, xa, xb, off1a, off1b, off2a, off3);
            snprintf(buf, sizeof(buf),
                yn == 1 ? "%s" : strchr(inner1, ' ') ? "(%s) x %d" : "%sx%d",
                inner1, yn
            );
        }
        
        /* yb is executed once */
        /* setting the last offset to off1b usually simplifies the formula */
        char * inner2 = edmac_format_size_1(yb, xn, xa, xb, off1a, off1b, off2b, off3 ? off3 : off1b);
        STR_APPEND(buf, "%s%s",
            !(buf[0] && inner2[0]) ? "" :   /* no separator needed */
            strlen(buf) > 20 && strlen(inner2) > 20 ? ",\n  " : ", ",   /* newline for long strings */
            inner2
        );
    }
    return buf;
}

static void edmac_test_format_size(void)
{
    return;

    printf("EDMAC format tests:\n");
    printf("%s\n", edmac_format_size(0, 0, 0x1df, 0, 0, 0x2d0,      0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(0, 0, 0x1df, 0, 0, 0x2d0,      0, 100, 0, 0, 0));
    printf("%s\n", edmac_format_size(0, 0, 0, 0x1df, 0x2d0, 0x2d0,  0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(0, 0, 0, 0x1000, 0x1000, 0,    0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(0, 0, 0, 0x12, 0x1000, 0xC00,  0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(0, 0, 0xfff, 0x7, 0x20, 0x20,  0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(0, 0, 0xb3f, 0x2, 0xf0, 0xf0,  0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(0, 0, 0, 1055, 3276, 32,       0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(0, 0, 10, 95, 3276, 3276,      0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(3, 0,  7, 95, 3276, 3276,      0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(33,0, 62, 10, 3276, 3276,      0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(5, 2,  3, 10, 3276, 3276,      0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(3, 6,  5, 10, 3276, 3276,      0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(3, 7,  5, 10, 3276, 3276,      0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(8, 9,  7, 10, 3276, 3276,      0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(3, 28, 8, 10, 3276, 3276,      0, 0, 0, 0, 0));
    printf("%s\n", edmac_format_size(3, 28, 8, 10, 3276, 3276,      0, 100, 0, 0, 0));
    printf("%s\n", edmac_format_size(3, 28, 8, 10, 3276, 3276,      44, 100, 0, 0, 0));
    printf("%s\n", edmac_format_size(3, 28, 8, 10, 3276, 1638,      44, 100, 172, 196, 1236));
    printf("%s\n", edmac_format_size(0, 0, 3839, 2, 768, 768,       0x2a00, 0x2a00, 0, 0xfd5d2d00, 0));
    printf("%s\n", edmac_format_size(137, 7, 7, 15, 320, 320,      -320,-320,-320,-320,-320));
    printf("%s\n", edmac_format_size(479, 0, 0, 9, 40, 40,          360, 360, 32, 32, 32));
    exit(1);
}

/* 1 on success, 0 = no data available yet (should retry) */
static int edmac_do_transfer(EOSState *s, int channel)
{
    /* not fully implemented */
    printf("[EDMAC#%d] Starting transfer %s 0x%X %s conn", channel,
        (channel & 8) ? "from" : "to",
        s->edmac.ch[channel].addr,
        (channel & 8) ? "to" : "from"
    );
    
    uint32_t conn = 0;
    
    if (channel & 8)
    {
        /* read channel */
        for (int c = 0; c < COUNT(s->edmac.read_conn); c++)
        {
            if (s->edmac.read_conn[c] == channel)
            {
                /* can a read operation have multiple destinations? */
                /* if yes, we don't handle that case yet */
                assert(conn == 0);
                conn = c;
            }
        }
    }
    else
    {
        conn = s->edmac.write_conn[channel];
    }
    
    printf(" #%d, ", conn);

    /* Hypothesis
     * ==========
     * 
     * (
     *    ((xa, skip off1a) * ya, xa, skip off2a) * xn
     *     (xb, skip off1b) * ya, xb, skip off3
     * ) * yn,
     * 
     * (
     *    ((xa, skip off1a) * yb, xa, skip off2b) * xn
     *     (xb, skip off1b) * yb, xb, skip off3
     * )
     * 
     */

    int xa = s->edmac.ch[channel].xa;
    int ya = s->edmac.ch[channel].ya;
    int xb = s->edmac.ch[channel].xb;
    int yb = s->edmac.ch[channel].yb;
    int xn = s->edmac.ch[channel].xn;
    int yn = s->edmac.ch[channel].yn;
    int off1a = edmac_fix_off1(s, s->edmac.ch[channel].off1a);
    int off1b = edmac_fix_off1(s, s->edmac.ch[channel].off1b);
    int off2a = s->edmac.ch[channel].off2a;
    int off2b = s->edmac.ch[channel].off2b;
    int off3  = s->edmac.ch[channel].off3;
    int flags = s->edmac.ch[channel].flags;
    
    printf("%s, ", edmac_format_size(yn, ya, yb, xn, xa, xb, off1a, off1b, off2a, off2b, off3));

    printf("flags=0x%X\n", flags);

    /* actual amount of data transferred */
    uint32_t transfer_data_size =
        (xa * (ya+1) * xn + xb * (ya+1)) * yn +
        (xa * (yb+1) * xn + xb * (yb+1));
    
    /* total size covered, including offsets */
    uint32_t transfer_data_skip_size =
        (((xa + off1a) * ya + xa + off2a) * xn +
         ((xb + off1b) * ya + xb + off3)) * yn +
        (((xa + off1a) * yb + xa + off2b) * xn +
          (xb + off1b) * yb + xb + off3);

    if (channel & 8)
    {
        /* from memory to image processing modules */
        uint32_t src = s->edmac.ch[channel].addr;
        
        /* repeated transfers will append to existing buffer */
        uint32_t old_size = s->edmac.conn_data[conn].data_size;
        uint32_t new_size = old_size + transfer_data_size;
        if (s->edmac.conn_data[conn].buf)
        {
            printf("[EDMAC] conn #%d: data size %d -> %d.\n",
                conn, old_size, new_size
            );
        }
        s->edmac.conn_data[conn].buf = realloc(s->edmac.conn_data[conn].buf, new_size );
        void * dst = s->edmac.conn_data[conn].buf + old_size;
        s->edmac.conn_data[conn].data_size = new_size;
        
        for (int jn = 0; jn <= yn; jn++)
        {
            int y     = (jn < yn) ? ya    : yb;
            int off2  = (jn < yn) ? off2a : off2b;
            for (int in = 0; in <= xn; in++)
            {
                int x     = (in < xn) ? xa    : xb;
                int off1  = (in < xn) ? off1a : off1b;
                int off23 = (in < xn) ? off2  : off3;
                for (int j = 0; j <= y; j++)
                {
                    int off = (j < y) ? off1 : off23;
                    cpu_physical_memory_read(src, dst, x);
                    src += x + off;
                    dst += x;
                }
            }
        }
        printf("[EDMAC#%d] %d bytes read from %X-%X.\n", channel, transfer_data_size, s->edmac.ch[channel].addr, s->edmac.ch[channel].addr + transfer_data_skip_size);
    }
    else
    {
        /* from image processing modules to memory */
        uint32_t dst = s->edmac.ch[channel].addr;
        
        if (conn == 0 || conn == 35)
        {
            /* sensor data? */
            s->edmac.conn_data[conn].buf = malloc(transfer_data_size);
            s->edmac.conn_data[conn].data_size = transfer_data_size;

            /* todo: autodetect all DNG files and cycle between them */
            char filename[1024];
            snprintf(filename, sizeof(filename), "%s/%s/VRAM/PH-QR/RAW-000.DNG", s->workdir, s->model->name);
            FILE* f = fopen(filename, "rb");
            if (f)
            {
                printf("Loading photo raw data from %s...\n", filename);
                /* fixme: hardcoded DNG offset */
                fseek(f, 33792, SEEK_SET);
                assert(fread(s->edmac.conn_data[conn].buf, 1, transfer_data_size, f) == transfer_data_size);
                fclose(f);
                reverse_bytes_order(s->edmac.conn_data[conn].buf, transfer_data_size);
            }
            else
            {
                printf("%s not found; generating random noise\n", filename);
                for (int i = 0; i < transfer_data_size; i++)
                {
                    ((uint8_t*)s->edmac.conn_data[conn].buf)[i] = rand();
                }
            }
        }
        
        if (s->edmac.conn_data[conn].data_size < transfer_data_size)
        {
            printf("[EDMAC#%d] Data %s; will try again later.\n", channel,
                s->edmac.conn_data[conn].data_size ? "incomplete" : "unavailable"
            );
            return 0;
        }
        assert(s->edmac.conn_data[conn].buf);

        void * src = s->edmac.conn_data[conn].buf;

        for (int jn = 0; jn <= yn; jn++)
        {
            int y     = (jn < yn) ? ya    : yb;
            int off2  = (jn < yn) ? off2a : off2b;
            for (int in = 0; in <= xn; in++)
            {
                int x     = (in < xn) ? xa    : xb;
                int off1  = (in < xn) ? off1a : off1b;
                int off23 = (in < xn) ? off2  : off3;
                for (int j = 0; j <= y; j++)
                {
                    int off = (j < y) ? off1 : off23;
                    cpu_physical_memory_write(dst, src, x);
                    src += x;
                    dst += x + off;
                }
            }
        }
        assert(src - s->edmac.conn_data[conn].buf == transfer_data_size);
        assert(dst - s->edmac.ch[channel].addr == transfer_data_skip_size);

        uint32_t old_size = s->edmac.conn_data[conn].data_size;
        uint32_t new_size = old_size - transfer_data_size;

        if (new_size)
        {
            /* only copied some of the data to memory;
             * shift the remaining data for use with subsequent transfers
             * (a little slow, kinda reinventing a FIFO)
             */
            memmove(s->edmac.conn_data[conn].buf, s->edmac.conn_data[conn].buf + transfer_data_size, new_size);
            s->edmac.conn_data[conn].buf = realloc(s->edmac.conn_data[conn].buf, new_size);
            s->edmac.conn_data[conn].data_size = new_size;
            printf("[EDMAC] conn #%d: data size %d -> %d.\n",
                conn, old_size, new_size
            );
        }
        else
        {
            free(s->edmac.conn_data[conn].buf);
            s->edmac.conn_data[conn].buf = 0;
            s->edmac.conn_data[conn].data_size = 0;
        }
        
        printf("[EDMAC#%d] %d bytes written to %X-%X.\n", channel, transfer_data_size, s->edmac.ch[channel].addr, s->edmac.ch[channel].addr + transfer_data_skip_size);
    }

    /* return end address when reading back the register */
    s->edmac.ch[channel].addr += transfer_data_skip_size;
    
    edmac_trigger_interrupt(s, channel);
    return 1;
}

static void prepro_execute(EOSState *s)
{
    if (s->prepro.adkiz_intr_en)
    {
        /* appears to use data from connections 8 and 15 */
        /* are these hardcoded? */
        if (s->edmac.conn_data[8].buf && s->edmac.conn_data[15].buf)
        {
            printf("[ADKIZ] Dummy operation.\n");
            
            /* "consume" the data from those two connections */
            assert(s->edmac.conn_data[8].buf);
            assert(s->edmac.conn_data[15].buf);

            /* free it to allow the next operation */
            free(s->edmac.conn_data[8].buf);
            s->edmac.conn_data[8].buf = 0;
            s->edmac.conn_data[8].data_size = 0;
            
            free(s->edmac.conn_data[15].buf);
            s->edmac.conn_data[15].buf = 0;
            s->edmac.conn_data[15].data_size = 0;
            
            eos_trigger_int(s, 0x65, 1);
        }
        else
        {
            printf("[ADKIZ] Data unavailable; will try again later.\n");
        }
    }
    
    if (s->prepro.hiv_enb == 0 || s->prepro.hiv_enb == 1)
    {
        if (s->edmac.conn_data[15].buf)
        {
            printf("[HIV] Dummy operation.\n");
            assert(s->edmac.conn_data[15].buf);
            free(s->edmac.conn_data[15].buf);
            s->edmac.conn_data[15].buf = 0;
            s->edmac.conn_data[15].data_size = 0;
        }
        else
        {
            printf("[HIV] Data unavailable; will try again later.\n");
        }
    }

    if (s->prepro.def_enb == 1)
    {
        if (s->edmac.conn_data[1].buf)
        {
            int transfer_size = s->edmac.conn_data[1].data_size;
            int old_size = s->edmac.conn_data[16].data_size;
            int new_size = old_size + transfer_size;
            printf("[DEF] Dummy operation (copy %d bytes from conn #1 to #16).\n", transfer_size);
            if (old_size) printf("[DEF] Data size %d -> %d.\n", old_size, new_size);
            s->edmac.conn_data[16].buf = realloc(s->edmac.conn_data[16].buf, new_size);
            s->edmac.conn_data[16].data_size = new_size;
            memcpy(s->edmac.conn_data[16].buf + old_size, s->edmac.conn_data[1].buf, transfer_size);
            free(s->edmac.conn_data[1].buf);
            s->edmac.conn_data[1].buf = 0;
            s->edmac.conn_data[1].data_size = 0;
        }
    }
}

unsigned int eos_handle_edmac ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    unsigned int ret = 0;
    int channel = (parm << 4) | ((address >> 8) & 0xF);
    assert(channel < COUNT(s->edmac.ch));
    
    switch(address & 0xFF)
    {
        case 0x00:
            msg = "control/status";
            if (value == 1)
            {
                if (channel & 8)
                {
                    /* read channel: data is always available */
                    assert(edmac_do_transfer(s, channel));
                    
                    /* any pending requests on other channels?
                     * data might be available now */
                    for (int ch = 0; ch < COUNT(s->edmac.pending); ch++)
                    {
                        if (s->edmac.pending[ch])
                        {
                            if (edmac_do_transfer(s, ch))
                            {
                                s->edmac.pending[ch] = 0;
                            }
                        }
                    }
                    
                    /* some image processing modules may now have input data */
                    prepro_execute(s);
                }
                else
                {
                    /* write channel: data may or may not be available right now */
                    if (!edmac_do_transfer(s, channel))
                    {
                        /* didn't work; schedule it for later */
                        s->edmac.pending[channel] = 1;
                    }
                }
            }
            break;

        case 0x04:
            msg = "flags";
            MMIO_VAR(s->edmac.ch[channel].flags);
            break;

        case 0x08:
            msg = "RAM address";
            MMIO_VAR(s->edmac.ch[channel].addr);
            break;

        case 0x0C:
            msg = "yn|xn";
            MMIO_VAR_2x16(s->edmac.ch[channel].xn, s->edmac.ch[channel].yn);
            break;

        case 0x10:
            msg = "yb|xb";
            MMIO_VAR_2x16(s->edmac.ch[channel].xb, s->edmac.ch[channel].yb);
            break;

        case 0x14:
            msg = "ya|xa";
            MMIO_VAR_2x16(s->edmac.ch[channel].xa, s->edmac.ch[channel].ya);
            break;

        case 0x18:
            msg = "off1b";
            MMIO_VAR(s->edmac.ch[channel].off1b);
            break;

        case 0x1C:
            msg = "off2b";
            MMIO_VAR(s->edmac.ch[channel].off2b);
            break;

        case 0x20:
            msg = "off1a";
            MMIO_VAR(s->edmac.ch[channel].off1a);
            break;

        case 0x24:
            msg = "off2a";
            MMIO_VAR(s->edmac.ch[channel].off2a);
            break;

        case 0x28:
            msg = "off3";
            MMIO_VAR(s->edmac.ch[channel].off3);
            break;

        case 0x40:
            msg = "off40";
            MMIO_VAR(s->edmac.ch[channel].off40);
            break;

        case 0x30:
            msg = "interrupt reason?";
            if(type & MODE_WRITE)
            {
            }
            else
            {
                /* read channels:
                 *   0x02 = normal?
                 *   0x10 = abort?
                 * write channels:
                 *   0x01 = normal? (used with PackMem)
                 *   0x02 = normal?
                 *   0x04 = pop?
                 *   0x10 = abort?
                 */
                int pop_request = (s->edmac.ch[channel].flags & 0xF) == 6;
                int abort_request = 0;  /* not sure yet */
                ret = (abort_request) ? 0x10 :
                      (pop_request)   ? 0x04 :
                      (channel & 8)   ? 0x02 :
                                        0x01 ;
            }
            break;

        case 0x34:
            msg = "abort request?";
            break;
    }
    
    char name[32];
    snprintf(name, sizeof(name), "EDMAC#%d", channel);
    io_log(name, s, address, type, value, ret, msg, 0, 0);
    return ret;
}

/* EDMAC channel switch (connections) */
unsigned int eos_handle_edmac_chsw ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    int msg_arg1 = 0;
    int msg_arg2 = 0;
    unsigned int ret = 0;
    
    /* fixme: reads not implemented */
    assert(type & MODE_WRITE);
    
    if (value == 0x80000000)
    {
        /* ?! used on M3 */
        goto end;
    }

    /* 0xC0F05020 - 0xC0F050E0: read edmac connections */
    /* 0xC0F05000 - 0xC0F0501C: write channel connections for channels 0-6, then 16 */
    /* 0xC0F05200 - 0xC0F05240: write channel connections for channels > 16 */
    switch(address & 0xFFF)
    {
        case 0x020 ... 0x0E0:
        {
            /* read channels  8...13 =>  0...5 */
            /* read channels 24...29 =>  6...11 */
            /* read channels 40...43 => 12...15 */
            uint32_t conn = ((address & 0xFF) - 0x20) >> 2;
            uint32_t ch = 
                (value <=  5) ? value + 8      :
                (value <= 11) ? value + 16 + 2 :
                (value <= 15) ? value + 32 - 4 : -1 ;
            assert(conn < COUNT(s->edmac.conn_data));
            assert(ch < COUNT(s->edmac.ch));
            s->edmac.read_conn[conn] = ch;
            
            /* make sure this mapping is unique */
            /* (not sure how it's supposed to work, but...) */
            for (int c = 0; c < COUNT(s->edmac.read_conn); c++)
            {
                if (c != conn && s->edmac.read_conn[c] == ch)
                {
                    printf("[CHSW] Warning: disabling RD#%d -> conn #%d.\n", ch, c);
                    s->edmac.read_conn[c] = 0;
                }
            }
            
            msg = "RAM -> RD#%d -> connection #%d";
            msg_arg1 = ch;
            msg_arg2 = conn;
            break;
        }

        case 0x000 ... 0x01C:
        {
            uint32_t conn = value;
            uint32_t ch = (address & 0x1F) >> 2;
            if (ch == 7) ch = 16;
            assert(conn < COUNT(s->edmac.conn_data));
            assert(ch < COUNT(s->edmac.ch));
            s->edmac.write_conn[ch] = conn;
            msg = "connection #%d -> WR#%d -> RAM";
            msg_arg1 = conn;
            msg_arg2 = ch;
            break;
        }

        case 0x200 ... 0x240:
        {
            /* write channels 17 ... 22: pos 0...5 */
            /* write channels 32 ... 33: pos 6...7 */
            uint32_t conn = value;
            uint32_t pos = (address & 0x3F) >> 2;
            uint32_t ch =
                (pos <= 5) ? pos + 16 + 1
                           : pos + 32 - 6 ;
            assert(conn < COUNT(s->edmac.conn_data));
            assert(ch < COUNT(s->edmac.ch));
            s->edmac.write_conn[ch] = conn;
            msg = "connection #%d -> WR#%d -> RAM";
            msg_arg1 = conn;
            msg_arg2 = ch;
            break;
        }
    }

end:
    io_log("CHSW", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

static const char * prepro_reg_name(unsigned int addr)
{
    /* http://magiclantern.wikia.com/wiki/Register_Map#Image_PreProcessing */
    switch(addr)
    {
        case 0xC0F08000: return "DARK_ENB";
        case 0xC0F08004: return "DARK_MODE";
        case 0xC0F08008: return "DARK_SETUP";
        case 0xC0F0800C: return "DARK_LIMIT";
        case 0xC0F08010: return "DARK_SETUP_14_12";
        case 0xC0F08014: return "DARK_LIMIT_14_12";
        case 0xC0F08018: return "DARK_SAT_LIMIT";

        case 0xC0F08020: return "SHAD_ENB";
        case 0xC0F08024: return "SHAD_MODE";
        case 0xC0F08028: return "SHADE_PRESETUP";
        case 0xC0F0802C: return "SHAD_POSTSETUP";
        case 0xC0F08030: return "SHAD_GAIN";
        case 0xC0F08034: return "SHAD_PRESETUP_14_12";
        case 0xC0F08038: return "SHAD_POSTSETUP_14_12";

        case 0xC0F08040: return "TWOADD_ENB";
        case 0xC0F08044: return "TWOADD_MODE";
        case 0xC0F08048: return "TWOADD_SETUP";
        case 0xC0F0804C: return "TWOADD_LIMIT";
        case 0xC0F08050: return "TWOADD_SETUP_14_12";
        case 0xC0F08054: return "TWOADD_LIMIT_14_12";
        case 0xC0F08058: return "TWOADD_SAT_LIMIT";

        case 0xC0F08060: return "DSUNPACK_ENB";
        case 0xC0F08064: return "DSUNPACK_MODE";

        case 0xC0F08070: return "UNPACK24_ENB";
        case 0xC0F08074: return "UNPACK24_MODE ";

        case 0xC0F08080: return "ADUNPACK_ENB";
        case 0xC0F08084: return "ADUNPACK_MODE";

        case 0xC0F08090: return "PACK32_ENB";
        case 0xC0F08094: return "PACK32_MODE";

        case 0xC0F080A0: return "DEF_ENB";
        case 0xC0F080A4: return "DEF_unk1";
        case 0xC0F080A8: return "DEF_DEF_MODE";
        case 0xC0F080AC: return "DEF_DEF_CTRL";
        case 0xC0F080B0: return "DEF_YB_XB";
        case 0xC0F080B4: return "DEF_YN_XN";
        case 0xC0F080BC: return "DEF_YA_XA?";
        case 0xC0F080C0: return "DEF_BUF_NUM";
        case 0xC0F080C4: return "DEF_INTR_BE";
        case 0xC0F080C8: return "DEF_INTR_AE";
        case 0xC0F080D0: return "DEF_INTR_NUM/DEF_INTR_EN?";
        case 0xC0F080D4: return "DEF_HOSEI";

        case 0xC0F08100: return "CCDSEL";
        case 0xC0F08104: return "DS_SEL";
        case 0xC0F08108: return "OBWB_ISEL";
        case 0xC0F0810C: return "PROC24_ISEL";
        case 0xC0F08110: return "DPCME_ISEL";
        case 0xC0F08114: return "PACK32_ISEL";

        case 0xC0F08120: return "PACK16_ENB";
        case 0xC0F08124: return "PACK16_MODE";

        case 0xC0F08130: return "DEFM_ENB";
        case 0xC0F08134: return "DEFM_unk1";
        case 0xC0F08138: return "DEFM_MODE";
        case 0xC0F08140: return "DEFM_INTR_NUM";
        case 0xC0F0814C: return "DEFM_GRADE";
        case 0xC0F08150: return "DEFM_DAT_TH";
        case 0xC0F08154: return "DEFM_INTR_CLR";
        case 0xC0F08158: return "DEFM_INTR_EN";
        case 0xC0F0815C: return "DEFM_14_12_SEL";
        case 0xC0F08160: return "DEFM_DAT_TH_14_12";
        case 0xC0F0816C: return "DEFM_X2MODE";

        case 0xC0F08180: return "HIV_ENB";
        case 0xC0F08184: return "HIV_V_SIZE";
        case 0xC0F08188: return "HIV_H_SIZE";
        case 0xC0F0818C: return "HIV_POS_V_OFST";
        case 0xC0F08190: return "HIV_POS_H_OFST";
        case 0xC0F08194: return "HIV_unk1";
        case 0xC0F08198: return "HIV_unk2";
        case 0xC0F0819C: return "HIV_POST_SETUP";
        case 0xC0F081C0: return "HIV_H_SWS_ENB";
        case 0xC0F08214: return "HIV_PPR_EZ";
        case 0xC0F08218: return "HIV_IN_SEL";

        case 0xC0F08220: return "ADKIZ_unk1";
        case 0xC0F08224: return "ADKIZ_THRESHOLD";
        case 0xC0F08234: return "ADKIZ_TOTAL_SIZE";
        case 0xC0F08238: return "ADKIZ_INTR_CLR";
        case 0xC0F0823C: return "ADKIZ_INTR_EN";
        case 0xC0F08240: return "ADMERG_INTR_EN";
        case 0xC0F08244: return "ADMERG_TOTAL_SIZE";
        case 0xC0F08248: return "ADMERG_MergeDefectsCount";
        case 0xC0F08250: return "ADMERG_2_IN_SE";
        case 0xC0F0825C: return "ADKIZ_THRESHOLD_14_12";

        case 0xC0F08260: return "UNPACK24_DM_EN";
        case 0xC0F08264: return "PACK32_DM_EN";
        case 0xC0F08268: return "PACK32_MODE_H";
        case 0xC0F0826C: return "unk_MODE_H";
        case 0xC0F08270: return "DEFC_X2MODE";
        case 0xC0F08274: return "DSUNPACK_DM_EN";
        case 0xC0F08278: return "ADUNPACK_DM_EN";
        case 0xC0F0827C: return "PACK16_CCD2_DM_EN";

        case 0xC0F08280: return "SHAD_CBIT";
        case 0xC0F08284: return "SHAD_C8MODE";
        case 0xC0F08288: return "SHAD_C12MODE";
        case 0xC0F0828C: return "SHAD_RMODE";
        case 0xC0F08290: return "SHAD_COF_SEL";

        case 0xC0F082A0: return "DARK_KZMK_SAV_A";
        case 0xC0F082A4: return "DARK_KZMK_SAV_B";
        case 0xC0F082A8: return "SHAD_KZMK_SAV";
        case 0xC0F082AC: return "TWOA_KZMK_SAV_A";
        case 0xC0F082B0: return "TWOA_KZMK_SAV_B";
        case 0xC0F082B4: return "DEFC_DET_MODE";
        case 0xC0F082B8: return "PACK16_DEFM_ON";
        case 0xC0F082BC: return "PACK32_DEFM_ON";
        case 0xC0F082C4: return "HIV_DEFMARK_CANCEL";

        case 0xC0F082D0: return "PACK16_ISEL";
        case 0xC0F082D4: return "WDMAC32_ISEL";
        case 0xC0F082D8: return "WDMAC16_ISEL";
        case 0xC0F082DC: return "OBINTG_ISEL";
        case 0xC0F082E0: return "AFFINE_ISEL";
        case 0xC0F08390: return "OBWB_ISEL2";
        case 0xC0F08394: return "PROC24_ISEL2";
        case 0xC0F08398: return "PACK32_ISEL2";
        case 0xC0F0839C: return "PACK16_ISEL2";
        case 0xC0F083A0: return "TAIWAN_ISEL";

        case 0xC0F08420: return "HIV_BASE_OFST";
        case 0xC0F08428: return "HIV_GAIN_DIV";
        case 0xC0F0842C: return "HIV_PATH";

        case 0xC0F08540: return "RSHD_ENB";

        case 0xC0F085B0: return "PACK32_ILIM";
        case 0xC0F085B4: return "PACK16_ILIM";
    }
    
    return NULL;
}

unsigned int eos_handle_prepro ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = prepro_reg_name(address);
    unsigned int ret = 0;

    switch (address & 0xFFF)
    {
        case 0x240:     /* ADMERG_INTR_EN */
            MMIO_VAR(s->prepro.adkiz_intr_en);
            break;
        
        case 0x0C8:     /* DEF_INTR_AE */
            if(type & MODE_WRITE)
            {
            }
            else
            {
                int AdKizDet_flag = (s->model->digic_version == 4) ? 0x20 : 
                                    (s->model->digic_version == 5) ? 0x10 : 0;
                assert(AdKizDet_flag);
                ret = AdKizDet_flag;   /* Interruppt AdKizDet */
            }
            break;
        
        case 0x180:     /* HIV_ENB */
            MMIO_VAR(s->prepro.hiv_enb);
            break;

        case 0x120:     /* PACK16_ENB */
            MMIO_VAR(s->prepro.pack16_enb);
            break;

        case 0x060:     /* DSUNPACK_ENB */
            MMIO_VAR(s->prepro.dsunpack_enb);
            break;

        case 0x0A0:     /* DEF_ENB */
            MMIO_VAR(s->prepro.def_enb);
            break;
    }

    io_log("PREPRO", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_head ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = prepro_reg_name(address);
    unsigned int ret = 0;
    
    uint32_t bases[] = { 0, 0xC0F07048, 0xC0F0705C, 0xC0F07134, 0xC0F07148 };
    uint32_t base = bases[parm];
    
    uint32_t interrupts[] = { 0, 0x6A, 0x6B, 0xD9, 0xE0 };
    
    switch (address - base)
    {
        case 0:
            msg = "Enable?";
            break;
        case 4:
            msg = "Start?";
            if(type & MODE_WRITE)
            {
                if (value == 0xC)
                {
                    eos_trigger_int(s, interrupts[parm], 50);
                }
            }
            break;
        case 8:
            msg = "Timer ticks";
            break;
    }

    char name[32];
    snprintf(name, sizeof(name), "HEAD%d", parm);
    io_log(name, s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_engio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    io_log("ENGIO", s, address, type, value, 0, 0, 0, 0);
    return 0;
}

unsigned int eos_handle_power_control ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    static uint32_t data[0x100 >> 2];
    uint32_t index = (address & 0xFF) >> 2;
    
    MMIO_VAR(data[index]);
    
    io_log("Power", s, address, type, value, ret, 0, 0, 0);
    return ret;
}


unsigned int eos_handle_adc ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    int msg_arg1 = 0;
    unsigned int ret = 0;
    
    if(type & MODE_WRITE)
    {
    }
    else
    {
        int channel = (address & 0xFF) >> 2;
        msg = "channel #%d";
        msg_arg1 = channel;
        
        if (strcmp(s->model->name, "EOSM3") == 0 ||
            strcmp(s->model->name, "EOSM10") == 0)
        {
            /* values from Ant123's camera (M3) */
            uint32_t adc_values[] = {
                0x0000de40, 0x00008c00, 0x00008300, 0x00003ca0,
                0x00003eb0, 0x00003f00, 0x0000aa90, 0x00000050,
                0x00003c20, 0x0000fd60, 0x0000f720, 0x00000030,
                0x00008a80, 0x0000a440, 0x00000020, 0x00000030,
                0x00000030, 0x00008900, 0x0000fd60, 0x0000fed0,
                0x0000fed0, 0x00000310, 0x00000020, 0x00000020,
                0x00000020, 0x00000020, 0x00000010, 0x00000000
            };
            
            if (channel < COUNT(adc_values))
            {
                ret = adc_values[channel];
            }
        }
    }
    
    io_log("ADC", s, address, type, value, ret, msg, msg_arg1, 0);
    return ret;
}

unsigned int eos_handle_dma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    unsigned int ret = 0;
    static unsigned int srcAddr = 0;
    static unsigned int dstAddr = 0;
    static unsigned int count = 0;
    unsigned int interruptId[] = { 0x00, 0x2f, 0x74, 0x75, 0x76, 0xA0, 0xA1, 0xA8, 0xA9 };

    switch(address & 0xFF)
    {
        case 0x08:
            if(type & MODE_WRITE)
            {
                if(value & 1)
                {
                    /* Start DMA */
                    printf("[DMA%i] Copy [0x%08X] -> [0x%08X], length [0x%08X], flags [0x%08X]\r\n", parm, srcAddr, dstAddr, count, value);

                    uint32_t blocksize = 8192;
                    uint8_t *buf = malloc(blocksize);
                    uint32_t remain = count;
                    
                    uint32_t src = srcAddr;
                    uint32_t dst = dstAddr;

                    while(remain)
                    {
                        uint32_t transfer = (remain > blocksize) ? blocksize : remain;

                        cpu_physical_memory_rw(src, buf, transfer, 0);
                        cpu_physical_memory_rw(dst, buf, transfer, 1);

                        remain -= transfer;
                        src += transfer;
                        dst += transfer;
                    }
                    free(buf);

                    printf("[DMA%i] OK\n", parm);

                    /* 1200D assumes the DMA transfer are not instant */
                    /* (otherwise, assert in Startup task - cannot find property 0x2) */
                    eos_trigger_int(s, interruptId[parm], count / 10000);
                    
                    /* quiet */
                    return 0;
                }
            }
            else
            {
                ret = 0;
            }
            break;

        case 0x18:
            msg = "srcAddr";
            MMIO_VAR(srcAddr);
            break;

        case 0x1C:
            msg = "dstAddr";
            MMIO_VAR(dstAddr);
            break;

        case 0x20:
            msg = "count";
            MMIO_VAR(count);
            break;
    }

    char dma_name[5];
    snprintf(dma_name, sizeof(dma_name), "DMA%i", parm);
    io_log(dma_name, s, address, type, value, ret, msg, 0, 0);

    return 0;
}


unsigned int eos_handle_uart ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 1;
    const char * msg = 0;
    int msg_arg1 = 0;
    static int enable_tio_interrupt = 0;

    if (address == 0xC0270000)
    {
        /* TIO enable flag on EOS M3? */
        static int mem = 0;
        MMIO_VAR(mem);
        goto end;
    }

    switch(address & 0xFF)
    {
        case 0x00:
            if(type & MODE_WRITE)
            {
                assert(value == (value & 0xFF));
                
                if (s->uart.chr) {
                    qemu_chr_fe_write_all(s->uart.chr, (void*) &value, 1);
                }
                
                /* fixme: better way to check whether the serial is printing to console? */
                if (strcmp(s->uart.chr->filename, "stdio") != 0 &&
                    strcmp(s->uart.chr->filename, "mux") != 0)
                {
                    printf("\x1B[31m%c\x1B[0m", value);

                    if (enable_tio_interrupt)
                    {
                        /* if using interrupts, prefer line-buffered output */
                        eos_trigger_int(s, 0x3A + parm, 0);
                    }
                    
                    if (!enable_tio_interrupt || value == '\n')
                    {
                        /* not all messages have a newline */
                        fflush(stdout);
                    }
                }
            }
            else
            {
                ret = 0;
            }
            break;

        case 0x04:
            msg = "Read byte: 0x%02X";
            s->uart.reg_st &= ~(ST_RX_RDY);
            ret = s->uart.reg_rx;
            msg_arg1 = ret;
            break;

        case 0x14:
            if(type & MODE_WRITE)
            {
                if(value & 1)
                {
                    msg = "Reset RX indicator";
                    s->uart.reg_st &= ~(ST_RX_RDY);
                }
                else
                {
                    ret = s->uart.reg_st;
                }
            }
            else
            {
                msg = "Status: 1 = char available, 2 = can write";
                ret = s->uart.reg_st;
            }
            break;

        case 0x18:
            /* 1000D expects interrupt 0x3A to be triggered after writing each char */
            /* most other cameras are upset by this interrupt */
            enable_tio_interrupt = (value == 0xFFFFFFC4);
            msg = (value == 0xFFFFFFC4) ? "enable interrupt?" : "interrupt related?";
            ret = 0;
            break;
    }

end:
    if (qemu_loglevel_mask(EOS_LOG_UART))
    {
        io_log("UART", s, address, type, value, ret, msg, msg_arg1, 0);
    }
    return ret;
}

unsigned int eos_handle_sio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    char msg[100] = "";
    char mod[10];
    
    snprintf(mod, sizeof(mod), "SIO%i", parm);

    static unsigned int last_sio_data = 0;
    static unsigned int last_sio_setup1 = 0;
    static unsigned int last_sio_setup2 = 0;
    static unsigned int last_sio_setup3 = 0;
    unsigned int pc = CURRENT_CPU->env.regs[15];

    switch(address & 0xFF)
    {
        case 0x04:
            if((type & MODE_WRITE) && (value & 1))
            {
                snprintf(msg, sizeof(msg), "Transmit: 0x%08X, setup 0x%08X 0x%08X 0x%08X PC: 0x%08X", last_sio_data, last_sio_setup1, last_sio_setup2, last_sio_setup3, pc );

                switch(s->rtc.transfer_format)
                {
                    /* no special mode */
                    case 0xFF:
                    {
                        uint8_t cmd = value & 0x0F;
                        uint8_t reg = (value>>4) & 0x0F;
                        s->rtc.transfer_format = cmd;
                        s->rtc.current_reg = reg;

                        switch(cmd)
                        {
                            /* burst writing */
                            case 0x00:
                            /* burst reading */
                            case 0x04:
                            /* 1 byte writing */
                            case 0x08:
                            /* 1 byte reading */
                            case 0x0C:
                                break;

                            default:
                                snprintf(mod, sizeof(mod), "RTC");
                                snprintf(msg, sizeof(msg), "Requested invalid transfer mode 0x%02X", value);
                                break;
                        }
                    }

                    /* burst writing */
                    case 0x00:
                        s->rtc.regs[s->rtc.current_reg] = value;
                        s->rtc.current_reg++;
                        s->rtc.current_reg %= 0x10;
                        break;

                    /* burst reading */
                    case 0x04:
                        last_sio_data = s->rtc.regs[s->rtc.current_reg];
                        s->rtc.current_reg++;
                        s->rtc.current_reg %= 0x10;
                        break;

                    /* 1 byte writing */
                    case 0x08:
                        s->rtc.regs[s->rtc.current_reg] = value;
                        s->rtc.transfer_format = 0xFF;
                        break;

                    /* 1 byte reading */
                    case 0x0C:
                        last_sio_data = s->rtc.regs[s->rtc.current_reg];
                        s->rtc.transfer_format = 0xFF;
                        break;

                    default:
                        break;
                }
            }
            else
            {
                ret = 0;
            }
            break;

        case 0x0C:
            MMIO_VAR(last_sio_setup1);
            break;

        case 0x10:
            MMIO_VAR(last_sio_setup2);
            break;

        case 0x14:
            MMIO_VAR(last_sio_setup3);
            break;

        case 0x18:
            snprintf(msg, sizeof(msg), "TX register");
            MMIO_VAR(last_sio_data);
            break;

        case 0x1C:
            snprintf(msg, sizeof(msg), "RX register");
            /* fixme */
            MMIO_VAR(last_sio_data);
            break;
    }

    io_log(mod, s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_digic_timer ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;

    if(type & MODE_WRITE)
    {
    }
    else
    {
        ret = s->digic_timer;
        msg = "DIGIC clock";
        return ret; /* be quiet */
    }

    io_log("TIMER", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

/* based on pl181_send_command from hw/sd/pl181.c */
#define EPRINTF(fmt, ...) do { fprintf(stderr, "[SDIO] " fmt , ## __VA_ARGS__); } while (0)
#define DPRINTF(fmt, ...) do { } while (0)
//#define DPRINTF EPRINTF
#define SDIO_STATUS_OK              0x1
#define SDIO_STATUS_ERROR           0x2
#define SDIO_STATUS_DATA_AVAILABLE  0x200000

static void sdio_send_command(SDIOState *sd)
{
    SDRequest request;
    uint8_t response[24] = {0};
    int rlen;

    uint32_t cmd_hi = sd->cmd_hi;
    uint32_t cmd = (cmd_hi >> 8) & ~0x40;
    uint64_t param_hi = sd->cmd_hi & 0xFF;
    uint64_t param_lo = sd->cmd_lo >> 8;
    uint64_t param = param_lo | (param_hi << 24);

    request.cmd = cmd;
    request.arg = param;
    DPRINTF("Command %d %08x\n", request.cmd, request.arg);
    rlen = sd_do_command(sd->card, &request, response+4);
    if (rlen < 0)
        goto error;

    if (sd->cmd_flags != 0x11 && sd->cmd_flags != 0x1) {
#define RWORD(n) (((uint32_t)response[n] << 24) | (response[n + 1] << 16) \
                  | (response[n + 2] << 8) | response[n + 3])
        if (rlen == 0)
            goto error;
        if (rlen != 4 && rlen != 16)
            goto error;
        
        if (rlen == 4) {
            /* response bytes are shifted by one, but only for rlen=4 ?! */
            sd->response[0] = RWORD(5);
            sd->response[1] = RWORD(1);
            sd->response[2] = sd->response[3] = 0;
        } else {
            sd->response[0] = RWORD(16);
            sd->response[1] = RWORD(12);
            sd->response[2] = RWORD(8);
            sd->response[3] = RWORD(4);
        }
        DPRINTF("Response received\n");
        sd->status |= SDIO_STATUS_OK;
#undef RWORD
    } else {
        DPRINTF("Command sent\n");
        sd->status |= SDIO_STATUS_OK;
    }
    return;

error:
    EPRINTF("Error\n");
    sd->status |= SDIO_STATUS_ERROR;
}

/* inspired from pl181_fifo_run from hw/sd/pl181.c */
/* only DMA transfers implemented */
static void sdio_read_data(SDIOState *sd)
{
    int i;

    if (sd->status & SDIO_STATUS_DATA_AVAILABLE)
    {
        EPRINTF("ERROR: read already done (%x)\n", sd->status);
        return;
    }
    
    if (!sd_data_ready(sd->card))
    {
        EPRINTF("ERROR: no data available\n");
        return;
    }

    if (!sd->dma_enabled)
    {
        EPRINTF("Reading %dx%d bytes without DMA (not implemented)\n", sd->transfer_count, sd->read_block_size);
        for (i = 0; i < sd->transfer_count * sd->read_block_size; i++)
        {
            /* dummy read, ignore this data */
            /* todo: send it on the 0x6C register? */
            sd_read_data(sd->card);
        }
        return;
    }

    DPRINTF("Reading %d bytes to %x\n", sd->dma_count, sd->dma_addr);

    for (i = 0; i < sd->dma_count/4; i++)
    {
        uint32_t value1 = sd_read_data(sd->card);
        uint32_t value2 = sd_read_data(sd->card);
        uint32_t value3 = sd_read_data(sd->card);
        uint32_t value4 = sd_read_data(sd->card);
        uint32_t value = (value1 << 0) | (value2 << 8) | (value3 << 16) | (value4 << 24);
        
        uint32_t addr = sd->dma_addr + i*4; 
        cpu_physical_memory_write(addr, &value, 4);
    }

    sd->status |= SDIO_STATUS_DATA_AVAILABLE;
    sd->dma_transferred_bytes = sd->dma_count;
}

static void sdio_write_data(SDIOState *sd)
{
    int i;

    if (sd->status & SDIO_STATUS_DATA_AVAILABLE)
    {
        EPRINTF("ERROR: write already done (%x)\n", sd->status);
        return;
    }

    if (!sd->dma_enabled)
    {
        EPRINTF("ERROR!!! Writing %dx%d bytes without DMA (not implemented)\n", sd->transfer_count, sd->read_block_size);
        EPRINTF("Cannot continue without risking corruption on the SD card image.\n");
        exit(1);
    }

    DPRINTF("Writing %d bytes from %x\n", sd->dma_count, sd->dma_addr);

    for (i = 0; i < sd->dma_count/4; i++)
    {
        uint32_t addr = sd->dma_addr + i*4; 
        uint32_t value;
        cpu_physical_memory_read(addr, &value, 4);
        
        sd_write_data(sd->card, (value >>  0) & 0xFF);
        sd_write_data(sd->card, (value >>  8) & 0xFF);
        sd_write_data(sd->card, (value >> 16) & 0xFF);
        sd_write_data(sd->card, (value >> 24) & 0xFF);
    }

    /* not sure */
    sd->status |= SDIO_STATUS_DATA_AVAILABLE;
    sd->dma_transferred_bytes = sd->dma_count;
}

void sdio_trigger_interrupt(EOSState *s, SDIOState *sd)
{
    /* after a successful operation, trigger interrupt if requested */
    if ((sd->cmd_flags == 0x13 || sd->cmd_flags == 0x14 || sd->cmd_flags == 0x04)
        && !(sd->status & SDIO_STATUS_DATA_AVAILABLE))
    {
        /* if the current command does a data transfer, don't trigger until complete */
        DPRINTF("Warning: data transfer not yet complete\n");
        return;
    }
    
    if ((sd->status & 3) == 1 && sd->irq_flags)
    {
        assert(s->model->sd_driver_interrupt);
        eos_trigger_int(s, s->model->sd_driver_interrupt, 0);
        
        if (sd->dma_enabled)
        {
            assert(s->model->sd_dma_interrupt);
            eos_trigger_int(s, s->model->sd_dma_interrupt, 0);
        }
    }
}

unsigned int eos_handle_sdio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;

    switch(address & 0xFFF)
    {
        case 0x08:
            msg = "DMA";
            MMIO_VAR(s->sd.dma_enabled);
            break;

        case 0x0C:
            msg = "Command flags?";
            if(type & MODE_WRITE)
            {
                /* must return 0? something else?
                 * maybe clear some flags after executing a command?
                 */
                s->sd.cmd_flags = value;
                
                /* reset status before doing any command */
                s->sd.status = 0;
                
                /* interpret this command */
                sdio_send_command(&s->sd);
                
                if (value == 0x14 || value == 0x4)
                {
                    /* read transfer */
                    s->sd.pio_transferred_bytes = 0;
                    s->sd.dma_transferred_bytes = 0;
                    
                    if (s->sd.dma_enabled)
                    {
                        /* DMA read transfer */
                        sdio_read_data(&s->sd);
                        sdio_trigger_interrupt(s,&s->sd);
                    }
                    else
                    {
                        /* PIO read transfer */
                        s->sd.status |= SDIO_STATUS_DATA_AVAILABLE;
                    }
                }
                else
                {
                    if (value == 0x13)  /* also 0x03? */
                    {
                        /* write transfer */
                        s->sd.pio_transferred_bytes = 0;
                        s->sd.dma_transferred_bytes = 0;
                    }

                    /* non-data or write transfer */
                    sdio_trigger_interrupt(s,&s->sd);
                }
            }
            break;

        case 0x10:
            msg = "Status";
            /**
             * 0x00000001 => command complete
             * 0x00000002 => error
             * 0x00200000 => data available?
             **/
            if(type & MODE_WRITE)
            {
                /* writes to this register appear to clear status bits */
                s->sd.status &= value;
            }
            else
            {
                ret = s->sd.status;
            }
            break;

        case 0x14:
            msg = "irq enable?";
            MMIO_VAR(s->sd.irq_flags);

            /* sometimes, a write command ends with this register
             * other times, it ends with SDDMA register 0x78/0x38
             */
            
            if (s->sd.cmd_flags == 0x13 && s->sd.dma_enabled && value)
            {
                sdio_write_data(&s->sd);
            }

            /* sometimes this register is configured after the transfer is started */
            /* since in our implementation, transfers are instant, this would miss the interrupt,
             * so we trigger it from here too. */
            sdio_trigger_interrupt(s,&s->sd);
            break;

        case 0x18:
            msg = "init?";
            break;

        case 0x20:
            msg = "cmd_lo";
            MMIO_VAR(s->sd.cmd_lo);
            break;

        case 0x24:
            msg = "cmd_hi";
            MMIO_VAR(s->sd.cmd_hi);
            break;

        case 0x28:
            msg = "Response size (bits)";
            break;

        case 0x2c:
            msg = "response setup?";
            break;

        case 0x34:
            msg = "Response[0]";
            ret = s->sd.response[0];
            break;

        case 0x38:
            msg = "Response[1]";
            ret = s->sd.response[1];
            break;

        case 0x3C:
            msg = "Response[2]";
            ret = s->sd.response[2];
            break;

        case 0x40:
            msg = "Response[3]";
            ret = s->sd.response[3];
            break;

        case 0x58:
            msg = "bus width";
            break;

        case 0x5c:
            msg = "write block size";
            MMIO_VAR(s->sd.write_block_size);
            break;

        case 0x64:
            msg = "bus width";
            break;

        case 0x68:
            msg = "read block size";
            MMIO_VAR(s->sd.read_block_size);
            break;

        case 0x6C:
            msg = "FIFO data";
            if(type & MODE_WRITE)
            {
            }
            else
            {
                if (sd_data_ready(s->sd.card))
                {
                    uint32_t value1 = sd_read_data(s->sd.card);
                    uint32_t value2 = sd_read_data(s->sd.card);
                    uint32_t value3 = sd_read_data(s->sd.card);
                    uint32_t value4 = sd_read_data(s->sd.card);
                    uint32_t value = (value1 << 0) | (value2 << 8) | (value3 << 16) | (value4 << 24);
                    ret = value;
                    s->sd.pio_transferred_bytes += 4;
                    
                    /* note: CMD18 does not report !sd_data_ready when finished */
                    if (s->sd.pio_transferred_bytes >= s->sd.transfer_count * s->sd.read_block_size)
                    {
                        DPRINTF("PIO transfer completed.\n");
                        s->sd.status |= SDIO_STATUS_DATA_AVAILABLE;
                        s->sd.status |= SDIO_STATUS_OK;
                        sdio_trigger_interrupt(s,&s->sd);
                    }
                }
                else
                {
                    EPRINTF("PIO: no data available.\n");
                }
            }
            break;

        case 0x70:
            msg = "transfer status?";
            break;

        case 0x7c:
            msg = "transfer block count";
            MMIO_VAR(s->sd.transfer_count);
            break;

        case 0x80:
            msg = "transferred blocks";
            /* Goro is very strong. Goro never fails. */
            ret = s->sd.transfer_count;
            break;

        case 0x84:
            msg = "SDREP: Status register/error codes";
            break;

        case 0x88:
            msg = "SDBUFCTR: Set to 0x03 before reading";
            break;

        case 0xD4:
            msg = "Data bus monitor (?)";
            break;
    }

    io_log("SDIO", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

unsigned int eos_handle_sddma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;

    switch(address & 0x1F)
    {
        case 0x00:
            msg = "Transfer memory address";
            MMIO_VAR(s->sd.dma_addr);
            break;
        case 0x04:
            msg = "Transfer byte count";
            if (type & MODE_WRITE)
            {
                s->sd.dma_count = value;
            }
            else
            {
                ret = (s->sd.dma_enabled)
                    ? s->sd.dma_transferred_bytes
                    : s->sd.pio_transferred_bytes;
                
                /* fixme: M3 fails with the above */
                ret = 0;
            }
            break;
        case 0x10:
            msg = "Flags/Status";
            if (type & MODE_WRITE)
            {
                s->sd.dma_enabled = value & 1;
            }
            break;
        case 0x14:
            msg = "Status?";
            ret = (s->sd.dma_enabled) ? 0x81 : 0;
            break;
        case 0x18:
            msg = "Transfer start?";

            /* DMA transfer? */
            if (s->sd.cmd_flags == 0x13)
            {
                sdio_write_data(&s->sd);
                sdio_trigger_interrupt(s,&s->sd);
            }

            break;
    }

    io_log("SDDMA", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

#undef DPRINTF
#undef EPRINTF

// #define DPRINTF(fmt, ...) do { printf("[CFDMA] " fmt , ## __VA_ARGS__); } while (0)
#define DPRINTF(fmt, ...) do { } while (0)

static int cfdma_read_data(EOSState *s, CFState *cf)
{
    DPRINTF("Reading %d of %d bytes to %x\n", cf->dma_count - cf->dma_read, cf->dma_count, cf->dma_addr + cf->dma_read);
    
    assert(cf->dma_count % 4 == 0);
    
    /* for some reason, reading many values in a loop sometimes fails */
    /* in this case, the status register has the DRQ bit cleared */
    /* and we need to wait until new data arrives in the buffer */
    while ((cf->dma_read < cf->dma_count) &&
           (ide_status_read(&cf->bus, 0) & 0x08))    /* DRQ_STAT */
    {
        uint32_t value = ide_data_readl(&cf->bus, 0);
        uint32_t addr = cf->dma_addr + cf->dma_read; 
        cpu_physical_memory_write(addr, &value, 4);
        DPRINTF("%08x: %08x\n", addr, value);
        cf->dma_read += 4;
    }

    if (cf->dma_read == cf->dma_count)
    {
        /* finished? */
        cfdma_trigger_interrupt(s);
        return 0;
    }
    
    return 1;
}

static int cfdma_write_data(EOSState *s, CFState *cf)
{
    DPRINTF("Writing %d bytes from %x\n", cf->dma_count, cf->dma_addr);

    /* todo */
    assert(0);
}

static void cfdma_trigger_interrupt(EOSState *s)
{
    if (s->cf.interrupt_enabled & 0x2000001)
    {
        assert(s->model->cf_driver_interrupt);
        eos_trigger_int(s, s->model->cf_driver_interrupt, 0);
    }

    if (s->cf.interrupt_enabled & 0x10000)
    {
        assert(s->model->cf_dma_interrupt);
        eos_trigger_int(s, s->model->cf_dma_interrupt, 0);
    }
}

#undef DPRINTF

unsigned int eos_handle_cfdma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;

    qemu_mutex_lock(&s->cf.lock);

    switch(address & 0x1F)
    {
        case 0x00:
            msg = "Transfer memory address";
            MMIO_VAR(s->cf.dma_addr);
            break;
        case 0x04:
            msg = "Transfer byte count";
            if(type & MODE_WRITE)
            {
                s->cf.dma_count = value;
            }
            else
            {
                ret = s->cf.dma_read;
            }
            break;
        case 0x10:
            msg = "Unknown transfer command";
            
            if(type & MODE_WRITE)
            {
                if (value == 0x3D)
                {
                    msg = "DMA write start";
                    s->cf.dma_write_request = 1;
                }
                else if (value == 0x39 || value == 0x21)
                {
                    msg = "DMA read start";
                    s->cf.dma_read = 0;
                    
                    /* for some reason, trying to read large blocks at once
                     * may fail; not sure what's the proper way to fix it
                     * workaround: do this in a different task,
                     * where we may retry as needed */
                    s->cf.dma_read_request = 1;
                }
            }
            break;
        case 0x14:
            msg = "DMA status?";
            ret = 3;
            break;
    }

    io_log("CFDMA", s, address, type, value, ret, msg, 0, 0);
    qemu_mutex_unlock(&s->cf.lock);
    return ret;
}

unsigned int eos_handle_cfata ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;

    qemu_mutex_lock(&s->cf.lock);

    switch(address & 0xFFFF)
    {
        case 0x8104:
            msg = "CFDMA ready maybe?";
            ret = (s->cf.dma_read_request || s->cf.dma_write_request) ? 0 : 4;
            break;
        
        case 0x8040:
            msg = "Interrupt enable?";
            MMIO_VAR(s->cf.interrupt_enabled);
            break;
            
        case 0x8044:
            msg = "Interrupt related?";
            if(type & MODE_WRITE)
            {
            }
            else
            {
                /* should return what was written to 0x8040?! */
                ret = s->cf.interrupt_enabled;
            }
            break;

        case 0x21F0:    /* quiet */
        case 0x2000:    /* still testing */
            msg = "ATA data port";
            
            if(type & MODE_WRITE)
            {
                ide_data_writew(&s->cf.bus, 0, value);
                if ((address & 0xFFFF) == 0x21F0)
                {
                    qemu_mutex_unlock(&s->cf.lock);
                    return 0;
                }
            }
            else
            {
                ret = ide_data_readw(&s->cf.bus, 0);
                if ((address & 0xFFFF) == 0x21F0)
                {
                    qemu_mutex_unlock(&s->cf.lock);
                    return ret;
                }
            }
            break;

        case 0x21F1:
        case 0x21F2:
        case 0x21F3:
        case 0x21F4:
        case 0x21F5:
        case 0x21F6:
        case 0x21F7:
        case 0x2001:
        case 0x2002:
        case 0x2003:
        case 0x2004:
        case 0x2005:
        case 0x2006:
        case 0x2007:
        {
            int offset = address & 0xF;

            const char * regnames[16] = {
                [1] = "ATA feature/error",
                [2] = "ATA sector count",
                [3] = "ATA LBAlo",
                [4] = "ATA LBAmid",
                [5] = "ATA LBAhi",
                [6] = "ATA drive/head port",
                [7] = "ATA command/status",
            };
            msg = regnames[offset];

            if(type & MODE_WRITE)
            {
                ide_ioport_write(&s->cf.bus, offset, value);
                if (offset == 7 && s->cf.ata_interrupt_enabled)
                {
                    /* a command for which interrupts were requested? */
                    s->cf.pending_interrupt = 1;
                }
            }
            else
            {
                ret = ide_ioport_read(&s->cf.bus, offset);
                if (offset == 7)
                {
                    /* reading the status register clears peding interrupt */
                    s->cf.pending_interrupt = 0;
                }
            }
            break;
        }

        case 0x23F6:
        case 0x200E:
            if(type & MODE_WRITE)
            {
                msg = "ATA device control: int %s%s";
                msg_arg1 = (intptr_t) ((value & 2) ? "disable" : "enable");
                msg_arg2 = (intptr_t) ((value & 4) ? ", soft reset" : "");
                ide_cmd_write(&s->cf.bus, 0, value & 2);
                s->cf.ata_interrupt_enabled = !(value & 2);
            }
            else
            {
                msg = "ATA alternate status";
                ret = ide_status_read(&s->cf.bus, 0);
            }
            break;
    }
    io_log("CFATA", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    qemu_mutex_unlock(&s->cf.lock);
    return ret;
}

static char* format_clock_enable(int value)
{
    const char* clock_modules[] = {
        "???",  "LCLK", "ASIF?", "SD1",     // 1 2 4 8
        "???",  "???",  "???",   "???",     // 10 20 40 80
        "PWM",  "???",  "Tmr0",  "Tmr1",    // 100 200 400 800
        "Tmr2", "???",  "???",   "???",     // ...
        "???",  "???",  "???",   "???",
        "???",  "SIO",  "???",   "???",
        "DMA0", "ASIF", "???",   "???",
        "SD2",  "???",  "???",   "???"
    };
    static char clock_msg[100];
    snprintf(clock_msg, sizeof(clock_msg), "CLOCK_ENABLE: ");
    int i;
    for (i = 0; i < 32; i++)
    {
        if (value & (1 << i))
        {
            STR_APPEND(clock_msg, "%s ", clock_modules[i]);
        }
    }
    return clock_msg;
}

unsigned int eos_handle_basic ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;
    
    /* from C0100000 */
    if (parm == 0)
    {
        if ((address & 0xFFF) == 0x1C)
        {
            /* 5D classic: expects 1 at 0xFFFF01A4 */
            ret = 1;
        }
        io_log("BASIC", s, address, type, value, ret, msg, 0, 0);
        return ret;
    }

    /* from C0720000 */
    if (parm == 2)
    {
        if ((address & 0xFFF) == 8)
        {
            msg = "SUSPEND_BIT";
            ret = 0x100;
            io_log("BASIC", s, address, type, value, ret, msg, 0, 0);
        }
        return ret;
    }
    
    /* from C0400000 */
    switch(address & 0xFFF)
    {
        case 0x008: /* CLOCK_ENABLE */
            MMIO_VAR(s->clock_enable);
            msg = format_clock_enable(s->clock_enable);
            break;
        
        case 0xA4:
            /* A1100: expects 3 at 0xFFFF0060 */
            msg = "A1100 init";
            ret = 3;
            break;
        
        case 0x244:
            /* idk, expected to be so in 5D3 123 */
            ret = 1;
            break;
        case 0x204:
            /* idk, expected to be so in 5D3 bootloader */
            ret = 2;
            break;
        
        case 0x284:
            msg = "5D3 display init?";
            ret = 1;
            break;
    }

    io_log("BASIC", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_asif ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;

    if(type & MODE_WRITE)
    {
    }
    else
    {
        switch(address & 0xFFF)
        {
            case 0x110:
            case 0x114:
                /* audio meters; don't print messages, since it will flood the console */
                return 0;
        }
    }

    io_log("ASIF", s, address, type, value, ret, 0, 0, 0);
    return ret;
}

static void process_palette_entry(uint32_t value, struct palette_entry * palette_entry, int palette_index, const char** msg)
{
    uint32_t pal = value;

    int opacity = (pal >> 24) & 0xFF;
    uint8_t Y = (pal >> 16) & 0xFF;
    int8_t  U = (pal >>  8) & 0xFF;
    int8_t  V = (pal >>  0) & 0xFF;
    int R, G, B;
    yuv2rgb(Y, U, V, &R, &G, &B);

    static char msg_pal[50];

    if (value)
    {
        snprintf(msg_pal, sizeof(msg_pal), 
            "Palette[%X] -> R%03d G%03d B%03d %s",
            palette_index, R, G, B,
            opacity != 3 ? "transparent?" : ""
        );
    }
    else
    {
        snprintf(msg_pal, sizeof(msg_pal), 
            "Palette[%X] -> empty",
            palette_index
        );
    }
    *msg = msg_pal;

    palette_entry->R = R;
    palette_entry->G = G;
    palette_entry->B = B;
    palette_entry->opacity = opacity;
}

unsigned int eos_handle_display ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;

    switch (address & 0xFFF)
    {
        case 0x0D0:
            msg = "BMP VRAM";
            MMIO_VAR(s->disp.bmp_vram);
            break;

        case 0x0E0:
            msg = "YUV VRAM";
            MMIO_VAR(s->disp.img_vram);
            break;

        case 0x080 ... 0x0BC:
            msg = "4-bit palette";
            if(type & MODE_WRITE)
            {
                int entry = ((address & 0xFFF) - 0x80) / 4;
                process_palette_entry(value, &s->disp.palette_4bit[entry], entry, &msg);
                s->disp.is_4bit = 1;
                s->disp.bmp_pitch = 360;
            }
            break;

        case 0x400 ... 0x7FC:
        case 0x800 ... 0xBFC:
            msg = "8-bit palette";
            if(type & MODE_WRITE)
            {
                int entry = (((address & 0xFFF) - 0x400) / 4) % 0x100;
                process_palette_entry(value, &s->disp.palette_8bit[entry], entry, &msg);
                s->disp.is_4bit = 0;
                s->disp.bmp_pitch = 960;
            }
            break;
    }

    io_log("Display", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

#define FLASH_STATE_READ      0
#define FLASH_STATE_UNLOCK_2  1
#define FLASH_STATE_UNLOCKED  2
#define FLASH_STATE_ERASE_1   3
#define FLASH_STATE_ERASE_2   4
#define FLASH_STATE_ERASE_3   5
#define FLASH_STATE_PROGRAM   6
#define FLASH_STATE_UNLOCK_BYPASS   7
#define FLASH_STATE_UNLOCK_BYPASS_RESET 8
#define FLASH_STATE_UNLOCK_BYPASS_ERASE 9
#define FLASH_STATE_BLOCK_ERASE_BUSY 10

unsigned int flash_get_blocksize(unsigned int rom, unsigned int size, unsigned int word_offset)
{
    switch(size)
    {
        /* 32mbit flash x16 */
        case 0x00400000:
            if((word_offset < 0x8000) || (word_offset > 0x1F0000))
            {
                /* 4 kwords */
                return 4 * 1024 * 2;
            }
            else
            {
                /* 32 kwords */
                return 32 * 1024 * 2;
            }
            break;

        default:
            return 0;
    }
}

unsigned int eos_handle_rom ( unsigned int rom, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int pc = CURRENT_CPU->env.regs[15];
    unsigned int ret = 0;
    unsigned int real_address = 0;
    unsigned int byte_offset = 0;
    unsigned int word_offset = 0;
    unsigned int base = 0;
    unsigned int size = 0;
    unsigned int fail = 0;

    static int block_erase_counter = 0;
    static int state[2] = { FLASH_STATE_READ, FLASH_STATE_READ };

    switch(rom)
    {
        case 0:
            base = 0xF8000000;
            size = ROM1_SIZE;
            break;
        case 1:
            base = 0xF0000000;
            size = ROM0_SIZE;
            break;
    }

    /* the offset relative from flash chip start */
    byte_offset = (address - base) & (size - 1);
    word_offset = byte_offset >> 1;

    /* the address of the flash data in memory space */
    real_address = base + byte_offset;

    if(!s->flash_state_machine)
    {
        return eos_default_handle ( s, real_address, type, value );
    }

    if(type & MODE_WRITE)
    {
        switch(state[rom])
        {
            case FLASH_STATE_READ:
                if(value == 0xF0)
                {
                    state[rom] = FLASH_STATE_READ;
                }
                else if(word_offset == 0x555 && value == 0xAA)
                {
                    state[rom] = FLASH_STATE_UNLOCK_2;
                }
                else if(value == 0xA0)
                {
                    printf("[ROM%i:%i] at [0x%04X] Command: UNLOCK BYPASS PROGRAM\r\n", rom, state[rom], pc);
                    state[rom] = FLASH_STATE_PROGRAM;
                }
                else if(value == 0x80)
                {
                    state[rom] = FLASH_STATE_UNLOCK_BYPASS_ERASE;
                }
                else if(value == 0x90)
                {
                    state[rom] = FLASH_STATE_UNLOCK_BYPASS_RESET;
                }
                else if(value == 0x98)
                {
                    printf("[ROM%i:%i] at [0x%04X] Command: UNLOCK BYPASS CFI unhandled\r\n", rom, state[rom], pc);
                    state[rom] = FLASH_STATE_READ;
                }
                else
                {
                    fail = 1;
                }
                break;

            case FLASH_STATE_UNLOCK_BYPASS:
                printf("[ROM%i:%i] at [0x%04X]       2nd UNLOCK BYPASS [0x%08X] -> [0x%08X] unhandled\r\n", rom, state[rom], pc, value, word_offset);
                state[rom] = FLASH_STATE_READ;
                break;


            case FLASH_STATE_UNLOCK_BYPASS_RESET:
                if(value == 0x00)
                {
                    printf("[ROM%i:%i] at [0x%04X] Command: UNLOCK BYPASS RESET\r\n", rom, state[rom], pc);
                    state[rom] = FLASH_STATE_READ;
                }
                else
                {
                    fail = 1;
                }
                break;

            case FLASH_STATE_UNLOCK_2:
                if(word_offset == 0x2AA && value == 0x55)
                {
                    state[rom] = FLASH_STATE_UNLOCKED;
                }
                else
                {
                    state[rom] = FLASH_STATE_READ;
                    fail = 1;
                }
                break;

            case FLASH_STATE_UNLOCKED:
                if(value == 0x90)
                {
                    printf("[ROM%i:%i] at [0x%04X] [0x%08X] -> [0x%08X] in autoselect unhandled\r\n", rom, state[rom], pc, value, word_offset);
                    state[rom] = FLASH_STATE_READ;
                }
                else if(word_offset == 0x555 && value == 0xA0)
                {
                    //printf("[ROM%i:%i] at [0x%04X] Command: PROGRAM\r\n", rom, state[rom], pc);
                    state[rom] = FLASH_STATE_PROGRAM;
                }
                else if(word_offset == 0x555 && value == 0x20)
                {
                    printf("[ROM%i:%i] at [0x%04X] Command: UNLOCK BYPASS\r\n", rom, state[rom], pc);
                    state[rom] = FLASH_STATE_READ;
                }
                else if(word_offset == 0x555 && value == 0x80)
                {
                    state[rom] = FLASH_STATE_ERASE_1;
                }
                else
                {
                    state[rom] = FLASH_STATE_READ;
                    fail = 1;
                }
                break;

            case FLASH_STATE_ERASE_1:
                if(word_offset == 0x555 && value == 0xAA)
                {
                    state[rom] = FLASH_STATE_ERASE_2;
                }
                else
                {
                    state[rom] = FLASH_STATE_READ;
                    fail = 1;
                }
                break;

            case FLASH_STATE_ERASE_2:
                if(word_offset == 0x2AA && value == 0x55)
                {
                    state[rom] = FLASH_STATE_ERASE_3;
                }
                else
                {
                    state[rom] = FLASH_STATE_READ;
                    fail = 1;
                }
                break;

            case FLASH_STATE_UNLOCK_BYPASS_ERASE:
                if(value == 0x30)
                {
                    int pos = 0;
                    int block_size = flash_get_blocksize(rom, size, word_offset);

                    printf("[ROM%i:%i] at [0x%04X] Command: UNLOCK BYPASS BLOCK ERASE [0x%08X]\r\n", rom, state[rom], pc, real_address);
                    for(pos = 0; pos < block_size; pos += 2)
                    {
                        eos_set_mem_w ( s, real_address + pos, 0xFFFF );
                    }
                    block_erase_counter = 0;
                    state[rom] = FLASH_STATE_BLOCK_ERASE_BUSY;
                }
                else if(value == 0x10)
                {
                    int pos = 0;

                    printf("[ROM%i:%i] at [0x%04X] Command: UNLOCK BYPASS CHIP ERASE\r\n", rom, state[rom], pc);
                    for(pos = 0; pos < size; pos += 2)
                    {
                        eos_set_mem_w ( s, base + pos, 0xFFFF );
                    }
                    state[rom] = FLASH_STATE_READ;
                }
                else
                {
                    fail = 1;
                }
                break;

            case FLASH_STATE_ERASE_3:
                if(word_offset == 0x555 && value == 0x10)
                {
                    int pos = 0;
                    printf("[ROM%i:%i] at [0x%04X] Command: CHIP ERASE\r\n", rom, state[rom], pc);
                    for(pos = 0; pos < size; pos += 2)
                    {
                        eos_set_mem_w ( s, base + pos, 0xFFFF );
                    }
                    state[rom] = FLASH_STATE_READ;
                }
                else if(value == 0x30)
                {
                    int pos = 0;
                    int block_size = flash_get_blocksize(rom, size, word_offset);

                    printf("[ROM%i:%i] at [0x%04X] Command: BLOCK ERASE [0x%08X]\r\n", rom, state[rom], pc, real_address);
                    for(pos = 0; pos < block_size; pos += 2)
                    {
                        eos_set_mem_w ( s, real_address + pos, 0xFFFF );
                    }
                    block_erase_counter = 0;
                    state[rom] = FLASH_STATE_BLOCK_ERASE_BUSY;
                }
                else
                {
                    state[rom] = FLASH_STATE_READ;
                    fail = 1;
                }
                break;

            case FLASH_STATE_PROGRAM:
                printf("[ROM%i:%i] at [0x%04X] Command: PROGRAM [0x%04X] -> [0x%08X]\r\n", rom, state[rom], pc, value, real_address);
                eos_set_mem_w ( s, real_address, value );
                state[rom] = FLASH_STATE_READ;
                break;
        }
        if(fail)
        {
            printf("[ROM%i:%i] at [0x%04X] [0x%08X] -> [0x%08X]\r\n", rom, state[rom], pc, value, word_offset);
        }
    }
    else
    {

        switch(state[rom])
        {
            case FLASH_STATE_READ:
                ret = eos_default_handle ( s, real_address, type, value );
                break;

            case FLASH_STATE_BLOCK_ERASE_BUSY:
                if(block_erase_counter < 0x10)
                {
                    block_erase_counter++;
                    ret = ((block_erase_counter&1)<<6) | ((block_erase_counter&1)<<2);
                }
                else
                {
                    ret = 0x80;
                    state[rom] = FLASH_STATE_READ;
                }
                break;

            default:
                printf("[ROM%i:%i] at [0x%04X] read in unknown state [0x%08X] <- [0x%08X]\r\n", rom, state[rom], pc, ret, word_offset);
                break;
        }
    }

    return ret;
}


unsigned int eos_handle_flashctrl ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    unsigned int ret = 0;

    switch(address & 0x1FF)
    {
        case 0x10:
            if(type & MODE_WRITE)
            {
                if(((value | (value >> 16)) & 0xFFFF) == 0xD9C5)
                {
                    msg = "'Write enable' enabled";
                }
                else if(value == 0x0)
                {
                    msg = "'Write enable' disabled";
                }
                else
                {
                    msg = "unknown command";
                }
            }
            else
            {
                ret = 1;
            }
            break;
    }

    io_log("FlashIF", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_jpcore( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * module_name = 0;
    const char * msg = 0;
    unsigned int ret = 0;

    switch (parm)
    {
        case 0:
        {
            /* used for JPEG and old-style lossless compression (TTJ) */
            module_name = "JP51";
            break;
        }

        case 1:
        {
            /* used for H.264 */
            module_name = "JP62";
            break;
        }

        case 2:
        {
            /* used for new-style lossless compression (TTL) */
            module_name = "JP57";
            break;
        }
    }

    /* common */
    switch(address & 0xFFFF)
    {
        case 0x0000:
            msg = "control/status?";
            if(type & MODE_WRITE)
            {
                if (value & 1)
                {
                    msg = "Start JPCORE";
                    eos_trigger_int(s, 0x64, 100);
                }
            }
            else
            {
                /* EOSM: this value starts JPCORE, but fails the DCIM test */
                //ret = 0x1010000;
            }
            break;

        case 0x0004:
            msg = "mode? (encode, decode etc)";
            break;

        case 0x000C:
            msg = "operation mode?";
            break;

        case 0x0010 ... 0x001C:
            msg = "JPEG tags, packed";
            break;

        case 0x0024:
            msg = "output size";
            break;

        case 0x0030:
            msg = "JPEGIC status?";
            ret = 0x1FF;
            break;

        case 0x0040:
            msg = "set to 0x600";
            break;

        case 0x0044:
            msg = "interrupt status? (70D loop)";
            ret = rand();
            break;

        case 0x0080:
            msg = "slice size";
            break;

        case 0x0084:
            msg = "number of components and bit depth";
            break;

        case 0x008C:
            msg = "component subsampling";
            break;

        case 0x0094:
            msg = "sample properties? for SOS header?";
            break;

        case 0x00E8:
            msg = "slice width";
            break;

        case 0x0EC:
            msg = "slice height";
            break;

        case 0x0928:
            msg = "JpCoreFirm";
            break;

        case 0x092C:
            msg = "JpCoreExtStream";
            break;
    }

    io_log(module_name, s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_eeko_comm( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = "INT%Xh: ???";
    int msg_arg1 = 0;
    unsigned int ret = 0;

    /* these interrupts are in pairs, e.g. 0x101, 0x102, 0x109, 0x10A ... */
    /* even indices / odd interrupts (reg offset 0x00, 0x40 ...) are from eeko to icu */
    /* odd indices / even interrupts (reg offset 0x20, 0x60 ...) are from icu to eeko */
    const int interrupt_map[] = {
        0x101, 0x109, 0x111, 0x119, 0x121, 0x129, 0x131, 0x139,
        0x0FF, 0x107, 0x10F, 0x117, 0x11F, 0x127, 0x12F, 0x137,
        0x123,
    };
    
    int interrupt_index = (address >> 5) & 0x3F;
    assert(interrupt_index/2 < COUNT(interrupt_map));
    int interrupt_id = interrupt_map[interrupt_index/2] + interrupt_index % 2;
    msg_arg1 = interrupt_id;
    
    switch (address & 0x1F)
    {
        case 0x04:
            msg = "INT%Xh: interrupt acknowledged";
            break;
        case 0x08:
            msg = "INT%Xh: setup interrupts? (1)";
            break;
        case 0x10:
            msg = "INT%Xh: trigger interrupt?";
            break;
        case 0x18:
            msg = "INT%Xh: setup interrupts? (B)";
            break;
    }

    io_log("EEKO", s, address, type, value, ret, msg, msg_arg1, 0);
    return ret;
}

unsigned int eos_handle_digic6 ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    unsigned int ret = 0;
    
    static uint32_t palette_addr = 0;
    
    /* 0xD20B0A24/C34/994/224, depending on model */
    if (address == s->model->card_led_address)
    {
        return eos_handle_card_led(parm, s, address, type, value);
    }

    switch (address)
    {
        case 0xD203046C:
        case 0xD203086C:
            ret = 1;
            msg = "7D2 init";
            break;

        case 0xD2030000:    /* M3: memif_wait_us */
        case 0xD20F0000:    /* M3: many reads from FC000382, value seems ignored */
            return 0;
        
        case 0xD2013800:    /* D6 */
        case 0xD201381C:    /* D6 */
        case 0xD2018200:    /* 5D4 */
        case 0xD2018230:    /* 5D4 */
            msg = "Display resolution";
            MMIO_VAR_2x16(s->disp.width, s->disp.height);
            break;
        
        case 0xD2030108:    /* D6 */
            s->disp.bmp_vram = value << 8;
            s->disp.bmp_pitch = s->disp.width;
            msg = "BMP VRAM";
            break;

        case 0xD2018228:    /* 5D4 */
            msg = "BMP VRAM";
            MMIO_VAR(s->disp.bmp_vram);
            break;
        
        case 0xD201822C:    /* 5D4 */
            msg = "BMP pitch";
            MMIO_VAR(s->disp.bmp_pitch);
            break;

        case 0xD20139A8:    /* D6 */
        case 0xD2018398:    /* 5D4 */
        {
            msg = "Bootloader palette address";
            palette_addr = value << 4;
            break;
        }
        case 0xD20139A0:
        case 0xD2018390:
        {
            msg = "Bootloader palette confirm";
            for (int i = 0; i < 16; i++)
            {
                uint32_t entry = eos_get_mem_w(s, palette_addr + i*4);
                /* palette entry is different; adjust it to match DIGIC 4/5 routines */
                uint8_t* ovuy = (uint8_t*) &entry;
                ovuy[1] -= 128; ovuy[2] -= 128;
                entry = (entry >> 8) | 0x3000000;
                const char* msg;
                process_palette_entry(entry, &s->disp.palette_8bit[i], i, &msg);
                printf("%08X: %s\n", entry, msg);
            }
            break;
        }
        
        case 0xD203040C:
        {
            msg = "MR (RAM manufacturer ID)";
            static int last = 0;
            if(type & MODE_WRITE)
            {
                last = value;
            }
            else
            {
                /* these should match the values saved in ROM at FC080010 */
                uint32_t mr = s->model->ram_manufacturer_id;
                int mr_index = (last >> 8) - 5;
                ret = (mr >> (mr_index * 8)) & 0xFF;
            }
            break;
        }

        case 0xD2090008: /* CLOCK_ENABLE */
            msg = "CLOCK_ENABLE";
            MMIO_VAR(s->clock_enable_6);
            break;

        case 0xD20B053C:
            msg = "PhySwBootSD";        /* M3: card write protect switch? */
            ret = 0x10000;
            break;

        case 0xD20BF4A0:
            msg = "PhySwKeyboard 0";    /* M3: keyboard  */
            ret = 0x10077ffb;
            break;
        
        case 0xD20BF4B0:
            msg = "PhySw 1";            /* M3:  */
            ret = 0x00001425;
            break;

        case 0xD20BF4D8:
            msg = "PhySw 2";            /* M3:  */
            ret = 0x20bb4d30;
            break;

        case 0xD20BF4F0:
            msg = "PhySw Internal Flash + ";    /* M3: Flash + */
            ret = 0x00000840;
            break;

        case 0xD20B0400:                /* 80D: 0x10000 = no card present */
        case 0xD20B22A8:                /* 5D4: same */
            msg = "SD detect";
            ret = 0;
            break;

        case 0xD20B210C:
            msg = "CF detect";          /* 5D4: same as above */
            ret = 0x10000;
            break;

        case 0xD6040000:                /* M3: appears to expect 0x3008000 or 0x3108000 */
            ret = 0x3008000;
            break;

        case 0xD5202018:                /* M5: expects 1 at 0xE0009E9C */
        case 0xD5203018:                /* M5: expects 1 at 0xE0009EBA */
            ret = 1;
            break;

        case 0xD6050000:
        {
            static int last = 0;
            if(type & MODE_WRITE)
            {
                last = value;
            }
            else
            {
                msg = "I2C status?";
                ret = (last & 0x8000) ? 0x2100100 : 0x20000;
                ret = rand();
            }
            break;
        }

        case 0xD6060000:
            msg = "E-FUSE";
            break;

        case 0xD9890014:
            msg = "Battery level maybe (ADC?)";     /* M3: called from Battery init  */
            ret = 0x00020310;
            break;

        // 100D AVS
        case 0xd02c3004: // TST 8
        case 0xd02c3024: // TST 1
        case 0xd02c4004: // TST 8
        case 0xd02c4024: // TST 1
            msg = "AVS??";
            ret = 0xff;
            break;

        case 0xC8100154:
            msg = "IPC?";
            ret = 0x10001;              /* M5: expects 0x10001 at 0xE0009E66 */
            break;

        case 0xD2101504:
            msg = "Wake up CPU1?";       /* M5: wake up the second CPU? */
            assert(s->cpu1);
            CPU(s->cpu1)->halted = 0;
            break;
    }
    
    io_log("DIGIC6", s, address, type, value, ret, msg, 0, 0);
    return ret;
}




/** EOS ROM DEVICE **/



/* its not done yet */
#if defined(EOS_ROM_DEVICE_IMPLEMENTED)
ROMState *eos_rom_register(hwaddr base, DeviceState *qdev, const char *name, hwaddr size,
                                BlockDriverState *bs,
                                uint32_t sector_len, int nb_blocs, int width,
                                uint16_t id0, uint16_t id1,
                                uint16_t id2, uint16_t id3, int be)
{
    DeviceState *dev = qdev_create(NULL, "eos.rom");
    SysBusDevice *busdev = SYS_BUS_DEVICE(dev);
    ROMState *pfl = (ROMState *)object_dynamic_cast(OBJECT(dev),
                                                    "cfi.pflash01");

    if (bs && qdev_prop_set_drive(dev, "drive", bs)) {
        abort();
    }
    qdev_prop_set_uint32(dev, "num-blocks", nb_blocs);
    qdev_prop_set_uint64(dev, "sector-length", sector_len);
    qdev_prop_set_uint8(dev, "width", width);
    qdev_prop_set_uint8(dev, "big-endian", !!be);
    qdev_prop_set_uint16(dev, "id0", id0);
    qdev_prop_set_uint16(dev, "id1", id1);
    qdev_prop_set_uint16(dev, "id2", id2);
    qdev_prop_set_uint16(dev, "id3", id3);
    qdev_prop_set_string(dev, "name", name);
    qdev_init_nofail(dev);

    sysbus_mmio_map(busdev, 0, base);
    return pfl;
}

static const MemoryRegionOps eos_rom_ops = {
/*    .old_mmio = {
        .read = { pflash_readb_be, pflash_readw_be, pflash_readl_be, },
        .write = { pflash_writeb_be, pflash_writew_be, pflash_writel_be, },
    },
    .endianness = DEVICE_NATIVE_ENDIAN,*/
};

static int eos_rom_init(SysBusDevice *dev)
{
    ROMState *pfl = FROM_SYSBUS(typeof(*pfl), dev);
    uint64_t total_len = 0x00100000;
    int ret = 0;

    memory_region_init_rom_device(&pfl->mem, &eos_rom_ops, pfl, pfl->name, total_len);
    vmstate_register_ram(&pfl->mem, DEVICE(pfl));
    pfl->storage = memory_region_get_ram_ptr(&pfl->mem);
    sysbus_init_mmio(dev, &pfl->mem);

    if (pfl->bs) {
        /* read the initial flash content */
        ret = bdrv_read(pfl->bs, 0, pfl->storage, total_len >> 9);

        if (ret < 0) {
            vmstate_unregister_ram(&pfl->mem, DEVICE(pfl));
            memory_region_destroy(&pfl->mem);
            return 1;
        }
    }

    return ret;
}

static Property eos_rom_properties[] = {
    DEFINE_PROP_DRIVE("drive", ROMState, bs),
    DEFINE_PROP_UINT32("num-blocks", ROMState, nb_blocs, 0),
    DEFINE_PROP_UINT64("sector-length", ROMState, sector_len, 0),
    DEFINE_PROP_UINT8("width", ROMState, width, 0),
    DEFINE_PROP_UINT8("big-endian", ROMState, be, 0),
    DEFINE_PROP_UINT16("id0", ROMState, ident0, 0),
    DEFINE_PROP_UINT16("id1", ROMState, ident1, 0),
    DEFINE_PROP_UINT16("id2", ROMState, ident2, 0),
    DEFINE_PROP_UINT16("id3", ROMState, ident3, 0),
    DEFINE_PROP_STRING("name", ROMState, name),
    DEFINE_PROP_END_OF_LIST(),
};

static const TypeInfo eos_rom_info = {
    .name           = "eos.rom",
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(ROMState),
    .class_init     = eos_rom_class_init,
};


static void eos_rom_class_init(ObjectClass *class, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(class);

    k->init = eos_rom_init;
    dc->props = eos_rom_properties;
}

static void eos_rom_register_types(void)
{
    type_register_static(&eos_rom_info);
}

type_init(eos_rom_register_types)
#endif
