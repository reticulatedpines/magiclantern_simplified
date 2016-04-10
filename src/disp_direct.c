/**
 * Direct display access from bootloader context
 * Portable (same binary for all cameras)
 * Simplified version, BMP only
 */

#include <string.h>
#include <stdint.h>

#include "font_direct.h"
#include "disp_direct.h"
#include "compiler.h"
#include "consts.h"

#define MEM(x) (*(volatile uint32_t *)(x))

static uint8_t *disp_framebuf = (uint8_t *)0x44000000;

static int disp_yres = 480;
static int disp_xres = 720;

static void disp_set_palette()
{
    // transparent
    // 1 - red
    // 2 - green
    // 3 - blue
    // 4 - cyan
    // 5 - magenta
    // 6 - yellow
    // 7 - orange
    // 8 - transparent black
    // 9 - black
    // A - gray 1
    // B - gray 2
    // C - gray 3
    // D - gray 4
    // E - gray 5
    // F - white

    uint32_t palette_pb[16] = {0x00fc0000, 0x0346de7f, 0x036dcba1, 0x031a66ea, 0x03a42280, 0x03604377, 0x03cf9a16, 0x0393b94b, 0x00000000, 0x03000000, 0x03190000, 0x033a0000, 0x03750000, 0x039c0000, 0x03c30000, 0x03eb0000};

    for(uint32_t i = 0; i < 16; i++)
    {
        MEM(0xC0F14080 + i*4) = palette_pb[i];
    }
    MEM(0xC0F14078) = 1;
}

void disp_set_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    uint32_t pixnum = ((y * disp_xres) + x) / 2;
    
    if(x & 1)
    {
        disp_framebuf[pixnum] = (disp_framebuf[pixnum] & 0x0F) | ((color & 0x0F)<<4);
    }
    else
    {
        disp_framebuf[pixnum] = (disp_framebuf[pixnum] & 0xF0) | (color & 0x0F);
    }
}

static void disp_fill(uint32_t color)
{
    /* build a 32 bit word */
    uint32_t val = color;
    val |= val << 4;
    val |= val << 8;
    val |= val << 16;
    
    for(int ypos = 0; ypos < disp_yres; ypos++)
    {
        /* we are writing 8 pixels at once with a 32 bit word */
        for(int xpos = 0; xpos < disp_xres; xpos += 8)
        {
            /* get linear pixel number */
            uint32_t pixnum = ((ypos * disp_xres) + xpos);
            /* two pixels per byte */
            uint32_t *ptr = (uint32_t *)&disp_framebuf[pixnum / 2];
            
            *ptr = val;
        }
    }
}

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

static uint32_t find_func_called_before_string_ref(char* ref_string)
{
    /* look for this pattern:
     * fffe1824:    eb0019a5    bl  @fromutil_disp_init
     * fffe1828:    e28f0e2e    add r0, pc, #736    ; *'Other models\n'
     */
    
    int found = 0;
    uint32_t answer = 0;
    
    /* only scan the bootloader area */
    for (uint32_t i = 0xFFFE0000; i < 0xFFFFFFF0; i += 4 )
    {
        uint32_t this = MEM(i);
        uint32_t next = MEM(i+4);
        int is_bl         = ((this & 0xFF000000) == 0xEB000000);
        int is_string_ref = ((next & 0xFFFFF000) == 0xe28f0000); /* add R0, pc, #offset */
        if (is_bl && is_string_ref)
        {
            uint32_t string_offset = decode_immediate_shifter_operand(next);
            uint32_t pc = i + 4;
            char* string_addr = (char*)(pc + string_offset + 8);
            if (strcmp(string_addr, ref_string) == 0)
            {
                /* bingo? */
                found++;
                uint32_t func_offset = (this & 0x00FFFFFF) << 2;
                uint32_t pc = i;
                uint32_t func_addr = pc + func_offset + 8;
                if (func_addr > 0xFFFE0000)
                {
                    /* looks ok? */
                    answer = func_addr;
                }
            }
        }
    }
    
    if (found == 1)
    {
        /* only return success if there's a single match (no ambiguity) */
        return answer;
    }
    
    return 0;
}

static void* disp_init_autodetect()
{
    /* Called right before printing the following strings:
     * "Other models\n"                     (5D2, 5D3, 60D, 500D, 70D, 7D)
     * "File(*.fir) not found\n"            (5D2, 5D3, 60D, 500D, 70D)
     * "sum check error or code modify\n"   (5D2, 60D, 500D, 7D)
     * "sum check error\n"                  (5D3, 70D)
     * "CF Read error\n"                    (5D2, 60D, 500D, 7D)
     * ...
     */
    
    uint32_t a = find_func_called_before_string_ref("Other models\n");
    uint32_t b = find_func_called_before_string_ref("File(*.fir) not found\n");
    uint32_t c = find_func_called_before_string_ref("sum check error or code modify\n");
    
    if (a == b)
    {
        /* I think this is what we are looking for :) */
        return (void*) a;
    }

    if (a == c)
    {
        /* I think this is what we are looking for :) */
        return (void*) a;
    }

    return 0;
}

void disp_init()
{
    /* is this address valid for all cameras? */
    disp_framebuf = (uint8_t *)0x44000000;
    
    /* this should cover most (if not all) ML-supported cameras */
    /* and maybe most unsupported cameras as well :) */
    void (*fromutil_disp_init)(uint32_t) = disp_init_autodetect();
    
    /* do not continue if fromutil_disp_init could not be found */
    if (!fromutil_disp_init)
    {
        while(1);
    }
    
    /* first clear, then init */
    disp_fill(COLOR_BLACK);
    
    /* this one initializes everyhting that is needed for display usage. PWM, PWR, GPIO, SIO and DISPLAY */
    fromutil_disp_init(0);

    /* we want our own palette */
    disp_set_palette();
    
    /* BMP foreground is transparent */
    disp_fill(COLOR_BLACK);
    
    /* set frame buffer memory area */
    MEM(0xC0F140D0) = (uint32_t)disp_framebuf & ~0x40000000;
    MEM(0xC0F140D4) = (uint32_t)disp_framebuf & ~0x40000000;
    
    /* we don't use YUV */
    
    /* trigger a display update */
    MEM(0xC0F14000) = 1;
    
    /* from now on, everything you write on the display buffers
     * will appear on the screen without doing anything special */
}

static uint32_t print_line_pos = 2;

void print_line(uint32_t color, uint32_t scale, char *txt)
{
    font_draw(20, print_line_pos * 10, color, scale, txt);
    
    print_line_pos += scale;
}
