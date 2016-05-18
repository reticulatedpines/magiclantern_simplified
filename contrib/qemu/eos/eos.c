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
#include "dirent.h"
#include "ui/console.h"
#include "ui/pixel_ops.h"
#include "hw/display/framebuffer.h"
#include "hw/sd/sd.h"
#include "eos.h"

#include "hw/eos/model_list.h"


/* Machine class */

typedef struct {
    MachineClass parent;
    const char * model;
    uint32_t rom_start;
    uint32_t digic_version;
} EosMachineClass;

#define EOS_DESC_BASE "Canon EOS"
#define TYPE_EOS_MACHINE   "eos"
#define EOS_MACHINE_GET_CLASS(obj) \
    OBJECT_GET_CLASS(EosMachineClass, obj, TYPE_EOS_MACHINE)
#define EOS_MACHINE_CLASS(klass) \
    OBJECT_CLASS_CHECK(EosMachineClass, klass, TYPE_EOS_MACHINE)

/* FIXME: merge to one function instead */
static void eos_init_common(const char *rom_filename, uint32_t rom_start, uint32_t digic_version);
static void ml_init_common(const char *rom_filename, uint32_t rom_start, uint32_t digic_version);
static void eos_common_init(MachineState *machine)
{
    char rom_filename[24];
    EosMachineClass *emc = EOS_MACHINE_GET_CLASS(machine);
    snprintf(rom_filename,24,"ROM-%s.BIN",emc->model);
    eos_init_common(rom_filename, emc->rom_start, emc->digic_version);
    if (false) // For linkage. ML is broken in 2.3.0, so not sure if we should keep it
        ml_init_common(rom_filename, emc->rom_start, emc->digic_version);
}


static void eos_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = EOS_DESC_BASE;
    mc->init = eos_common_init;
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
    const struct eos_model_desc * model = (struct eos_model_desc*) data;

    /* Create description from name */
    int desc_size = sizeof(EOS_DESC_BASE) + strlen(model->name) + 1;
    char * desc = (char*)malloc(desc_size * sizeof(char));
    if (desc) {
        snprintf(desc, desc_size, EOS_DESC_BASE " %s", model->name);
    }
    mc->desc = desc;

    emc->model = model->name;
    emc->rom_start = model->rom_start;
    emc->digic_version = model->digic_version;
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
    const struct eos_model_desc * desc = eos_model_list;
    while (desc->name != NULL) {
        snprintf(name, 32, "%s" TYPE_MACHINE_SUFFIX, desc->name);
        info.class_data = (void*)desc;
        type_register(&info);
        desc++;
    }
}




EOSRegionHandler eos_handlers[] =
{
    { "RAM Trace",    0x00000000, 0x10000000, eos_handle_ram, 0 },
    { "FlashControl", 0xC0000000, 0xC0001FFF, eos_handle_flashctrl, 0 },
    { "ROM0",         0xF8000000, 0xFFFFFFFF, eos_handle_rom, 0 },
    { "ROM1",         0xF0000000, 0xF7FFFFFF, eos_handle_rom, 1 },
    { "Interrupt",    0xC0201000, 0xC0201FFF, eos_handle_intengine, 0 },
    { "Timers",       0xC0210000, 0xC0210FFF, eos_handle_timers, 0 },
    { "Timer",        0xC0242014, 0xC0242014, eos_handle_digic_timer, 0 },
    { "HPTimer",      0xC0243000, 0xC0243FFF, eos_handle_hptimer, 0 },
    { "GPIO",         0xC0220000, 0xC022FFFF, eos_handle_gpio, 0 },
    { "Basic",        0xC0400000, 0xC0400FFF, eos_handle_basic, 1 },
    { "Basic",        0xC0720000, 0xC0720FFF, eos_handle_basic, 2 },
    { "SDIO1",        0xC0C10000, 0xC0C10FFF, eos_handle_sdio, 1 },
    { "SDIO2",        0xC0C20000, 0xC0C20FFF, eos_handle_sdio, 2 },
    { "SDDMA1",       0xC0510000, 0xC0510FFF, eos_handle_sddma, 1 },
    { "SDDMA3",       0xC0530000, 0xC0530FFF, eos_handle_sddma, 3 },
    { "CFDMA",        0xC0620000, 0xC062FFFF, eos_handle_cfdma, 1 },
    { "TIO",          0xC0800000, 0xC08000FF, eos_handle_tio, 0 },
    { "SIO0",         0xC0820000, 0xC08200FF, eos_handle_sio, 0 },
    { "SIO1",         0xC0820100, 0xC08201FF, eos_handle_sio, 1 },
    { "SIO2",         0xC0820200, 0xC08202FF, eos_handle_sio, 2 },
    { "SIO3",         0xC0820300, 0xC08203FF, eos_handle_sio3, 3 },
    { "MREQ",         0xC0203000, 0xC02030FF, eos_handle_mreq, 0 },
    { "DMA1",         0xC0A10000, 0xC0A100FF, eos_handle_dma, 1 },
    { "DMA2",         0xC0A20000, 0xC0A200FF, eos_handle_dma, 2 },
    { "DMA3",         0xC0A30000, 0xC0A300FF, eos_handle_dma, 3 },
    { "DMA4",         0xC0A40000, 0xC0A400FF, eos_handle_dma, 4 },
    { "CARTRIDGE",    0xC0F24000, 0xC0F24FFF, eos_handle_cartridge, 0 },
    { "ASIF",         0xC0920000, 0xC0920FFF, eos_handle_asif, 4 },
    { "Display",      0xC0F14000, 0xC0F14FFF, eos_handle_display, 0 },

    { "DIGIC6",       0xD0000000, 0xDFFFFFFF, eos_handle_digic6, 0 },
    
    { "ML helpers",   0xCF123000, 0xCF123EFF, eos_handle_ml_helpers, 0 },
    { "FIO wrapper",  0xCF123F00, 0xCF123FFF, eos_handle_ml_fio, 0 },
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

static void reverse_bytes_order(uint8_t* buf, int count)
{
    short* buf16 = (short*) buf;
    int i;
    for (i = 0; i < count/2; i++)
    {
        short x = buf16[i];
        buf[2*i+1] = x;
        buf[2*i] = x >> 8;
    }
}

unsigned int eos_handle_ml_helpers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    if(type & MODE_WRITE)
    {
        switch (address)
        {
            case REG_PRINT_CHAR:    /* print in blue */
                printf("\x1B[34m%c\x1B[0m", (uint8_t)value);
                return 0;

            case REG_PRINT_NUM:     /* print in green */
                printf("\x1B[32m%x (%d)\x1B[0m\n", (uint32_t)value, (uint32_t)value);
                return 0;

            case REG_SHUTDOWN:
                printf("Goodbye!\n");
                qemu_system_shutdown_request();
                return 0;
            
            case REG_BMP_VRAM:
                s->disp.bmp_vram = (uint32_t) value;
                return 0;

            case REG_IMG_VRAM:
                s->disp.img_vram = (uint32_t) value;
                if (value)
                {
                    eos_load_image(s, "LV-000.422", 0, -1, value, 0);
                }
                else
                {
                    printf("Image buffer disabled\n");
                }
                return 0;
            
            case REG_RAW_BUFF:
                s->disp.raw_buff = (uint32_t) value;
                if (value)
                {
                    /* fixme: hardcoded strip offset */
                    eos_load_image(s, "RAW-000.DNG", 33792, -1, value, 1);
                }
                else
                {
                    printf("Raw buffer disabled\n");
                }
                return 0;

            case REG_DISP_TYPE:
                s->disp.type = (uint32_t) value;
                return 0;
        }
    }
    else
    {
        switch (address)
        {
            case REG_GET_KEY:
            {
                if (s->keyb.head == s->keyb.tail)
                {
                    /* no key in queue */
                    return 0;
                }
                else
                {
                    /* return a key from the circular buffer */
                    return s->keyb.buf[(s->keyb.head++) & 15];
                }
            }

            case REG_BMP_VRAM:
                return s->disp.bmp_vram;

            case REG_IMG_VRAM:
                return s->disp.img_vram;
            
            case REG_RAW_BUFF:
                return s->disp.raw_buff;
            
            case REG_DISP_TYPE:
                return s->disp.type;
        }
        return 0;
    }
    return 0;
}

static char* ml_fio_translate_filename(char* filename)
{
    static char buf[1000];
    char* f;
    
    for (f = filename; *f; f++)
    {
        if (*f == '.' && *(f+1) == '.')
        {
            printf("Hacker detected :) %s\n", filename);
            goto err;
        }
    }
    
    if (filename[0] == 'A' && filename[1] == ':' && filename[2] == '/')
    {
        snprintf(buf, sizeof(buf), "cfcard/%s", filename+3);
        goto ok;
    }
    
    if (filename[0] == 'B' && filename[1] == ':' && filename[2] == '/')
    {
        snprintf(buf, sizeof(buf), "sdcard/%s", filename+3);
        goto ok;
    }

err:
    printf("[FIO wrapper] invalid filename (%s)\n", filename);
    buf[0] = 0;
    return buf;

ok:
    /* files copied from a FAT32 card are all uppercase */
    for (f = buf+6; *f; f++)
    {
        *f = toupper(*f);
    }
    return buf;
}

unsigned int eos_handle_ml_fio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    static int arg0, arg1, arg2, arg3;
    static char buffer[1000] = {0};
    static uint32_t  buffer_index = 0;
    
    static char dirname[1000];
    static DIR * dir;
    static FILE* file;
    
    if(type & MODE_WRITE)
    {
        switch (address)
        {
            case REG_FIO_NUMERIC_ARG0:
                arg0 = value;
                return 0;

            case REG_FIO_NUMERIC_ARG1:
                arg1 = value;
                return 0;

            case REG_FIO_NUMERIC_ARG2:
                arg2 = value;
                return 0;

            case REG_FIO_NUMERIC_ARG3:
                arg3 = value;
                return 0;
            
            case REG_FIO_BUFFER:
                if (buffer_index < COUNT(buffer))
                {
                    buffer[buffer_index++] = value;
                }
                return 0;
            
            case REG_FIO_BUFFER_SEEK:
                buffer_index = value;
                return 0;
        }
    }
    else
    {
        switch (address)
        {
            case REG_FIO_NUMERIC_ARG0:
                return arg0;

            case REG_FIO_NUMERIC_ARG1:
                return arg1;

            case REG_FIO_NUMERIC_ARG2:
                return arg2;

            case REG_FIO_NUMERIC_ARG3:
                return arg3;

            case REG_FIO_BUFFER:
                if (buffer_index < COUNT(buffer))
                {
                    return buffer[buffer_index++];
                }

            case REG_FIO_GET_FILE_SIZE:
            {
                struct stat sb;
                char* filename = ml_fio_translate_filename(buffer);
                if (stat(filename, &sb) != -1)
                {
                    printf("[FIO wrapper] GetFileSize(%s) => %d\n", filename, (int)sb.st_size);
                    return sb.st_size;
                }
                else
                {
                    printf("[FIO wrapper] GetFileSize(%s) error\n", filename);
                    return -1;
                }
            }

            case REG_FIO_OPENFILE:
            {
                char* filename = ml_fio_translate_filename(buffer);

                if (file == 0)
                {
                    file = fopen(filename, "rb");
                    if (file)
                    {
                        printf("[FIO wrapper] openfile(%s)\n", filename);
                        return 1;
                    }
                    else
                    {
                        printf("[FIO wrapper] openfile(%s) error\n", filename);
                    }
                }
                else
                {
                    printf("[FIO wrapper] openfile(%s) already in use\n", filename);
                }
                return 0;
            }
            
            case REG_FIO_CLOSEFILE:
            {
                if (file)
                {
                    fclose(file);
                    file = 0;
                    printf("[FIO wrapper] closefile()\n");
                    return 1;
                }
                else
                {
                    printf("[FIO wrapper] closefile() nothing open\n");
                }
                return 0;
            }

            case REG_FIO_READFILE:
            {
                int size = MIN(arg0, sizeof(buffer));
                int pos = arg1;
                buffer[0] = buffer_index = 0;
                
                if (file)
                {
                    //~ printf("[FIO wrapper] reading %d bytes from pos=%d\n", size, pos);
                    fseek(file, pos, pos >= 0 ? SEEK_SET : SEEK_END);
                    return fread(buffer, 1, size, file);
                }
                else
                {
                    printf("[FIO wrapper] readfile() nothing open\n");
                }
                return 0;
            }
            
            case REG_FIO_OPENDIR:
            {
                char* path = ml_fio_translate_filename(buffer);

                if (dir == 0)
                {
                    dir = opendir(path);
                    if (dir)
                    {
                        int len = snprintf(dirname, sizeof(dirname), "%s", path);
                        if (dirname[len-1] == '/') dirname[len-1] = 0;
                        printf("[FIO wrapper] opendir(%s)\n", path);
                        return 1;
                    }
                    else
                    {
                        printf("[FIO wrapper] opendir(%s) error\n", path);
                    }
                }
                else
                {
                    printf("[FIO wrapper] opendir(%s) already in use\n", path);
                }
                return 0;
            }
            
            case REG_FIO_CLOSEDIR:
            {
                if (dir)
                {
                    closedir(dir);
                    dir = 0;
                    printf("[FIO wrapper] closedir()\n");
                    return 1;
                }
                else
                {
                    printf("[FIO wrapper] closedir() nothing open\n");
                }
                return 0;
            }

            case REG_FIO_READDIR:
            {
                buffer[0] = buffer_index = 0;
                arg0 = arg1 = arg2 = 0;
                
                if (dir)
                {
                    struct dirent * d = readdir(dir);
                    if (d)
                    {
                        snprintf(buffer, sizeof(buffer), "%s", d->d_name);

                        char fullpath[1000];
                        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, d->d_name);
                        
                        struct stat sb;
                        if (stat(fullpath, &sb) != -1)
                        {
                            arg0 = sb.st_size;
                            arg1 = S_ISDIR(sb.st_mode) ? 0x10 : 0;
                            arg2 = sb.st_mtime;
                        }
                        
                        printf("[FIO wrapper] readdir() => %s size=%d mode=%x time=%x\n", buffer, arg0, arg1, arg2);
                        return 1;
                    }
                }
                else
                {
                    printf("[FIO wrapper] closedir() nothing open\n");
                }
                return 0;
            }
        }
    }
    return 0;
}

static void eos_load_image(EOSState *s, const char* file, int offset, int max_size, uint32_t addr, int swap_endian)
{
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

    fprintf(stderr, "[EOS] loading '%s' to 0x%08X-0x%08X\n", file, addr, size + addr - 1);

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
    
    if (swap_endian) {
        reverse_bytes_order(buf + offset, size);
    }

    MEM_WRITE_ROM(addr, buf + offset, size);

    free(buf);
}

static void *eos_interrupt_thread(void *parm)
{
    EOSState *s = (EOSState *)parm;

    while (1)
    {
        uint32_t pos;

        usleep(0x100);

        qemu_mutex_lock(&s->irq_lock);

        s->digic_timer += 0x100;
        s->digic_timer &= 0xFFF00;
        
        for (pos = 0; pos < 3; pos++)
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
        for(pos = 0; pos < INT_ENTRIES; pos++)
        {
            /* it is pending, so trigger int and set to 0 */
            if(s->irq_schedule[pos] == 1)
            {
                /* wait, its not enabled. keep it pending */
                if(s->irq_enabled[pos] && !s->irq_id)
                {
                    /* timer interrupt will re-fire periodically */
                    if(pos == 0x0A)
                    {
                        //~ printf("[EOS] trigger int 0x%02X (delayed)\n", pos);
                        s->irq_schedule[pos] = s->timer_reload_value[2] >> 8;
                    }
                    else
                    {
                        printf("[EOS] trigger int 0x%02X (delayed)\n", pos);
                        s->irq_schedule[pos] = 0;
                    }

                    s->irq_id = pos;
                    s->irq_enabled[s->irq_id] = 0;
                    cpu_interrupt(CPU(s->cpu), CPU_INTERRUPT_HARD);
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
        int trigger_hptimers = 0;
        for (pos = 0; pos < 8; pos++)
        {
            if (s->HPTimers[pos].active && s->HPTimers[pos].output_compare == s->digic_timer)
            {
                printf("[HPTimer] Firing HPTimer %d/8\n", pos+1);
                s->HPTimers[pos].triggered = 1;
                trigger_hptimers = 1;
            }
        }
        
        qemu_mutex_unlock(&s->irq_lock);

        if (trigger_hptimers)
        {
            eos_trigger_int(s, 0x10, 0);
        }
    }

    return NULL;
}

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
        MemoryRegionSection section;
        section = memory_region_find(s->system_mem,s->disp.bmp_vram, size);
        framebuffer_update_display(
            surface,
            &section,
            width, height,
            360, linesize, 0, 1,
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
            960, yuv_width*2, linesize, 0, s->disp.invalidate,
            draw_line8_32_bmp_yuv, s,
            &first, &last
        );
    }
    else if (s->disp.bmp_vram) /* FIXME! nkls: had to add this to prevent segmentation fault */
    {
        uint64_t size = height * linesize;
        MemoryRegionSection section;
        section = memory_region_find(s->system_mem,s->disp.bmp_vram, size);
        framebuffer_update_display(
            surface,
            &section,
            width, height,
            960, linesize, 0, 1,
            draw_line8_32, s,
            &first, &last
        );
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
    s->keyb.buf[(s->keyb.tail++) & 15] = keycode;
}

static EOSState *eos_init_cpu(int digic_version)
{
    EOSState *s = g_new(EOSState, 1);
    memset(s, 0, sizeof(*s));

    s->verbosity = 0xFFFFFFFF;
    s->tio_rxbyte = 0x100;

    s->system_mem = get_system_memory();

    memory_region_init_ram(&s->tcm_code, NULL, "eos.tcm_code", TCM_SIZE, &error_abort);
    memory_region_add_subregion(s->system_mem, 0x00000000, &s->tcm_code);
    memory_region_init_ram(&s->tcm_data, NULL, "eos.tcm_data", TCM_SIZE, &error_abort);
    memory_region_add_subregion(s->system_mem, CACHING_BIT, &s->tcm_data);

    /* set up RAM, cached and uncached */
    memory_region_init_ram(&s->ram, NULL, "eos.ram", RAM_SIZE - TCM_SIZE, &error_abort);
    memory_region_add_subregion(s->system_mem, TCM_SIZE, &s->ram);
    memory_region_init_alias(&s->ram_uncached, NULL, "eos.ram_uncached", &s->ram, 0x00000000, RAM_SIZE - TCM_SIZE);
    memory_region_add_subregion(s->system_mem, CACHING_BIT | TCM_SIZE, &s->ram_uncached);

    if (digic_version == 6)
    {
        memory_region_init_ram(&s->ram2, NULL, "eos.ram2", RAM2_SIZE, &error_abort);
        memory_region_add_subregion(s->system_mem, RAM2_ADDR, &s->ram2);
    }

    /* set up ROM0 */
    memory_region_init_ram(&s->rom0, NULL, "eos.rom0", ROM0_SIZE, &error_abort);
    memory_region_add_subregion(s->system_mem, ROM0_ADDR, &s->rom0);

    uint64_t offset;
    for(offset = ROM0_ADDR + ROM0_SIZE; offset < ROM1_ADDR; offset += ROM0_SIZE)
    {
        char name[32];
        MemoryRegion *image = g_new(MemoryRegion, 1);
        sprintf(name, "eos.rom0_mirror_%02X", (uint32_t)offset >> 24);

        memory_region_init_alias(image, NULL, name, &s->rom0, 0x00000000, ROM0_SIZE);
        memory_region_add_subregion(s->system_mem, offset, image);
    }

    /* set up ROM1 */
    memory_region_init_ram(&s->rom1, NULL, "eos.rom1", ROM1_SIZE, &error_abort);
    memory_region_add_subregion(s->system_mem, ROM1_ADDR, &s->rom1);

    // uint64_t offset;
    for(offset = ROM1_ADDR + ROM1_SIZE; offset < 0x100000000; offset += ROM1_SIZE)
    {
        char name[32];
        MemoryRegion *image = g_new(MemoryRegion, 1);
        sprintf(name, "eos.rom1_mirror_%02X", (uint32_t)offset >> 24);

        memory_region_init_alias(image, NULL, name, &s->rom1, 0x00000000, ROM1_SIZE);
        memory_region_add_subregion(s->system_mem, offset, image);
    }

    //memory_region_init_ram(&s->rom1, "eos.rom", 0x10000000, &error_abort);
    //memory_region_add_subregion(s->system_mem, 0xF0000000, &s->rom1);

    /* set up io space */
    uint32_t io_mem_len = (digic_version == 6 ? IO_MEM_LEN6 : IO_MEM_LEN45);
    memory_region_init_io(&s->iomem, NULL, &iomem_ops, s, "eos.iomem", io_mem_len);
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

    const char* cpu_name = (digic_version == 6) ? "arm-digic6-eos" : "arm946eos";
    
    s->cpu = cpu_arm_init(cpu_name);
    if (!s->cpu)
    {
        fprintf(stderr, "Unable to find CPU definition\n");
        exit(1);
    }

    s->rtc.transfer_format = 0xFF;

    qemu_mutex_init(&s->irq_lock);
    qemu_thread_create(&s->interrupt_thread_id, "eos_interrupt", eos_interrupt_thread, s, QEMU_THREAD_JOINABLE);

    s->disp.con = graphic_console_init(NULL, 0, &eos_display_ops, s);
    
    qemu_add_kbd_event_handler(eos_key_event, s);

    return s;
}

static void patch_bootloader_autoexec(EOSState *s)
{
    /* on 6D, patch bootdisk_check and file_read so it will believe it can read autoexec.bin */
    if (eos_get_mem_w(s, 0xFFFEA10C) != 0xE92D41F0 ||
        eos_get_mem_w(s, 0xFFFE23CC) != 0xE92D41F0)
    {
        printf("This ROM doesn't look like a 6D\n");
        return;
    }
    uint32_t ret_0[2] = { 0xe3a00000, 0xe12fff1e };
    MEM_WRITE_ROM(0xFFFEA10C, (uint8_t*) ret_0, 8);
    MEM_WRITE_ROM(0xFFFE23CC, (uint8_t*) ret_0, 8);
    eos_load_image(s, "autoexec.bin", 0, -1, 0x40800000, 0);
    s->cpu->env.regs[15] = 0xFFFF0000;
}

#if 0 /* nkls: removed because it won't compile with newer QEMU */
static void patch_7D2(EOSState *s)
{
    int is_7d2m = (eos_get_mem_w(s, 0xFE106062) == 0x0F31EE19);

    uint32_t nop = 0;
    uint32_t addr;
    for (addr = 0xFE000000; addr < 0xFE200000; addr += 2)
    {
        uint32_t old = eos_get_mem_w(s, addr);
        if (old == 0x0F12EE06   /* MCR p15, 0, R0,c6,c2, 0 */
         || old == 0x1F91EE06   /* MCR p15, 0, R1,c6,c1, 4 */
         || old == 0x0F11EE19   /* MRC p15, 0, R0,c9,c1, 0 */
         || old == 0x0F11EE09   /* MCR p15, 0, R0,c9,c1, 0 */
         || old == 0x0F10EE11   /* MRC p15, 0, R0,c1,c0, 0 */
         || old == 0x0F31EE19   /* MRC p15, 0, R0,c9,c1, 1 */
         || old == 0x0F90EE10   /* MRC p15, 0, R0,c0,c0, 4 */
        ) {
            printf("Patching ");
            target_disas(stdout, &s->cpu->env, addr, 4, 1);
            MEM_WRITE_ROM(addr, (uint8_t*) &nop, 4);
        }
    }

    if (is_7d2m)
    {
        uint32_t one = 1;
        printf("Patching 0x%X (enabling TIO on 7D2M)\n", 0xFEC4DCBC);
        MEM_WRITE_ROM(0xFEC4DCBC, (uint8_t*) &one, 4);
    }
    else
    {
        printf("This ROM doesn't look like a 7D2M\n");
    }
}
#endif

static void eos_init_common(const char *rom_filename, uint32_t rom_start, uint32_t digic_version)
{
    precompute_yuv2rgb(1);

    EOSState *s = eos_init_cpu(digic_version);

    /* populate ROM0 */
    eos_load_image(s, rom_filename, 0, ROM0_SIZE, ROM0_ADDR, 0);
    /* populate ROM1 */
    eos_load_image(s, rom_filename, ROM0_SIZE, ROM1_SIZE, ROM1_ADDR, 0);

    /* init SD card */
    DriveInfo *di;
    /* FIXME use a qdev drive property instead of drive_get_next() */
    di = drive_get_next(IF_SD);
    s->sd.card = sd_init(di ? blk_by_legacy_dinfo(di) : NULL, false);
    if (!s->sd.card) {
        printf("SD init failed\n");
        exit(1);
    }

    if (0)
    {
        /* 6D bootloader experiment */
        patch_bootloader_autoexec(s);
        return;
    }
    
    if (1)
    {
        /* make sure the boot flag is enabled */
        uint32_t flag = 0xFFFFFFFF;
        MEM_WRITE_ROM(0xF8000004, (uint8_t*) &flag, 4);

        /* emulate the bootloader, not the main firmware */
        s->cpu->env.regs[15] = 0xFFFF0000;
        return;
    }
    
    
#if 0
        /* 7D2 experiments */
        patch_7D2(s);
#endif

    s->cpu->env.regs[15] = rom_start;
}

static void ml_init_common(const char *rom_filename, uint32_t rom_start, uint32_t digic_version)
{
    precompute_yuv2rgb(1);

    EOSState *s = eos_init_cpu(digic_version);

    /* populate ROM0 */
    eos_load_image(s, rom_filename, 0, ROM0_SIZE, ROM0_ADDR, 0);
    /* populate ROM1 */
    eos_load_image(s, rom_filename, ROM0_SIZE, ROM1_SIZE, ROM1_ADDR, 0);

    eos_load_image(s, "autoexec.bin", 0, -1, 0x40800000, 0);

    /* we will replace Canon stubs with our own implementations */
    /* see qemu-helper.c, stub_mappings[] */
    eos_load_image(s, "qemu-helper.bin", 0, -1, Q_HELPER_ADDR, 0);
    uint32_t magic  = 0x12345678;
    uint32_t addr   = Q_HELPER_ADDR;
    while (eos_get_mem_w(s, addr) != magic || eos_get_mem_w(s, addr + 4) != magic)
    {
        addr += 4;
        if (addr > 0x30100000) { fprintf(stderr, "stub list not found\n"); abort(); }
    }
    uint32_t ram_offset = eos_get_mem_w(s, addr + 8);
    addr += 12;
    while (eos_get_mem_w(s, addr) != magic || eos_get_mem_w(s, addr + 4) != magic)
    {
        uint32_t old = eos_get_mem_w(s, addr);
        uint32_t new = eos_get_mem_w(s, addr + 4);
        if (old < 0xFF000000)
            old += ram_offset;
        uint32_t jmp[] = {FAR_CALL_INSTR, new};
        printf("[QEMU_HELPER] stub %x -> %x (%x)\n", old, new, eos_get_mem_w(s, old));
        MEM_WRITE_ROM(old, (uint8_t*)jmp, 8);

        addr += 8;
        if (addr > 0x30100000) { fprintf(stderr, "stub list error\n"); abort(); }
    }

    // set entry point
    s->cpu->env.regs[15] = 0x800000;
    s->cpu->env.regs[13] = 0x1900;
}

machine_init(eos_machine_init);

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

static void io_log(const char * module_name, EOSState *s, unsigned int address, unsigned char type, unsigned int in_value, unsigned int out_value, const char * msg, intptr_t msg_arg1, intptr_t msg_arg2)
{
    unsigned int pc = s->cpu->env.regs[15];
    if (!module_name) module_name = "???";
    if (!msg) msg = "???";
    
    char mod_name[50];
    char mod_name_and_pc[50];
    snprintf(mod_name, sizeof(mod_name), "[%s]", module_name);
    snprintf(mod_name_and_pc, sizeof(mod_name_and_pc), "%-10s at 0x%08X", mod_name, pc);
    
    /* description may have two optional integer arguments */
    char desc[200];
    snprintf(desc, sizeof(desc), msg, msg_arg1, msg_arg2);
    
    printf("%-28s [0x%08X] %s 0x%-8X%s%s\n",
        mod_name_and_pc,
        address,
        type & MODE_WRITE ? "<-" : "->",
        type & MODE_WRITE ? in_value : out_value,
        strlen(msg) ? ": " : "",
        desc
    );
}

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
        printf("[EOS] trigger int 0x%02X\n", id);
        s->irq_id = id;
        s->irq_enabled[s->irq_id] = 0;
        cpu_interrupt(CPU(s->cpu), CPU_INTERRUPT_HARD);
    }
    else
    {
        printf("[EOS] trigger int 0x%02X (delayed!)\n", id);
        if(!s->irq_enabled[id])
        {
            delay = 1;
        }
        s->irq_schedule[id] = MAX(delay, 1);
    }

    qemu_mutex_unlock(&s->irq_lock);
    return 0;
}

unsigned int eos_handle_intengine ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    int msg_arg1 = 0;
    int msg_arg2 = 0;
    unsigned int ret = 0;

    switch(address & 0xFFF)
    {
        case 0x04:
            if(type & MODE_WRITE)
            {
                msg = "Wrote int reason ???";
            }
            else
            {
                msg = "Requested int reason %x (INT %02Xh)";
                msg_arg1 = s->irq_id << 2;
                msg_arg2 = s->irq_id;
                ret = s->irq_id << 2;

                /* this register resets on read (subsequent reads should report 0) */
                s->irq_id = 0;
                cpu_reset_interrupt(CPU(s->cpu), CPU_INTERRUPT_HARD);

                if(msg_arg2 == 0x0A)
                {
                    /* timer interrupt, quiet */
                    return ret;
                }
            }
            break;

        case 0x10:
            if(type & MODE_WRITE)
            {
                msg = "Enabled interrupt %02Xh";
                msg_arg1 = value;
                s->irq_enabled[value] = 1;

                /* we shouldn't reset s->irq_id here (we already reset it on read) */
                /* if we reset it here also, it will trigger interrupt 0 incorrectly (on race conditions) */

                if (value == 0x0A)
                {
                    /* timer interrupt, quiet */
                    return 0;
                }
            }
            break;
    }

    io_log("INT", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

unsigned int eos_handle_timers_ ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int pc = s->cpu->env.regs[15];

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

    int timer_id = (address & 0xF00) >> 8;
    msg_arg1 = timer_id;
    
    if (timer_id < 3)
    {
        switch(address & 0xFF)
        {
            case 0x00:
                if(type & MODE_WRITE)
                {
                    if(value & 1)
                    {
                        if (timer_id == 2)
                        {
                            msg = "Timer #%d: starting triggering";
                            eos_trigger_int(s, 0x0A, s->timer_reload_value[timer_id] >> 8);   /* digic timer */
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
                if(type & MODE_WRITE)
                {
                    s->timer_reload_value[timer_id] = value;
                    msg = "Timer #%d: will trigger every %d ms";
                    msg_arg2 = ((uint64_t)value + 1) / 1000;
                }
                break;
            
            case 0x0C:
                if(type & MODE_WRITE)
                {
                }
                else
                {
                    msg = "Timer #%d: current value";
                    ret = s->timer_current_value[timer_id];
                }
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
    int timer_id = ((address & 0xF0) >> 4) - 6;
    msg_arg1 = timer_id + 1;

    switch(address & 0xFFF)
    {
        case 0x160:
        case 0x170:
        case 0x180:
        case 0x190:
        case 0x1A0:
        case 0x1B0:
        case 0x1C0:
        case 0x1D0:
            if(type & MODE_WRITE)
            {
                s->HPTimers[timer_id].active = value;

                msg = value == 1 ? "HPTimer %d/8: active" :
                      value == 0 ? "HPTimer %d/8: inactive" :
                                   "???";
            }
            else
            {
                ret = s->HPTimers[timer_id].active;
                msg = "HPTimer %d/8: status?";
            }
            break;

        case 0x164:
        case 0x174:
        case 0x184:
        case 0x194:
        case 0x1A4:
        case 0x1B4:
        case 0x1C4:
        case 0x1D4:
            if(type & MODE_WRITE)
            {
                s->HPTimers[timer_id].output_compare = value & 0xFFF00;
                msg_arg2 = value;
                msg = "HPTimer %d/8: output compare %d microseconds";
            }
            else
            {
                ret = s->HPTimers[timer_id].output_compare;
                msg = "HPTimer %d/8: output compare flags?";
            }
            break;

        case 0x260:
        case 0x270:
        case 0x280:
        case 0x290:
        case 0x2A0:
        case 0x2B0:
        case 0x2C0:
        case 0x2D0:
            msg = "HPTimer %d/8: ?!";
            break;

        case 0x264:
        case 0x274:
        case 0x284:
        case 0x294:
        case 0x2A4:
        case 0x2B4:
        case 0x2C4:
        case 0x2D4:
            msg = "HPTimer %d/8: ???";
            if(type & MODE_WRITE)
            {
                msg = "HPTimer %d/8: reset trigger?";
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
                    if (s->HPTimers[i].triggered)
                        ret |= 1 << (2*i+4);
                
                msg = "Which timer(s) triggered";
            }
            break;
    }

    io_log("HPTimer", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    qemu_mutex_unlock(&s->irq_lock);
    return ret;
}

#define MPU_CURRENT_OUT_SPELL mpu_init_spells[s->mpu.spell_set].out_spells[s->mpu.out_spell]

/**
 * We don't know the meaning of MPU messages yet, so we'll replay them from a log file.
 * Timing is important - if we would just send everything as a response to first message,
 * the tasks that handle that message may not be started yet.
 * 
 * We will attempt to guess a causal relationship between mpu_send and mpu_recv calls.
 * Although not perfect, that guess is a good starting point.
 * 
 * These values are valid for a 60D.
 */

struct mpu_init_spell mpu_init_spells[] = { {
    { 0x06, 0x04, 0x02, 0x00, 0x00 }, {                         /* spell #1 */
        { 0x08, 0x07, 0x01, 0x33, 0x09, 0x00, 0x00, 0x00 },     /* reply #1.1 */
        { 0x06, 0x05, 0x01, 0x20, 0x00, 0x00 },                 /* reply #1.2 */
        { 0x06, 0x05, 0x01, 0x21, 0x01, 0x00 },                 /* reply #1.3 */
        { 0x06, 0x05, 0x01, 0x22, 0x00, 0x00 },                 /* reply #1.4 */
        { 0x06, 0x05, 0x03, 0x0c, 0x01, 0x00 },                 /* reply #1.5 */
        { 0x06, 0x05, 0x03, 0x0d, 0x01, 0x00 },                 /* reply #1.6 */
        { 0x06, 0x05, 0x03, 0x0e, 0x01, 0x00 },                 /* reply #1.7 */
        { 0x08, 0x06, 0x01, 0x23, 0x00, 0x01, 0x00 },           /* reply #1.8 */
        { 0x08, 0x06, 0x01, 0x24, 0x00, 0x00, 0x00 },           /* reply #1.9 */
        { 0x08, 0x06, 0x01, 0x25, 0x00, 0x01, 0x00 },           /* reply #1.10 */
        { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 },                 /* reply #1.11 */
        { 0x06, 0x05, 0x01, 0x2c, 0x02, 0x00 },                 /* reply #1.12 */
        { 0x06, 0x05, 0x03, 0x20, 0x04, 0x00 },                 /* reply #1.13 */
        { 0x06, 0x05, 0x01, 0x3d, 0x00, 0x00 },                 /* reply #1.14 */
        { 0x06, 0x05, 0x01, 0x42, 0x00, 0x00 },                 /* reply #1.15 */
        { 0x06, 0x05, 0x01, 0x00, 0x03, 0x00 },                 /* reply #1.16 */
        { 0x2c, 0x2a, 0x02, 0x00, 0x03, 0x03, 0x03, 0x04, 0x03, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x14, 0x50, 0x00, 0x00, 0x00, 0x00, 0x81, 0x06, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x04, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x4b, 0x01 },/* reply #1.17 */
        { 0x0c, 0x0b, 0x01, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.18 */
        { 0x06, 0x05, 0x01, 0x37, 0x00, 0x00 },                 /* reply #1.19 */
        { 0x06, 0x05, 0x01, 0x49, 0x01, 0x00 },                 /* reply #1.20 */
        { 0x06, 0x05, 0x01, 0x3e, 0x00, 0x00 },                 /* reply #1.21 */
        { 0x08, 0x06, 0x01, 0x45, 0x00, 0x10, 0x00 },           /* reply #1.22 */
        { 0x06, 0x05, 0x01, 0x48, 0x01, 0x00 },                 /* reply #1.23 */
        { 0x06, 0x05, 0x01, 0x4b, 0x01, 0x00 },                 /* reply #1.24 */
        { 0x06, 0x05, 0x01, 0x40, 0x00, 0x00 },                 /* reply #1.25 */
        { 0x06, 0x05, 0x01, 0x41, 0x00, 0x00 },                 /* reply #1.26 */
        { 0x06, 0x05, 0x01, 0x3f, 0x00, 0x00 },                 /* reply #1.27 */
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.28 */
        { 0x06, 0x05, 0x01, 0x48, 0x01, 0x00 },                 /* reply #1.29 */
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },                 /* reply #1.30 */
        { 0x06, 0x05, 0x01, 0x4a, 0x00, 0x00 },                 /* reply #1.31 */
        { 0x06, 0x05, 0x01, 0x50, 0x03, 0x00 },                 /* reply #1.32 */
        { 0x08, 0x06, 0x01, 0x51, 0x70, 0x48, 0x00 },           /* reply #1.33 */
        { 0x06, 0x05, 0x01, 0x52, 0x00, 0x00 },                 /* reply #1.34 */
        { 0x06, 0x05, 0x01, 0x54, 0x00, 0x00 },                 /* reply #1.35 */
        { 0x06, 0x05, 0x03, 0x37, 0x00, 0x00 },                 /* reply #1.36 */
        { 0x0e, 0x0c, 0x02, 0x05, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.37 */
        { 0x0a, 0x08, 0x02, 0x06, 0x04, 0x00, 0x00, 0x00, 0x00 },/* reply #1.38 */
        { 0x0c, 0x0a, 0x02, 0x07, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.39 */
        { 0x0c, 0x0a, 0x02, 0x08, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.40 */
        { 0x0a, 0x08, 0x03, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.41 */
        { 0x06, 0x05, 0x03, 0x05, 0x02, 0x00 },                 /* reply #1.42 */
        { 0x1e, 0x1c, 0x03, 0x30, 0x65, 0x65, 0x50, 0x50, 0x53, 0x53, 0x53, 0x53, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00 },/* reply #1.43 */
        { 0x0e, 0x0c, 0x03, 0x2e, 0x00, 0x00, 0x83, 0xad, 0x00, 0x00, 0xdb, 0x71, 0x00 },/* reply #1.44 */
        { 0x06, 0x05, 0x03, 0x35, 0x01, 0x00 },                 /* reply #1.45 */
        { 0x1c, 0x1b, 0x03, 0x1d, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x50, 0x2d, 0x45, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xae, 0x7e, 0x3b, 0x61, 0x00 },/* reply #1.46 */
        { 0x06, 0x04, 0x03, 0x36, 0x00 },                       /* reply #1.47 */
        { 0 } } }, {
    { 0x08, 0x06, 0x00, 0x00, 0x02, 0x00, 0x00 }, {             /* spell #2 */
        { 0x08, 0x07, 0x01, 0x55, 0x00, 0x02, 0x01, 0x01 },     /* reply #2.1 */
        { 0 } } }, {
    { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 }, {                   /* spell #3 */
        { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 },                 /* reply #3.1 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x40, 0x00, 0x00 }, {                   /* spell #4 */
        { 0x06, 0x05, 0x03, 0x38, 0x95, 0x00 },                 /* reply #4.1 */
        { 0 } } }, {
    { 0x08, 0x06, 0x01, 0x24, 0x00, 0x01, 0x00 }, {             /* spell #5 */
        { 0x08, 0x06, 0x01, 0x24, 0x00, 0x01, 0x00 },           /* reply #5.1 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x0c, 0x00, 0x00 }, {                   /* spell #6 */
        { 0x06, 0x05, 0x01, 0x2c, 0x02, 0x00 },                 /* reply #6.1 */
        { 0x0a, 0x08, 0x03, 0x00, 0x6c, 0x00, 0x00, 0x2f, 0x00 },/* reply #6.2 */
        { 0x06, 0x05, 0x03, 0x04, 0x00, 0x00 },                 /* reply #6.3 */
        { 0x1a, 0x18, 0x03, 0x15, 0x01, 0x2d, 0x58, 0x00, 0x30, 0x00, 0x12, 0x00, 0x37, 0x91, 0x75, 0x92, 0x1f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 },/* reply #6.4 */
        { 0x24, 0x22, 0x03, 0x3c, 0x00, 0x00, 0x88, 0xb5, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #6.5 */
        { 0x06, 0x05, 0x03, 0x17, 0x92, 0x00 },                 /* reply #6.6 */
        { 0x06, 0x05, 0x03, 0x23, 0x19, 0x00 },                 /* reply #6.7 */
        { 0x1e, 0x1d, 0x03, 0x24, 0x45, 0x46, 0x2d, 0x53, 0x31, 0x38, 0x2d, 0x35, 0x35, 0x6d, 0x6d, 0x20, 0x66, 0x2f, 0x33, 0x2e, 0x35, 0x2d, 0x35, 0x2e, 0x36, 0x20, 0x49, 0x53, 0x00, 0x00 },/* reply #6.8 */
        { 0x06, 0x04, 0x03, 0x25, 0x00 },                       /* reply #6.9 */
        { 0x06, 0x05, 0x01, 0x3d, 0x00, 0x00 },                 /* reply #6.10 */
        { 0x06, 0x05, 0x03, 0x37, 0x00, 0x00 },                 /* reply #6.11 */
        { 0x06, 0x05, 0x03, 0x0d, 0x00, 0x00 },                 /* reply #6.12 */
        { 0x1a, 0x18, 0x03, 0x15, 0x01, 0x2d, 0x58, 0x00, 0x30, 0x00, 0x12, 0x00, 0x37, 0x91, 0x75, 0x92, 0x1f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 },/* reply #6.13 */
        { 0x06, 0x05, 0x03, 0x0c, 0x00, 0x00 },                 /* reply #6.14 */
        { 0 } } }, {
    { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 }, {/* spell #7 */
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #7.1 */
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },                 /* reply #7.2 */
        { 0 } } }, {
    { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 }, {/* spell #8 */
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #8.1 */
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },                 /* reply #8.2 */
        { 0 } } }, {
    { 0x06, 0x05, 0x02, 0x0a, 0x01, 0x00 }, {                   /* spell #9 */
        { 0x06, 0x05, 0x06, 0x11, 0x01, 0x00 },                 /* reply #9.1 */
        { 0x06, 0x05, 0x06, 0x12, 0x00, 0x00 },                 /* reply #9.2 */
        { 0x06, 0x05, 0x06, 0x13, 0x00, 0x00 },                 /* reply #9.3 */
        { 0x42, 0x41, 0x0a, 0x08, 0xff, 0x1f, 0x01, 0x00, 0x01, 0x01, 0xa0, 0x10, 0x00, 0x4d, 0x01, 0x01, 0x58, 0x2d, 0x4b, 0x01, 0x01, 0x00, 0x48, 0x04, 0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #9.4 */
        { 0x06, 0x05, 0x04, 0x0e, 0x01, 0x00 },                 /* reply #9.5 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x1d, 0x1f, 0x00 }, {                   /* spell #10 */
        { 0x06, 0x05, 0x03, 0x35, 0x01, 0x00 },                 /* reply #10.1 */
        { 0x1c, 0x1b, 0x03, 0x1d, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x50, 0x2d, 0x45, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xae, 0x7e, 0x3b, 0x61, 0x00 },/* reply #10.2 */
        { 0x06, 0x04, 0x03, 0x36, 0x00 },                       /* reply #10.3 */
        { 0 } } }, {
    { 0x06, 0x05, 0x08, 0x06, 0xff, 0x00 }, {                   /* spell #11 */
        { 0x06, 0x05, 0x08, 0x06, 0x00, 0x00 },                 /* reply #11.1 */
    { 0 } } }
};

/**
 * Alternative version: send everything after the first message,
 * with one exception: delay GUI-related messages.
 */
struct mpu_init_spell mpu_init_spells_alt[] = { {
    { 0x06, 0x04, 0x02, 0x00, 0x00 }, {
        { 0x08, 0x07, 0x01, 0x33, 0x09, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x20, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x21, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x22, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x0c, 0x01, 0x00 },
        { 0x06, 0x05, 0x03, 0x0d, 0x01, 0x00 },
        { 0x06, 0x05, 0x03, 0x0e, 0x01, 0x00 },
        { 0x08, 0x06, 0x01, 0x23, 0x00, 0x01, 0x00 },
        { 0x08, 0x06, 0x01, 0x24, 0x00, 0x00, 0x00 },
        { 0x08, 0x06, 0x01, 0x25, 0x00, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x2c, 0x02, 0x00 },
        { 0x06, 0x05, 0x03, 0x20, 0x04, 0x00 },
        { 0x06, 0x05, 0x01, 0x3d, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x42, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x00, 0x03, 0x00 },
        { 0x2c, 0x2a, 0x02, 0x00, 0x03, 0x03, 0x03, 0x04, 0x03, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x14, 0x50, 0x00, 0x00, 0x00, 0x00, 0x81, 0x06, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x04, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x4b, 0x01 },
        { 0x0c, 0x0b, 0x01, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x37, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x49, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x3e, 0x00, 0x00 },
        { 0x08, 0x06, 0x01, 0x45, 0x00, 0x10, 0x00 },
        { 0x06, 0x05, 0x01, 0x48, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x4b, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x40, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x41, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x3f, 0x00, 0x00 },
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x48, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x4a, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x50, 0x03, 0x00 },
        { 0x08, 0x06, 0x01, 0x51, 0x70, 0x48, 0x00 },
        { 0x06, 0x05, 0x01, 0x52, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x54, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x37, 0x00, 0x00 },
        { 0x0e, 0x0c, 0x02, 0x05, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x0a, 0x08, 0x02, 0x06, 0x04, 0x00, 0x00, 0x00, 0x00 },
        { 0x0c, 0x0a, 0x02, 0x07, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x0c, 0x0a, 0x02, 0x08, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x0a, 0x08, 0x03, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x05, 0x02, 0x00 },
        { 0x1e, 0x1c, 0x03, 0x30, 0x65, 0x65, 0x50, 0x50, 0x53, 0x53, 0x53, 0x53, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00 },
        { 0x0e, 0x0c, 0x03, 0x2e, 0x00, 0x00, 0x83, 0xad, 0x00, 0x00, 0xdb, 0x71, 0x00 },
        { 0x06, 0x05, 0x03, 0x35, 0x01, 0x00 },
        { 0x1c, 0x1b, 0x03, 0x1d, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x50, 0x2d, 0x45, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xae, 0x7e, 0x3b, 0x61, 0x00 },
        { 0x06, 0x04, 0x03, 0x36, 0x00 },
        { 0x08, 0x07, 0x01, 0x55, 0x00, 0x02, 0x01, 0x01 },
        { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 },
        { 0x06, 0x05, 0x03, 0x38, 0x95, 0x00 },
        { 0x08, 0x06, 0x01, 0x24, 0x00, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x2c, 0x02, 0x00 },
        { 0x0a, 0x08, 0x03, 0x00, 0x6c, 0x00, 0x00, 0x2f, 0x00 },
        { 0x06, 0x05, 0x03, 0x04, 0x00, 0x00 },
        { 0x1a, 0x18, 0x03, 0x15, 0x01, 0x2d, 0x58, 0x00, 0x30, 0x00, 0x12, 0x00, 0x37, 0x91, 0x75, 0x92, 0x1f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 },
        { 0x24, 0x22, 0x03, 0x3c, 0x00, 0x00, 0x88, 0xb5, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x17, 0x92, 0x00 },
        { 0x06, 0x05, 0x03, 0x23, 0x19, 0x00 },
        { 0x1e, 0x1d, 0x03, 0x24, 0x45, 0x46, 0x2d, 0x53, 0x31, 0x38, 0x2d, 0x35, 0x35, 0x6d, 0x6d, 0x20, 0x66, 0x2f, 0x33, 0x2e, 0x35, 0x2d, 0x35, 0x2e, 0x36, 0x20, 0x49, 0x53, 0x00, 0x00 },
        { 0x06, 0x04, 0x03, 0x25, 0x00 },
        { 0x06, 0x05, 0x01, 0x3d, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x37, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x0d, 0x00, 0x00 },
        { 0x1a, 0x18, 0x03, 0x15, 0x01, 0x2d, 0x58, 0x00, 0x30, 0x00, 0x12, 0x00, 0x37, 0x91, 0x75, 0x92, 0x1f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 },
        { 0x06, 0x05, 0x03, 0x0c, 0x00, 0x00 },
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },
        { 0 } } }, {
    { 0x06, 0x05, 0x02, 0x0a, 0x01, 0x00 }, {       /* wait for this message before continuing */
        { 0x06, 0x05, 0x06, 0x11, 0x01, 0x00 },     /* although not correct (it's probably sensor cleaning related), this trick appears to launch the GUI */
        { 0x06, 0x05, 0x06, 0x12, 0x00, 0x00 },
        { 0x06, 0x05, 0x06, 0x13, 0x00, 0x00 },
        { 0x42, 0x41, 0x0a, 0x08, 0xff, 0x1f, 0x01, 0x00, 0x01, 0x01, 0xa0, 0x10, 0x00, 0x4d, 0x01, 0x01, 0x58, 0x2d, 0x4b, 0x01, 0x01, 0x00, 0x48, 0x04, 0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x04, 0x0e, 0x01, 0x00 },
        { 0x06, 0x05, 0x03, 0x35, 0x01, 0x00 },
        { 0x1c, 0x1b, 0x03, 0x1d, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x50, 0x2d, 0x45, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xae, 0x7e, 0x3b, 0x61, 0x00 },
        { 0x06, 0x04, 0x03, 0x36, 0x00 },
        { 0x06, 0x05, 0x08, 0x06, 0x00, 0x00 },
    { 0 } } }
};

static void mpu_send_next_spell(EOSState *s)
{
    if (s->mpu.sq_head != s->mpu.sq_tail)
    {
        /* get next spell from the queue */
        s->mpu.spell_set = s->mpu.send_queue[s->mpu.sq_head].spell_set;
        s->mpu.out_spell = s->mpu.send_queue[s->mpu.sq_head].out_spell;
        s->mpu.sq_head = (s->mpu.sq_head+1) & (COUNT(s->mpu.send_queue)-1);
        printf("[MPU] Sending spell #%d.%d ( ", s->mpu.spell_set+1, s->mpu.out_spell+1);

        int i;
        for (i = 0; i < MPU_CURRENT_OUT_SPELL[0]; i++)
        {
            printf("%02x ", MPU_CURRENT_OUT_SPELL[i]);
        }
        printf(")\n");

        s->mpu.out_char = -2;

        /* request a SIO3 interrupt */
        eos_trigger_int(s, 0x36, 0);
    }
    else
    {
        printf("[MPU] Nothing more to send.\n");
        s->mpu.sending = 0;
    }
}

static void mpu_enqueue_spell(EOSState *s, int spell_set, int out_spell)
{
    int next_tail = (s->mpu.sq_tail+1) & (COUNT(s->mpu.send_queue)-1);
    if (next_tail != s->mpu.sq_head)
    {
        printf("[MPU] Queueing spell #%d.%d\n", spell_set+1, out_spell+1);
        s->mpu.send_queue[s->mpu.sq_tail].spell_set = spell_set;
        s->mpu.send_queue[s->mpu.sq_tail].out_spell = out_spell;
        s->mpu.sq_tail = next_tail;
    }
    else
    {
        printf("[MPU] ERROR: send queue full\n");
    }
}


static void mpu_interpret_command(EOSState *s)
{
    printf("[MPU] Received: ");
    int i;
    for (i = 0; i < s->mpu.recv_index; i++)
    {
        printf("%02x ", s->mpu.recv_buffer[i]);
    }
    
    int spell_set;
    for (spell_set = 0; spell_set < COUNT(mpu_init_spells); spell_set++)
    {
        if (memcmp(s->mpu.recv_buffer+1, mpu_init_spells[spell_set].in_spell+1, s->mpu.recv_buffer[1]) == 0)
        {
            printf(" (recognized spell #%d)\n", spell_set+1);
            
            int out_spell;
            for (out_spell = 0; mpu_init_spells[spell_set].out_spells[out_spell][0]; out_spell++)
            {
                mpu_enqueue_spell(s, spell_set, out_spell);
            }
            
            if (!s->mpu.sending)
            {
                s->mpu.sending = 1;
                
                /* request a MREQ interrupt */
                eos_trigger_int(s, 0x50, 0);
            }
            return;
        }
    }
    
    printf(" (unknown spell)\n");
}

static void mpu_handle_sio3_interrupt(EOSState *s)
{
    if (s->mpu.sending)
    {
        int num_chars = MPU_CURRENT_OUT_SPELL[0];
        
        if (num_chars)
        {
            /* next two num_chars */
            s->mpu.out_char += 2;
            
            if (s->mpu.out_char < num_chars)
            {
                /*
                printf(
                    "[MPU] Sending spell #%d.%d, chars %d & %d out of %d\n", 
                    s->mpu.spell_set+1, s->mpu.out_spell+1,
                    s->mpu.out_char+1, s->mpu.out_char+2,
                    num_chars
                );
                */
                
                if (s->mpu.out_char + 2 < num_chars)
                {
                    eos_trigger_int(s, 0x36, 0);   /* SIO3 */
                }
                else
                {
                    printf("[MPU] spell #%d.%d finished\n", s->mpu.spell_set+1, s->mpu.out_spell+1);

                    if (s->mpu.sq_head != s->mpu.sq_tail)
                    {
                        printf("[MPU] Requesting next spell\n");
                        eos_trigger_int(s, 0x50, 0);   /* MREQ */
                    }
                    else
                    {
                        /* no more spells */
                        printf("[MPU] spells finished\n");
                        s->mpu.sending = 0;
                    }
                }
            }
        }
    }

    if (s->mpu.receiving)
    {
        if (s->mpu.recv_index < s->mpu.recv_buffer[0])
        {
            /* more data to receive */
            printf("[MPU] Request more data\n");
            eos_trigger_int(s, 0x36, 0);   /* SIO3 */
        }
    }
}

static void mpu_handle_mreq_interrupt(EOSState *s)
{
    if (s->mpu.sending)
    {
        mpu_send_next_spell(s);
    }
    
    if (s->mpu.receiving)
    {
        if (s->mpu.recv_index == 0)
        {
            printf("[MPU] receiving next message\n");
        }
        else
        {
            /* if a message is started in SIO3, it should continue with SIO3's, without triggering another MREQ */
            /* it appears to be harmless,  but I'm not sure what happens with more than 1 message queued */
            printf("[MPU] next message was started in SIO3\n");
        }
        eos_trigger_int(s, 0x36, 0);   /* SIO3 */
    }
}

static unsigned int eos_handle_mpu(unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    /* C022009C - MPU request/status
     * - set to 0x46 at startup
     * - bic 0x2 in mpu_send
     * - orr 0x2 at the end of a message sent to the MPU (SIO3_ISR, get_data_to_send)
     * - tst 0x2 in SIO3 and MREQ ISRs
     * - should return 0x44 when sending data to MPU
     * - and 0x47 when receiving data from MPU
     */
    
    int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;
    int receive_finished = 0;

    if(type & MODE_WRITE)
    {
        s->mpu.status = value;
        
        if (value & 2)
        {
            if (s->mpu.receiving)
            {
                if (s->mpu.recv_index == s->mpu.recv_buffer[0])
                {
                    msg = "Receive finished";
                    s->mpu.receiving = 0;
                    receive_finished = 1;
                }
                else
                {
                    msg = "Unknown request while receiving";
                }
            }
            else if (s->mpu.sending)
            {
                msg = "Unknown request while sending";
            }
            else /* idle */
            {
                if (value == 0x46)
                {
                    msg = "init";
                }
            }
        }
        else
        {
            msg = "Receive request %s";
            msg_arg1 = (intptr_t) "";
            
            if (s->mpu.receiving)
            {
                msg_arg1 = (intptr_t) "(I'm busy receiving stuff!)";
            }
            else if (s->mpu.sending)
            {
                msg_arg1 = (intptr_t) "(I'm busy sending stuff, but I'll try!)";
                s->mpu.receiving = 1;
                s->mpu.recv_index = 0;
            }
            else
            {
                s->mpu.receiving = 1;
                s->mpu.recv_index = 0;
                eos_trigger_int(s, 0x50, 0);   /* MREQ */
                /* next steps in eos_handle_mreq -> mpu_handle_mreq_interrupt */
            }
        }
    }
    else
    {
        ret = (s->mpu.sending && !s->mpu.receiving) ? 0x3 :  /* I have data to send */
              (!s->mpu.sending && s->mpu.receiving) ? 0x0 :  /* I'm ready to receive data */
              (s->mpu.sending && s->mpu.receiving)  ? 0x1 :  /* I'm ready to send and receive data */
                                                      0x2 ;  /* I believe this is some error code */
        ret |= (s->mpu.status & 0xFFFFFFFC);                 /* The other bits are unknown;
                                                                they are set to 0x44 by writing to the register */

        msg = "status (sending=%d, receiving=%d)";
        msg_arg1 = s->mpu.sending;
        msg_arg2 = s->mpu.receiving;
    }

    io_log("MPU", s, address, type, value, ret, msg, msg_arg1, msg_arg2);

    if (receive_finished)
    {
        mpu_interpret_command(s);
    }
    
    return ret;
}

unsigned int eos_handle_sio3( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;

    switch(address & 0xFF)
    {
        case 0x04:
            /* C0820304 
             * 
             * write:
             *      - confirm data sent to MPU, by writing 1; used together with C0820318
             *      - when sending data from the MPU, each char pair is confirmed
             *        by sending a 0 back on C0820318, followed by a 1 here
             *        but the last char pair is not confirmed
             * 
             * read:
             *      - tst 0x1 in SIO3 when sending to MPU, stop if NE
             *      - guess: 0 if idle, 1 if busy
             */
            
            if(type & MODE_WRITE)
            {
                if (value == 1)
                {
                    msg = "To MPU <- ack data";
                }
                else
                {
                    msg = "To MPU <- ???";
                }
            }
            else
            {
                msg = "request to send?";
            }
            break;

        case 0x10:  /* C0820310: set to 0 at the beginning of SIO3_ISR, not used anywhere else */
            if(type & MODE_WRITE)
            {
                if (value == 0)
                {
                    msg = "ISR started";
                    mpu_handle_sio3_interrupt(s);
                }
            }
            break;

        case 0x18:  /* C0820318 - data sent to MPU */
            if(type & MODE_WRITE)
            {
                if (s->mpu.receiving)
                {
                    msg = "Data to MPU, at index %d %s";
                    msg_arg1 = s->mpu.recv_index;
                    if (s->mpu.recv_index + 2 < COUNT(s->mpu.recv_buffer))
                    {
                        s->mpu.recv_buffer[s->mpu.recv_index++] = (value >> 8) & 0xFF;
                        s->mpu.recv_buffer[s->mpu.recv_index++] = (value >> 0) & 0xFF;
                    }
                    else
                    {
                        msg_arg2 = (intptr_t) "(overflow!)";
                    }
                }
                else if (s->mpu.sending && value == 0)
                {
                    msg = "Dummy data to MPU";
                }
                else
                {
                    msg = "Data to MPU (wait a minute, I'm not listening!)";
                }
            }
            break;

        case 0x1C:  /* C082031C - data coming from MPU */
        
            if(type & MODE_WRITE)
            {
                msg = "Data from MPU (why writing?!)";
            }
            else
            {
                if (s->mpu.sending)
                {
                    if (s->mpu.spell_set < COUNT(mpu_init_spells) &&
                        s->mpu.out_spell >= 0 &&
                        s->mpu.out_char >= 0 && s->mpu.out_char < MPU_CURRENT_OUT_SPELL[0])
                    {
                        int hi = MPU_CURRENT_OUT_SPELL[s->mpu.out_char];
                        int lo = MPU_CURRENT_OUT_SPELL[s->mpu.out_char+1];
                        ret = (hi << 8) | lo;
                        msg = "Data from MPU";
                    }
                    else
                    {
                        msg = "From MPU -> out of range (cmd %d, char %d)";
                        msg_arg1 = s->mpu.out_spell;
                        msg_arg2 = s->mpu.out_char;
                        ret = 0;
                    }
                }
                else
                {
                    msg = "No data from MPU";
                    return 0;
                }
            }
            break;
    }

    io_log("SIO3", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

unsigned int eos_handle_mreq( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;
    
    if ((address & 0xFF) == 0x2C)
    {
        /* C020302C */
        /* set to 0x1C at the beginning of MREQ ISR, and to 0x0C at startup */
        if(type & MODE_WRITE)
        {
            msg = "CTL register %s";
            if (value == 0x0C)
            {
                msg_arg1 = (intptr_t) "init";
            }
            else if (value == 0x1C)
            {
                msg_arg1 = (intptr_t) "(ISR started)";
                mpu_handle_mreq_interrupt(s);
            }
        }
        else
        {
            msg = "CTL register -> idk, sending 0xC";
            ret = 0xC;
        }
    }
    
    io_log("MREQ", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

unsigned int eos_handle_gpio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 1;
    const char * msg = 0;
    static int unk = 0;

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
                ret = 0x40000 | 0x80000;
                msg = "VSW_STATUS";
            }
            break;

        case 0xF48C:
            /* 6D: return -1 here to launch "System & Display Check & Adjustment program" */
            msg = "70D/6D SD detect?";
            ret = 0x10C;
            break;
        
        case 0x019C:
            /* 5D3: return 1 to launch "System & Display Check & Adjustment program" */
            msg = "5D3 system check";
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

        case 0x0070:
            /* VIDEO on 600D */
            msg = "VIDEO CONNECT";
            ret = 1;
            break;
        
        case 0x0108:
            /* ERASE SW OFF on 600D */
            msg = "ERASE SW OFF";
            ret = 1;
            break;

        case 0x010C:
            msg = "something from hotplug task on 60D";
            ret = 1;
            break;

        case 0x00E8:
            /* MIC on 600D */
            msg = "MIC CONNECT";
            ret = 1;
            break;
        
        case 0x0034:
            /* USB on 600D */
            msg = "USB CONNECT";
            ret = 0;
            break;
        
        case 0x0138:
            /* HDMI on 600D */
            msg = "HDMI CONNECT";
            ret = 0;
            break;

        case 0x014:
            /* /VSW_ON on 600D */
            msg = "/VSW_ON";
            ret = 0;
            break;

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
            if(type & MODE_WRITE)
            {
                last_value = value;
                msg = (value & 0x02) ? "SRM_SetBusy" 
                                     : "SRM_ClearBusy" ;
            }
            else
            {
                ret = last_value;
            }
            break;
        }

        case 0x009C:
            return eos_handle_mpu(parm, s, address, type, value);

        case 0x00BC:
        {
            /* 5D2 CF LED */
            static int last_value = 0;
            if(type & MODE_WRITE)
            {
                last_value = value;
                msg = value == 0x46 ? "CF LED ON"  :
                      value == 0x44 ? "CF LED OFF" :
                                      "CF LED ???" ;
            }
            else
            {
                ret = last_value;
                return ret; /* quiet */
            }
            break;
        }

        case 0x301C:
            /* 40D CF Detect -> set low, so there is no CF */
            msg = "40D CF detect";
            ret = 0;
            break;
        
        case 0x3020:
            /* 5D3 CF Detect -> set low, so there is no CF */
            msg = "5D3 CF detect";
            ret = 0;
            break;
    }

    io_log("GPIO", s, address, type, value, ret, msg, 0, 0);
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

unsigned int eos_handle_dma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    unsigned int ret = 0;
    static unsigned int srcAddr = 0;
    static unsigned int dstAddr = 0;
    static unsigned int count = 0;
    unsigned int interruptId[] = { 0x00, 0x2f, 0x74, 0x75, 0x76 };

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

                    eos_trigger_int(s, interruptId[parm], 0);
                    
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
            if(type & MODE_WRITE)
            {
                srcAddr = value;
            }
            else
            {
                return srcAddr;
            }
            break;

        case 0x1C:
            msg = "dstAddr";
            if(type & MODE_WRITE)
            {
                dstAddr = value;
            }
            else
            {
                return dstAddr;
            }
            break;

        case 0x20:
            msg = "count";
            if(type & MODE_WRITE)
            {
                count = value;
            }
            else
            {
                return count;
            }
            break;
    }

    char dma_name[5];
    snprintf(dma_name, sizeof(dma_name), "DMA%i", parm);
    io_log(dma_name, s, address, type, value, ret, msg, 0, 0);

    return 0;
}


unsigned int eos_handle_tio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 1;
    const char * msg = 0;
//    int msg_arg1 = 0;

    switch(address & 0xFF)
    {
        case 0x00:
            if(type & MODE_WRITE)
            {
                if((value == 0x08 || value == 0x0A || value == 0x0D || (value >= 0x20 && value <= 0x7F)))
                {
                    printf("\x1B[31m%c\x1B[0m", value);
                    return 0;
                }
            }
            else
            {
                return 0;
            }
            break;

        case 0x04:
            msg = "Read byte: 0x%02X";
//            msg_arg1 = s->tio_rxbyte & 0xFF;
            ret = s->tio_rxbyte & 0xFF;
            break;
        
        case 0x08:
            /* quiet */
            return 0;

        case 0x14:
            if(type & MODE_WRITE)
            {
                if(value & 1)
                {
                    msg = "Reset RX indicator";
                    s->tio_rxbyte |= 0x100;
                }
                else
                {
                    /* quiet */
                    return 0;
                }
            }
            else
            {
                if((s->tio_rxbyte & 0x100) == 0)
                {
                    msg = "Signalling RX indicator";
                    ret = 3;
                }
                else
                {
                    /* quiet */
                    return 2;
                }
            }
            break;
    }

    io_log("TIO", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_sio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    static unsigned int last_sio_data = 0;
    static unsigned int last_sio_setup1 = 0;
    static unsigned int last_sio_setup2 = 0;
    static unsigned int last_sio_setup3 = 0;
    unsigned int pc = s->cpu->env.regs[15];

    switch(address & 0xFF)
    {
        case 0x04:
            if((type & MODE_WRITE) && (value & 1))
            {
                printf("[SIO%i] Transmit: 0x%08X, setup 0x%08X 0x%08X 0x%08X PC: 0x%08X\r\n", parm, last_sio_data, last_sio_setup1, last_sio_setup2, last_sio_setup3, pc );

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
                                printf("[SIO%i] RTC: Requested invalid transfer mode 0x%02X at PC: 0x%08X\r\n", parm, value, pc );
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
                return 0;
            }
            else
            {
                return 0;
            }
            break;

        case 0x0C:
            if(type & MODE_WRITE)
            {
                last_sio_setup1 = value;
                return 0;
            }
            else
            {
                return last_sio_setup1;
            }

        case 0x10:
            if(type & MODE_WRITE)
            {
                last_sio_setup3 = value;
                return 0;
            }
            else
            {
                return last_sio_setup3;
            }

        case 0x14:
            if(type & MODE_WRITE)
            {
                last_sio_setup1 = value;
                return 0;
            }
            else
            {
                return last_sio_setup1;
            }

        case 0x18:
            if(type & MODE_WRITE)
            {
                last_sio_data = value;

                printf("[SIO%i] Write to TX register PC: 0x%08X\r\n", parm, pc);
                return 0;
            }
            else
            {
                return last_sio_data;
            }

        case 0x1C:
            if(type & MODE_WRITE)
            {
                printf("[SIO%i] Write access to RX register\r\n", parm);
                return 0;
            }
            else
            {
                printf("[SIO%i] Read from RX register PC: 0x%08X  read: 0x%02X\r\n", parm, pc, last_sio_data);

                return last_sio_data;
            }
    }

    return 0;
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
#define DPRINTF(fmt, ...) do { printf("[SDIO] " fmt , ## __VA_ARGS__); } while (0)
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
    if (sd->cmd_flags != 0x11) {
#define RWORD(n) (((uint32_t)response[n + 5] << 24) | (response[n + 6] << 16) \
                  | (response[n + 7] << 8) | response[n])
        if (rlen == 0)
            goto error;
        if (rlen != 4 && rlen != 16)
            goto error;
        
        /* response bytes are shifted by one, and something has to fill the gap */
        /* guess: response length? (no idea, it appears unused) */
        response[0] = rlen;
        
        sd->response[0] = RWORD(0);
        sd->response[1] = RWORD(4);
        if (rlen == 4) {
            sd->response[2] = sd->response[3] = sd->response[4] = 0;
        } else {
            sd->response[1] = RWORD(4);
            sd->response[2] = RWORD(8);
            sd->response[3] = RWORD(12);
            sd->response[4] = RWORD(16);
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
    DPRINTF("Error\n");
    sd->status |= SDIO_STATUS_ERROR;
}

/* inspired from pl181_fifo_run from hw/sd/pl181.c */
/* only DMA transfers implemented */
static void sdio_read_data(SDIOState *sd)
{
    int i;

    if (sd->status & SDIO_STATUS_DATA_AVAILABLE)
    {
        DPRINTF("ERROR: read already done (%x)\n", sd->status);
        return;
    }
    
    if (!sd_data_ready(sd->card))
    {
        DPRINTF("ERROR: no data available\n");
        return;
    }

    if (!sd->dma_enabled)
    {
        DPRINTF("Reading %dx%d bytes without DMA (not implemented)\n", sd->transfer_count, sd->read_block_size);
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
}

static void sdio_write_data(SDIOState *sd)
{
    int i;

    if (sd->status & SDIO_STATUS_DATA_AVAILABLE)
    {
        DPRINTF("ERROR: write already done (%x)\n", sd->status);
        return;
    }

    if (!sd->dma_enabled)
    {
        printf("[SDIO] ERROR!!! Writing %dx%d bytes without DMA (not implemented)\n", sd->transfer_count, sd->read_block_size);
        printf("Cannot continue without risking corruption on the SD card image.\n");
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
}

static void sdio_trigger_interrupt(EOSState *s)
{
    /* after a successful operation, trigger int 0xB1 if requested */
    
    if ((s->sd.cmd_flags == 0x13 || s->sd.cmd_flags == 0x14)
        && !(s->sd.status & SDIO_STATUS_DATA_AVAILABLE))
    {
        /* if the current command does a data transfer, don't trigger until complete */
        DPRINTF("Data transfer not yet complete\n");
        return;
    }
    
    if ((s->sd.status & 3) == 1 && s->sd.irq_flags)
    {
        eos_trigger_int(s, 0xB1, 0);
    }
}

#undef DPRINTF

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
            if(type & MODE_WRITE)
            {
                s->sd.dma_enabled = value;
            }
            break;
        case 0x0C:
            msg = "Command flags?";
            s->sd.cmd_flags = value;
            if(type & MODE_WRITE)
            {
                /* reset status before doing any command */
                s->sd.status = 0;
                
                /* interpret this command */
                sdio_send_command(&s->sd);
                
                if (value == 0x14)
                {
                    sdio_read_data(&s->sd);
                }
                
                sdio_trigger_interrupt(s);
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
                /* not sure */
                s->sd.status = value;
            }
            else
            {
                ret = s->sd.status;
                ret = 0x200001;
            }
            break;
        case 0x14:
            msg = "irq enable?";
            s->sd.irq_flags = value;

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
            sdio_trigger_interrupt(s);
            break;
        case 0x18:
            msg = "init?";
            break;
        case 0x20:
            msg = "cmd_lo";
            s->sd.cmd_lo = value;
            break;
        case 0x24:
            msg = "cmd_hi";
            s->sd.cmd_hi = value;
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
        case 0x44:
            msg = "Response[4]";
            ret = s->sd.response[4];
            break;
        case 0x58:
            msg = "bus width";
            break;
        case 0x5c:
            msg = "write block size";
            s->sd.write_block_size = value;
            break;
        case 0x64:
            msg = "bus width";
            break;
        case 0x68:
            msg = "read block size";
            s->sd.read_block_size = value;
            break;
        case 0x6C:
            msg = "FIFO data?";
            break;
        case 0x70:
            msg = "transfer status?";
            break;
        case 0x7c:
            msg = "transfer block count";
            s->sd.transfer_count = value;
            break;
        case 0x80:
            msg = "transferred blocks";
            /* Goro is very strong. Goro never fails. */
            ret = s->sd.dma_count / s->sd.read_block_size;
            break;
        case 0x84:
            msg = "SDREP: Status register/error codes";
            break;
        case 0x88:
            msg = "SDBUFCTR: Set to 0x03 before reading";
            break;
    }

    io_log("SDIO", s, address, type, value, ret, msg, msg_arg1, msg_arg2);
    return ret;
}

unsigned int eos_handle_sddma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;

    switch(address & 0xFFF)
    {
        case 0x14:
            msg = "70D ???";
            ret = 1;
            break;
        case 0x60:
        case 0x20:
            msg = "Transfer memory address";
            s->sd.dma_addr = value;
            break;
        case 0x64:
        case 0x24:
            msg = "Transfer byte count";
            s->sd.dma_count = value;
            break;
        case 0x70:
        case 0x30:
            msg = "Flags/Status";
            break;
        case 0x78:
        case 0x38:
            msg = "Transfer start?";

            /* DMA transfer? */
            if (s->sd.cmd_flags == 0x13)
            {
                sdio_write_data(&s->sd);
                sdio_trigger_interrupt(s);
            }

            break;
    }

    io_log("SDDMA", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

unsigned int eos_handle_cfdma ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    unsigned int ret = 0;
    const char * msg = 0;

    switch(address & 0xFFFF)
    {
        case 0x8104:
            msg = "5D3 unknown (trying random)";
            ret = rand();
            break;

        case 0x21F0:
            msg = "ATA data port";
            break;

        case 0x21F1:
            if(type & MODE_WRITE)
            {
                msg = "ATA features";
            }
            else
            {
                msg = "ATA error";
            }
            break;

        case 0x21F2:
            msg = "ATA sector count";
            break;

        case 0x21F3:
            msg = "ATA LBAlo";
            break;

        case 0x21F4:
            msg = "ATA LBAmid";
            break;

        case 0x21F5:
            msg = "ATA LBAhi";
            break;

        case 0x21F6:
            msg = "ATA drive/head port";
            break;

        case 0x21F7:
            if(type & MODE_WRITE)
            {
                msg = "ATA command";
            }
            else
            {
                msg = "ATA status (returning 'drive fault')";
                ret = (1<<5) | (1<<0);
            }
            break;
        
        case 0x23F6:
            msg = "ATA drive address";
            break;
    }
    io_log("CFDMA", s, address, type, value, ret, msg, 0, 0);
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
    
    switch(address & 0xFFF)
    {
        case 0x008: /* CLOCK_ENABLE */
            if(type & MODE_WRITE)
            {
                s->clock_enable = value;
                msg = format_clock_enable(value);
            }
            else
            {
                ret = s->clock_enable;
                msg = format_clock_enable(ret);
            }
            break;
        
        case 0xA4:
            if(type & MODE_WRITE)
            {
            }
            else
            {
                ret = 1;
            }
            break;
            
        case 0x244:
            if(type & MODE_WRITE)
            {
            }
            else
            {
                /* idk, expected to be so in 5D3 123 */
                ret = 1;
            }
            break;
        case 0x204:
            if(type & MODE_WRITE)
            {
            }
            else
            {
                /* idk, expected to be so in 5D3 bootloader */
                ret = 2;
            }
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
            if(type & MODE_WRITE)
            {
                msg = "BMP VRAM";
                s->disp.bmp_vram = value;
            }
            break;

        case 0x0E0:
            if(type & MODE_WRITE)
            {
                msg = "YUV VRAM";
                s->disp.img_vram = value;
            }
            break;
        
        case 0x80 ... 0xBC:
            msg = "4-bit palette";
            if(type & MODE_WRITE)
            {
                int entry = ((address & 0xFFF) - 0x80) / 4;
                process_palette_entry(value, &s->disp.palette_4bit[entry], entry, &msg);
                s->disp.is_4bit = 1;
            }
            break;

        case 0x800 ... 0xBFC:
            msg = "8-bit palette";
            if(type & MODE_WRITE)
            {
                int entry = ((address & 0xFFF) - 0x800) / 4;
                process_palette_entry(value, &s->disp.palette_8bit[entry], entry, &msg);
                s->disp.is_4bit = 0;
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
    unsigned int pc = s->cpu->env.regs[15];
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

unsigned int eos_handle_digic6 ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    const char * msg = 0;
    unsigned int ret = 0;

    switch (address)
    {
        case 0xD203046C:
        case 0xD203086C:
            ret = 1;
            msg = "7D2 init";
            break;
    }
    
    io_log("DIGIC6", s, address, type, value, ret, msg, 0, 0);
    return ret;
}

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
