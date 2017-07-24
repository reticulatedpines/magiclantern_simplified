#ifndef _qemu_util_h
#define _qemu_util_h

#ifdef CONFIG_QEMU

/* print messages to the QEMU console */
int qprintf(const char * fmt, ...);

/* print non-formatted messages */
/* (useful for large strings without risking stack overflow) */
int qprint(const char * msg);

void qemu_cam_init();
void qemu_hello();
void qemu_menu_screenshots();
void qemu_hptimer_test();

/** Some small engio API **/
#define REG_PRINT_CHAR 0xCF123000
#define REG_SHUTDOWN   0xCF123004
#define REG_DUMP_VRAM  0xCF123008
#define REG_PRINT_NUM  0xCF12300C
#define REG_DISAS_32   0xCF123010
#define REG_BMP_VRAM   0xCF123014
#define REG_IMG_VRAM   0xCF123018
#define REG_RAW_BUFF   0xCF12301C
#define REG_DISP_TYPE  0xCF123020
#define REG_CALLSTACK  0xCF123030

/* print a 32-bit number (also works very early in the boot process) */
/* number formatting done in QEMU */
static inline void qprintn(int32_t num)
{
    *(volatile uint32_t *)(REG_PRINT_NUM) = num;
}

/* disassemble a 32-bit address */
static inline void qdisas(uint32_t addr)
{
    *(volatile uint32_t *)(REG_DISAS_32) = addr;
}

#else /* without CONFIG_QEMU */

/* these don't execute anything on regular builds (without CONFIG_QEMU) */
/* also, the strings used as arguments won't be included in the compiled binary */
/* but the calls will still be type-checked, unlike ifdef */
static inline int qprintf(const char * fmt, ...) { return 0; }
static inline int qprint(const char * msg) { return 0; }
static inline void qprintn(int num) { }
static inline void qdisas(uint32_t addr) { }

#endif
#endif
