
#include "dryos.h"
#include "bmp.h"
#include "rbf_font.h"

/* here some macros to wrap the CHDK functions for ML code */
#define FG_COLOR(f) FONT_FG(f)
#define BG_COLOR(f) FONT_BG(f)
#define MAKE_COLOR(fg,bg) FONT(0,fg,bg)

#define draw_char(x,y,c,h) do{}while(0)



//-------------------------------------------------------------------
static unsigned int RBF_HDR_MAGIC1 = 0x0DF00EE0;
static unsigned int RBF_HDR_MAGIC2 = 0x00000003;


static unsigned char *ubuffer = 0;                  // uncached memory buffer for reading font data from SD card
static int rbf_codepage = FONT_CP_WIN; 

struct font font_dynamic[MAX_DYN_FONTS];
static char *dyn_font_name[MAX_DYN_FONTS];
uint32_t dyn_fonts = 0;

//-------------------------------------------------------------------

font *new_font() {
    // allocate font from cached memory
    font *f = AllocateMemory(sizeof(font));
    if (f) {
        memset(f,0,sizeof(font));      // wipe memory
        // return address in cached memory
        return f;
    }

    // memory not allocated ! should probably do something else in this case ?
    return 0;
}

uint32_t font_by_name(char *file, uint32_t fg_color, uint32_t bg_color)
{
    /* check if this font was already loaded */
    for(int pos = 0; pos < dyn_fonts; pos++)
    {
        if(!strcmp(file, dyn_font_name[pos]))
        {   
            /* yeah, so return a new specifier for this id */
            return FONT_DYN(pos, fg_color, bg_color);
        }
    }
    
    /* too many fonts loaded, return default font */
    if(dyn_fonts >= MAX_DYN_FONTS)
    {
        bmp_printf(FONT_MED, 10, 30, "too many fonts");
        return FONT_SMALL;
    }
    
    /* was not loaded, try to load */
    char filename[128];
    snprintf(filename, sizeof(filename), CARD_DRIVE "ML/FONTS/%s.RBF", file);
    
    uint32_t size;
    if((FIO_GetFileSize( filename, &size ) != 0) || (size == 0))
    {
        /* failed to load, return default font */
        bmp_printf(FONT_MED, 10, 30, "File '%s' not found", filename);
        return FONT_MED;
    }
    
    void *font = new_font();
    
    /* load the font here */
    if(!rbf_font_load(filename, font, 0))
    {
        bmp_printf(FONT_MED, 10, 30, "File '%s' failed to load", filename);
        FreeMemory(font);
        return FONT_LARGE;
    }

    /* now updated cached font name (not filename) */
    dyn_font_name[dyn_fonts] = AllocateMemory(strlen(filename) + 1);
    strcpy(dyn_font_name[dyn_fonts], file);
    
    /* and measure font sizes */
    font_dynamic[dyn_fonts].bitmap = font;
    font_dynamic[dyn_fonts].height = rbf_font_height(font_dynamic[dyn_fonts].bitmap);
    font_dynamic[dyn_fonts].width = rbf_char_width(font_dynamic[dyn_fonts].bitmap, 'X');
    dyn_fonts++;

    return FONT_DYN(dyn_fonts - 1, fg_color, bg_color);
}

void alloc_cTable(font *f) {

    // Calculate additional values for font
    f->width = 8 * f->hdr.charSize / f->hdr.height;
    f->charCount = f->hdr.charLast - f->hdr.charFirst + 1;

    // set width table to default value
    memset(f->wTable, f->width, 256);
    
    // allocate cTable memory

    // If existing data has been allocated then we are re-using the font data
    // See if it the existing cTable data is large enough to hold the new font data
    // If not FreeMemory it so new memory will be allocated
    if ((f->cTable != 0) && (f->cTableSizeMax < (f->charCount*f->hdr.charSize))) {
        FreeMemory(f->cTable);              // free the memory
        f->cTable = 0;                // clear pointer so new memory is allocated
        f->cTableSizeMax = 0;
    }

    // Allocated memory if needed
    if (f->cTable == 0) {
        // Allocate memory from cached pool
        int size = f->charCount*f->hdr.charSize;
        NotifyBox(1000, "%d %d %d ", size, f->charCount, f->hdr.charSize);
        f->cTable = AllocateMemory(size);

        // save size
        f->cTableSize = f->charCount*f->hdr.charSize;
        if (f->cTableSizeMax == 0) f->cTableSizeMax = f->cTableSize;    // Save actual size allocated
    }
}

//-------------------------------------------------------------------
static const char tbl_dos2win[] = {
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0x2D, 0x2D, 0x2D, 0xA6, 0x2B, 0xA6, 0xA6, 0xAC, 0xAC, 0xA6, 0xA6, 0xAC, 0x2D, 0x2D, 0x2D, 0xAC,
    0x4C, 0x2B, 0x54, 0x2B, 0x2D, 0x2B, 0xA6, 0xA6, 0x4C, 0xE3, 0xA6, 0x54, 0xA6, 0x3D, 0x2B, 0xA6,
    0xA6, 0x54, 0x54, 0x4C, 0x4C, 0x2D, 0xE3, 0x2B, 0x2B, 0x2D, 0x2D, 0x2D, 0x2D, 0xA6, 0xA6, 0x2D,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    0xA8, 0xB8, 0xAA, 0xBA, 0xAF, 0xBF, 0xA1, 0xA2, 0xB0, 0x95, 0xB7, 0x76, 0xB9, 0xA4, 0xA6, 0xA0
};

int code_page_char(int ch)
{
    // convert character value based on selected code page
    if ((rbf_codepage == FONT_CP_DOS) && (ch >= 128) && (ch < 256)) {
        // Convert DOS to WIN char
        ch = tbl_dos2win[ch-128];
    }
    return ch;
}

//-------------------------------------------------------------------
// Return address of 'character' data for specified font & char
char* rbf_font_char(font* f, int ch)
{
    if (f && (ch >= f->hdr.charFirst) && (ch <= f->hdr.charLast))
    {
        return &f->cTable[(ch-f->hdr.charFirst)*f->hdr.charSize];
    }

    return 0;
}
//-------------------------------------------------------------------
// Read data from SD file using uncached buffer and copy to cached
// font memory
int font_read(int fd, unsigned char *dest, int len)
{
    // Return actual bytes read
    int bytes_read = 0;

    if(!ubuffer)
    {
        ubuffer = alloc_dma_memory(UBUFFER_SIZE);
    }
    
    if (ubuffer)
    {
        // Read file in UBUFFER_SIZE blocks
        while (len)
        {
            // Calc size of next block to read = min(UBUFFER_SIZE, len)
            int to_read = UBUFFER_SIZE;
            if (to_read > len) to_read = len;

            // Read block and copy to dest
            bytes_read += FIO_ReadFile(fd, ubuffer, to_read);
            memcpy(dest, ubuffer, to_read);

            // Increment dest pointer, decrement len left to read
            dest += to_read;
            len -= to_read;
        }
    }

    return bytes_read;
}
//-------------------------------------------------------------------
// Load from from file. If maxchar != 0 limit charLast (for symbols)
int rbf_font_load(char *file, font* f, int maxchar)
{
    int i;

    // make sure the font has been allocated
    if (f == 0)
    {
        return 0;
    }

    // open file (can't use fopen here due to potential conflict FsIoNotify crash)
    FILE *fd = FIO_Open(file, O_RDONLY | O_SYNC);
    if( fd == INVALID_PTR )
    {
        return;
    }
    
    // read header
    i = font_read(fd, (unsigned char*)&(f->hdr), sizeof(font_hdr));

    // check size read is correct and magic numbers are valid
    if ((i != sizeof(font_hdr)) || (f->hdr.magic1 != RBF_HDR_MAGIC1) || (f->hdr.magic2 != RBF_HDR_MAGIC2))
    {
        return 0;
    }
    
    if (maxchar != 0)
    {
        f->hdr.charLast = maxchar;
    }

    alloc_cTable(f);

    // read width table (using uncached buffer)
    FIO_SeekFile(fd, f->hdr._wmapAddr, SEEK_SET);
    font_read(fd, (unsigned char*)&f->wTable[f->hdr.charFirst], f->charCount);

    // read cTable data (using uncached buffer)
    FIO_SeekFile(fd, f->hdr._cmapAddr, SEEK_SET);
    font_read(fd, (unsigned char*)f->cTable, f->charCount*f->hdr.charSize);

    FIO_CloseFile(fd);

    return 1;
}


//-------------------------------------------------------------------
void rbf_set_codepage(int codepage) {
    rbf_codepage = codepage;
}

//-------------------------------------------------------------------
int rbf_font_height(font *rbf_font) {
    return rbf_font->hdr.height;
}
//-------------------------------------------------------------------
int rbf_char_width(font *rbf_font, int ch) {
    return rbf_font->wTable[code_page_char(ch)];
}

//-------------------------------------------------------------------
int rbf_str_width(font *rbf_font, const char *str) {
    int l=0;

    // Calculate how long the string is in pixels
    while (*str)
        l+=rbf_char_width(rbf_font, *str++);

    return l;
}

int rbf_str_clipped_width(font *rbf_font, const char *str, int l, int maxlen) {
    // Calculate how long the string is in pixels (possibly clipped to 'maxlen')
    while (*str && l+rbf_char_width(rbf_font, *str)<=maxlen)
        l+=rbf_char_width(rbf_font, *str++);

    return l;
}

//-------------------------------------------------------------------
void font_draw_char(font *rbf_font, int x, int y, char *cdata, int width, int height, int pixel_width, color cl) {
    int xx, yy;
    void *vram = bmp_vram();
    
    // draw pixels for font character
    if (cdata)
        for (yy=0; yy<height; ++yy)
            for (xx=0; xx<pixel_width; ++xx)
                bmp_putpixel_fast(vram,x+xx ,y+yy, (cdata[yy*width/8+xx/8] & (1<<(xx%8))) ? FG_COLOR(cl) : BG_COLOR(cl));
}

//-------------------------------------------------------------------
int rbf_draw_char(font *rbf_font, int x, int y, int ch, color cl) {
    // Convert char for code page
    ch = code_page_char(ch);

    // Get char data pointer
    char* cdata = rbf_font_char(rbf_font, ch);

    font_draw_char(rbf_font, x, y, cdata, rbf_font->width, rbf_font->hdr.height, rbf_font->wTable[ch], cl);

    return rbf_font->wTable[ch];
}


//-------------------------------------------------------------------
// Draw a string colored 'c1' with the character at string-position 'c' colored 'c2'.
int rbf_draw_string_c(font *rbf_font, int x, int y, const char *str, color c1, int c, color c2) {
     int l=0, i=0;

     while (*str) {
          if (*str == '\n')
          {
              l = 0;
              y += rbf_font->hdr.height;
              str++;
              i++;
              continue;
          }
          l+=rbf_draw_char(rbf_font, x+l, y, *str++, (i==c)?c2:c1);
          ++i;
     }
     return l;
}

//-------------------------------------------------------------------
int rbf_draw_string(font *rbf_font, int x, int y, const char *str, color cl) {
    return rbf_draw_string_c(rbf_font, x, y, str, cl, -1, 0);
}

//-------------------------------------------------------------------
static int cursor_on = 0;
static int cursor_start = 0;
static int cursor_end = 0;

void rbf_enable_cursor(int s, int e)
{
    cursor_on = 1;
    cursor_start = s;
    cursor_end = e;
}

void rbf_disable_cursor()
{
    cursor_on = 0;
}

int rbf_draw_clipped_string(font *rbf_font, int x, int y, const char *str, color cl, int l, int maxlen)
{
    int i = 0;
    color inv_cl = ((cl & 0xFF00) >> 8) | ((cl & 0xFF) << 8);

    // Draw chars from string up to max pixel length
    while (*str && l+rbf_char_width(rbf_font, *str)<=maxlen)
    {
        if (cursor_on && (cursor_start <= i) && (i <= cursor_end))
            l+=rbf_draw_char(rbf_font, x+l, y, *str++, inv_cl);
        else
            l+=rbf_draw_char(rbf_font, x+l, y, *str++, cl);
        i++;
    }

    return l;
}

//-------------------------------------------------------------------
int rbf_draw_string_len(font *rbf_font, int x, int y, int len, const char *str, color cl) {
    // Draw string characters
    int l = rbf_draw_clipped_string(rbf_font, x, y, str, cl, 0, len);

    // Fill any remaining space on right with background color
    if (l < len)
        bmp_fill(x+l, y, len-2, rbf_font->hdr.height-1, cl);

    return len;
}

//-------------------------------------------------------------------
int rbf_draw_string_right_len(font *rbf_font, int x, int y, int len, const char *str, color cl) {
    // Calulate amount of padding needed on the left
    int l = len - rbf_str_clipped_width(rbf_font, str, 0, len);

    // Fill padding with background color
    if (l > 0)
        bmp_fill(x, y, l-1, rbf_font->hdr.height-1, cl);

    // Draw chars
    l = rbf_draw_clipped_string(rbf_font, x, y, str, cl, l, len);

    return l;
}

static void rbf_init()
{
    /* load some fonts */
    font_by_name("sans20", COLOR_BLACK, COLOR_WHITE);
    font_by_name("sans32", COLOR_BLACK, COLOR_WHITE);
    font_by_name("term12", COLOR_BLACK, COLOR_WHITE);
    font_by_name("term20", COLOR_BLACK, COLOR_WHITE);
    font_by_name("term32", COLOR_BLACK, COLOR_WHITE);
}

INIT_FUNC("rbf", rbf_init);

