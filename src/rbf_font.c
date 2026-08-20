
#include "dryos.h"
#include "bmp.h"
#include "beep.h"
#include "rbf_font.h"

extern uint32_t ml_refresh_display_needed;

/* here some macros to wrap the CHDK functions for ML code */
#define FG_COLOR(f) FONT_FG(f)
#define BG_COLOR(f) FONT_BG(f)
#define MAKE_COLOR(fg,bg) FONT(0,fg,bg)

#define FONT_CONDENSED 0x00100000 /* this bit is not (yet) used, so we use it as internal flag */

#define draw_char(x,y,c,h) do{}while(0)

static int rbf_font_load(char *file, font* f, int maxchar);
static inline int rbf_font_height(font *rbf_font);
static inline int rbf_char_width(font *rbf_font, int ch);

#define ZH_PACK_MAGIC 0x31485A4D

struct zh_pack_header
{
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t translation_count;
    uint32_t glyph_count;
    uint32_t translation_offset;
    uint32_t glyph_offset;
};

struct zh_translation
{
    uint32_t english_offset;
    uint32_t chinese_offset;
};

struct zh_glyph
{
    uint16_t codepoint;
    uint16_t advance;
    uint16_t rows[16];
};

static const struct zh_glyph *zh_find_glyph(uint32_t codepoint);
static int zh_glyph_advance(font *rbf_font, const struct zh_glyph *glyph);
static int draw_zh_glyph(font *rbf_font, int x, int y, const struct zh_glyph *glyph, int fontspec);

SIZE_CHECK_STRUCT(zh_pack_header, 28);
SIZE_CHECK_STRUCT(zh_translation, 8);
SIZE_CHECK_STRUCT(zh_glyph, 36);

static uint8_t *zh_pack;
static const struct zh_pack_header *zh_header;
static const struct zh_translation *zh_translations;
static const struct zh_glyph *zh_glyphs;

static int zh_range_valid(uint32_t offset, uint32_t count, uint32_t item_size, uint32_t size)
{
    return offset <= size && count <= (size - offset) / item_size;
}

static int zh_has_nul(const uint8_t *text, uint32_t size)
{
    while (size--)
        if (!*text++) return 1;
    return 0;
}

static void zh_load_from_card(void)
{
    int file_size = 0;
    uint8_t *file_data = read_entire_file("ML/data/zh_cn.bin", &file_size);
    if (!file_data || file_size < (int) sizeof(struct zh_pack_header)) return;

    uint8_t *data = malloc(file_size);
    if (!data)
    {
        fio_free(file_data);
        return;
    }
    memcpy(data, file_data, file_size);
    fio_free(file_data);

    const struct zh_pack_header *header = (const void *) data;
    if (header->magic != ZH_PACK_MAGIC || header->version != 2 ||
        header->size != (uint32_t) file_size ||
        header->translation_offset < sizeof(struct zh_pack_header) ||
        !zh_range_valid(header->translation_offset, header->translation_count,
                        sizeof(struct zh_translation), header->size) ||
        !zh_range_valid(header->glyph_offset, header->glyph_count,
                        sizeof(struct zh_glyph), header->size))
    {
        free(data);
        return;
    }

    uint32_t translation_end = header->translation_offset +
        header->translation_count * sizeof(struct zh_translation);
    uint32_t glyph_end = header->glyph_offset + header->glyph_count * sizeof(struct zh_glyph);
    if (header->glyph_offset < translation_end)
    {
        free(data);
        return;
    }

    const struct zh_translation *translations = (const void *) (data + header->translation_offset);
    for (uint32_t i = 0; i < header->translation_count; i++)
    {
        uint32_t english = translations[i].english_offset;
        uint32_t chinese = translations[i].chinese_offset;
        if (english < glyph_end || chinese < glyph_end ||
            english >= header->size || chinese >= header->size ||
            !zh_has_nul(data + english, header->size - english) ||
            !zh_has_nul(data + chinese, header->size - chinese) ||
            (i && strcmp((const char *) data + translations[i-1].english_offset,
                         (const char *) data + english) >= 0))
        {
            free(data);
            return;
        }
    }

    const struct zh_glyph *glyphs = (const void *) (data + header->glyph_offset);
    for (uint32_t i = 1; i < header->glyph_count; i++)
    {
        if (glyphs[i-1].codepoint >= glyphs[i].codepoint)
        {
            free(data);
            return;
        }
    }

    zh_pack = data;
    zh_header = (const void *) data;
    zh_translations = (const void *) (data + header->translation_offset);
    zh_glyphs = (const void *) (data + header->glyph_offset);
}

const char *rbf_translate(const char *str)
{
    if (!zh_pack) return str;
    int lo = 0;
    int hi = zh_header->translation_count - 1;

    while (lo <= hi)
    {
        int mid = lo + (hi - lo) / 2;
        const char *english = (const char *) zh_pack + zh_translations[mid].english_offset;
        int cmp = strcmp(str, english);
        if (cmp == 0) return (const char *) zh_pack + zh_translations[mid].chinese_offset;
        if (cmp < 0) hi = mid - 1;
        else lo = mid + 1;
    }
    return str;
}

int rbf_use_utf8_renderer(const char *str)
{
    if (zh_pack) return 1;
    while (*str)
        if ((unsigned char) *str++ >= 0x80) return 1;
    return 0;
}

//-------------------------------------------------------------------
static int RBF_HDR_MAGIC1 = 0x0DF00EE0;
static int RBF_HDR_MAGIC2 = 0x00000003;

struct font font_dynamic[MAX_DYN_FONTS+1];
static char *dyn_font_name[MAX_DYN_FONTS+1];
uint32_t dyn_fonts = 0;

#ifdef CONFIG_NO_BFNT
/* kitor: Those replace entries in platform consts.h for DIGIC6+
 *        that does not use BMP fonts anymore.  */
uint8_t *BFNT_CHAR_CODES;
uint8_t *BFNT_BITMAP_OFFSET;
uint8_t *BFNT_BITMAP_DATA;

bfnt_font* BFNT_FONT;
#endif

//-------------------------------------------------------------------

static font *new_font() {
    // allocate font from cached memory
    font *f = malloc(sizeof(font));
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
    for(int pos = 0; pos < (int)dyn_fonts; pos++)
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
        beep();
        bmp_printf(FONT_CANON, 0, 0, "Too many fonts");
        return FONT_CANON;
    }
    
    /* was not loaded, try to load */
    char filename[128];
    snprintf(filename, sizeof(filename), "ML/FONTS/%s.RBF", file);
    
    uint32_t size;
    if((FIO_GetFileSize( filename, &size ) != 0) || (size == 0))
    {
        /* failed to load, return default font */
        beep();
        bmp_printf(FONT_CANON, 0, 0, "%s not found", file);
        return FONT_CANON;
    }
    
    void *font = new_font();
    
    /* load the font here */
    if(!rbf_font_load(filename, font, 0))
    {
        bmp_printf(FONT_CANON, 0, 0, "%s not loaded", file);
        free(font);
        return FONT_CANON;
    }

    /* now updated cached font name (not filename) */
    dyn_font_name[dyn_fonts] = malloc(strlen(filename) + 1);
    strcpy(dyn_font_name[dyn_fonts], file);
    
    /* and measure font sizes */
    font_dynamic[dyn_fonts].bitmap = font;
    font_dynamic[dyn_fonts].height = rbf_font_height((void*)font_dynamic[dyn_fonts].bitmap);
    font_dynamic[dyn_fonts].width = rbf_char_width((void*)font_dynamic[dyn_fonts].bitmap, '0');
    dyn_fonts++;

    return FONT_DYN(dyn_fonts - 1, fg_color, bg_color);
}

static void alloc_cTable(font *f) {

    // Calculate additional values for font
    f->width = 8 * f->hdr.charSize / f->hdr.height;
    f->charCount = f->hdr.charLast - f->hdr.charFirst + 1;

    // set width table to default value
    memset(f->wTable, f->width, 256);
    
    // allocate cTable memory

    // If existing data has been allocated then we are re-using the font data
    // See if it the existing cTable data is large enough to hold the new font data
    // If not free it so new memory will be allocated
    if ((f->cTable != NULL) && (f->cTableSizeMax < (f->charCount*f->hdr.charSize))) {
        free(f->cTable);  // free the memory
        f->cTable = NULL; // clear pointer so new memory is allocated
        f->cTableSizeMax = 0;
    }

    // Allocated memory if needed
    if (f->cTable == NULL) {
        // Allocate memory from cached pool
        int size = f->charCount*f->hdr.charSize;
        f->cTable = malloc(size);

        // save size
        f->cTableSize = f->charCount*f->hdr.charSize;
        if (f->cTableSizeMax == 0) f->cTableSizeMax = f->cTableSize;    // Save actual size allocated
    }
}

//-------------------------------------------------------------------
// Return address of 'character' data for specified font & char
static inline char* FAST rbf_font_char(font* f, int ch)
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
static int font_read(FILE* fd, unsigned char *dest, int len)
{
    // Return actual bytes read
    int bytes_read = 0;

    unsigned char *ubuffer = fio_malloc(len);
    
    if (ubuffer)
    {
        // Read block and copy to dest
        bytes_read += FIO_ReadFile(fd, ubuffer, len);
        memcpy(dest, ubuffer, len);
        fio_free(ubuffer);
    }

    return bytes_read;
}
//-------------------------------------------------------------------
// Load from from file. If maxchar != 0 limit charLast (for symbols)
static int rbf_font_load(char *file, font* f, int maxchar)
{
    int i;

    // make sure the font has been allocated
    if (f == 0)
    {
        return 0;
    }

    // open file (can't use fopen here due to potential conflict FsIoNotify crash)
    FILE *fd = FIO_OpenFile(file, O_RDONLY | O_SYNC);
    if (!fd)
    {
        return 0;
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
    memset(&f->wTable[0], 0, sizeof(f->wTable));
    FIO_SeekSkipFile(fd, f->hdr._wmapAddr, SEEK_SET);
    font_read(fd, (unsigned char*)&f->wTable[f->hdr.charFirst], f->charCount);

    // read cTable data (using uncached buffer)
    FIO_SeekSkipFile(fd, f->hdr._cmapAddr, SEEK_SET);
    font_read(fd, (unsigned char*)f->cTable, f->charCount*f->hdr.charSize);

    FIO_CloseFile(fd);

    /* hardcoded tab width: 4 spaces */
    f->wTable['\t'] = f->wTable[' '] * 4;

    return 1;
}

//-------------------------------------------------------------------
static inline int rbf_font_height(font *rbf_font) {
    return rbf_font->hdr.height;
}
//-------------------------------------------------------------------
static inline int rbf_char_width(font *rbf_font, int ch) {
    const struct zh_glyph *glyph = ch >= 32 && ch < 127 ? zh_find_glyph(ch) : 0;
    if (glyph) return zh_glyph_advance(rbf_font, glyph);
    return rbf_font->wTable[ch];
}

//-------------------------------------------------------------------
int rbf_str_width(font *rbf_font, const char *str) {
    int l=0;
    int maxl = 0;

    // Calculate how long the string is in pixels
    // and return the length of the longest line
    while (*str)
    {
        if (*str == '\n')
        {
            maxl = MAX(l, maxl);
            l = 0;
        }
        else
        {
            l += rbf_char_width(rbf_font, *str);
        }
        str++;
    }
    maxl = MAX(l, maxl);

    return maxl;
}

int rbf_str_clipped_width(font *rbf_font, const char *str, int maxlen) {
    int l = 0;
    // Calculate how long the string is in pixels (possibly clipped to 'maxlen')
    while (*str && l+rbf_char_width(rbf_font, *str)<=maxlen)
        l+=rbf_char_width(rbf_font, *str++);

    return l;
}

int rbf_strlen_clipped(font *rbf_font, const char *str, int maxlen) {
    int l = 0;
    // Calculate how long the string is in characters (possibly clipped to 'maxlen')
    char* str0 = (char*) str;
    while (*str && l+rbf_char_width(rbf_font, *str)<=maxlen)
        l+=rbf_char_width(rbf_font, *str++);

    return str - str0;
}

//-------------------------------------------------------------------
static void FAST font_draw_char(font *rbf_font, int x, int y, char *cdata, int width, int height, int pixel_width, int fontspec) {
    int xx, yy;
    uint8_t * bmp = bmp_vram();
    int fg = FG_COLOR(fontspec);
    int bg = BG_COLOR(fontspec);
    int x0 = fontspec & FONT_CONDENSED ? 1 : 0;
    
    // draw pixels for font character
    if (cdata)
    {
        if (bg != NO_BG_ERASE)
        {
            bmp_fill(bg, x, y, pixel_width, height);
        }

        for (yy=0; yy<height; ++yy)
        {
            if (y+yy <= BMP_H_MINUS || y+yy >= BMP_H_PLUS)
            {
                break;
            }
            for (xx=x0; xx<pixel_width; ++xx)
            {
                if(cdata[yy*width/8+xx/8] & (1<<(xx%8)))
                {
                    bmp_putpixel_fast(bmp, x+xx, y+yy, fg);
                }
            }
        }
    }
}

static void FAST font_draw_char_shadow(font *rbf_font, int x, int y, char *cdata, int width, int height, int pixel_width, int fontspec) {
    int xx, yy;
    uint8_t * bmp = bmp_vram();
    int fg = FG_COLOR(fontspec);
    int bg = BG_COLOR(fontspec);
    
    // draw pixels for font character
    if (cdata)
    {
        for (yy=0; yy<height; ++yy)
        {
            if (y+yy <= BMP_H_MINUS || y+yy >= BMP_H_PLUS)
            {
                break;
            }
            for (xx=0; xx<pixel_width; ++xx)
            {
                int px = (cdata[yy*width/8+xx/8] & (1<<(xx%8)));
                if (px)
                {
                    bmp_putpixel_fast(bmp, x+xx, y+yy, fg);
                    
                    /* shadow: background pixels are only drawn near a foreground pixel */
                    /* heuristic: usually there are much less fg pixels than bg, so it makes sense to look for neighbours here */
                    for (int xxx = MAX(xx-1, 0); xxx <= MIN(xx+1, pixel_width-1); xxx++)
                    {
                        for (int yyy = MAX(yy-1, 0); yyy <= MIN(yy+1, height-1); yyy++)
                        {
                            int pxx = (cdata[yyy*width/8+xxx/8] & (1<<(xxx%8)));
                            if (!pxx)
                            {
                                bmp_putpixel_fast(bmp, x+xxx, y+yyy, bg);
                            }
                        }
                    }
                }
            }
        }
    }
}

//-------------------------------------------------------------------
static int FAST rbf_draw_char(font *rbf_font, int x, int y, int ch, int fontspec) {
    const struct zh_glyph *glyph = ch >= 32 && ch < 127 ? zh_find_glyph(ch) : 0;
    if (glyph) return draw_zh_glyph(rbf_font, x, y, glyph, fontspec);

    // Get char data pointer
    char* cdata = rbf_font_char(rbf_font, ch);
    
    if (!rbf_font->cTable)
        bfnt_draw_char(ch, x, y, FG_COLOR(fontspec), BG_COLOR(fontspec));
    else if (fontspec & SHADOW_MASK)
        font_draw_char_shadow(rbf_font, x, y, cdata, rbf_font->width, rbf_font->hdr.height, rbf_font->wTable[ch], fontspec);
    else
        font_draw_char(rbf_font, x, y, cdata, rbf_font->width, rbf_font->hdr.height, rbf_font->wTable[ch], fontspec);

#if 0   /* fixme: breaks cursor in editor.lua */
    if (ch == '\t')
    {
        int tab_width = rbf_font->wTable[' '] * 4;
        return (x + tab_width) / tab_width * tab_width - x;
    }
#endif
    ml_refresh_display_needed = 1;
    return rbf_font->wTable[ch];
}

static uint32_t utf8_next(const char **text)
{
    const unsigned char *s = (const unsigned char *) *text;
    if (s[0] < 0x80)
    {
        (*text)++;
        return s[0];
    }
    if ((s[0] & 0xe0) == 0xc0 && (s[1] & 0xc0) == 0x80)
    {
        *text += 2;
        return ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
    }
    if ((s[0] & 0xf0) == 0xe0 && (s[1] & 0xc0) == 0x80 && (s[2] & 0xc0) == 0x80)
    {
        *text += 3;
        return ((s[0] & 0x0f) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
    }
    (*text)++;
    return '?';
}

static const struct zh_glyph *zh_find_glyph(uint32_t codepoint)
{
    if (!zh_pack) return 0;
    int lo = 0;
    int hi = zh_header->glyph_count - 1;
    while (lo <= hi)
    {
        int mid = lo + (hi - lo) / 2;
        if (codepoint == zh_glyphs[mid].codepoint) return &zh_glyphs[mid];
        if (codepoint < zh_glyphs[mid].codepoint) hi = mid - 1;
        else lo = mid + 1;
    }
    return 0;
}

static int zh_glyph_width(font *rbf_font)
{
    if (rbf_font->hdr.height >= 28) return 24;
    if (rbf_font->hdr.height >= 20) return 15;
    return 12;
}

static int zh_glyph_height(font *rbf_font)
{
    if (rbf_font->hdr.height >= 28) return 32;
    if (rbf_font->hdr.height >= 20) return 20;
    return 16;
}

static int zh_glyph_advance(font *rbf_font, const struct zh_glyph *glyph)
{
    return MAX(1, glyph->advance * zh_glyph_width(rbf_font) / 12);
}

static int utf8_char_width(font *rbf_font, uint32_t codepoint)
{
    const struct zh_glyph *glyph = zh_find_glyph(codepoint);
    return glyph ? zh_glyph_advance(rbf_font, glyph)
                 : rbf_char_width(rbf_font, codepoint < 0x100 ? codepoint : '?');
}

static int draw_zh_glyph(font *rbf_font, int x, int y, const struct zh_glyph *glyph, int fontspec)
{
    int width = zh_glyph_width(rbf_font);
    int advance = zh_glyph_advance(rbf_font, glyph);
    int height = zh_glyph_height(rbf_font);
    int y_offset = (rbf_font->hdr.height - height) / 2;
    int fg = FG_COLOR(fontspec);
    int bg = BG_COLOR(fontspec);
    uint8_t *bmp = bmp_vram();

    if (!bmp) return advance;

    if (!(fontspec & SHADOW_MASK) && bg != NO_BG_ERASE)
        bmp_fill(bg, x, y, advance, rbf_font->hdr.height);

    for (int pass = (fontspec & SHADOW_MASK) ? 0 : 1; pass < 2; pass++)
    {
        for (int row = 0; row < 16; row++)
        {
            for (int col = 0; col < 12; col++)
            {
                if (!(glyph->rows[row] & (1 << (11 - col)))) continue;
                int px = x + col * width / 12;
                int px_end = x + (col + 1) * width / 12;
                int py = y + y_offset + row * height / 16;
                int py_end = y + y_offset + (row + 1) * height / 16;
                int border = pass == 0 ? 1 : 0;
                int color = pass == 0 ? bg : fg;
                for (int yy = py - border; yy < py_end + border; yy++)
                {
                    for (int xx = px - border; xx < px_end + border; xx++)
                    {
                        if (xx >= BMP_W_MINUS && xx < BMP_W_PLUS && yy >= BMP_H_MINUS && yy < BMP_H_PLUS)
                            bmp_putpixel_fast(bmp, xx, yy, color);
                    }
                }
            }
        }
    }
    ml_refresh_display_needed = 1;
    return advance;
}

static int utf8_line_width(font *rbf_font, const char *str)
{
    int width = 0;
    while (*str && *str != '\n')
    {
        uint32_t codepoint = utf8_next(&str);
        width += utf8_char_width(rbf_font, codepoint);
    }
    return width;
}

int rbf_utf8_str_width(font *rbf_font, const char *str)
{
    int width = 0;
    int max_width = 0;
    while (*str)
    {
        uint32_t codepoint = utf8_next(&str);
        if (codepoint == '\n')
        {
            max_width = MAX(width, max_width);
            width = 0;
        }
        else
        {
            width += utf8_char_width(rbf_font, codepoint);
        }
    }
    return MAX(width, max_width);
}

int rbf_utf8_strlen_clipped(font *rbf_font, const char *str, int max_width)
{
    int width = 0;
    const char *start = str;
    while (*str)
    {
        const char *next = str;
        uint32_t codepoint = utf8_next(&next);
        int char_width = utf8_char_width(rbf_font, codepoint);
        if (width + char_width > max_width) break;
        width += char_width;
        str = next;
    }
    return str - start;
}

static int draw_utf8_line(font *rbf_font, int x, int y, const char *str, int fontspec, int max_width)
{
    int width = 0;
    while (*str && *str != '\n')
    {
        uint32_t codepoint = utf8_next(&str);
        int char_width = utf8_char_width(rbf_font, codepoint);
        if (max_width && width + char_width > max_width) break;

        const struct zh_glyph *glyph = zh_find_glyph(codepoint);
        width += glyph
            ? draw_zh_glyph(rbf_font, x + width, y, glyph, fontspec)
            : rbf_draw_char(rbf_font, x + width, y, codepoint < 0x100 ? codepoint : '?', fontspec);
    }
    return width;
}

int rbf_draw_utf8_string(font *rbf_font, int x, int y, const char *str, int fontspec)
{
    int requested_width = FONT_GET_TEXT_WIDTH(fontspec);
    int result = requested_width ? requested_width : rbf_utf8_str_width(rbf_font, str);

    while (*str)
    {
        int content_width = utf8_line_width(rbf_font, str);
        int line_width = requested_width ? requested_width : content_width;
        int draw_x = x;

        if ((fontspec & FONT_ALIGN_MASK) == FONT_ALIGN_CENTER)
            draw_x = x - line_width / 2 + (line_width - content_width) / 2;
        else if ((fontspec & FONT_ALIGN_MASK) == FONT_ALIGN_RIGHT)
            draw_x = x - line_width + line_width - content_width;

        if ((fontspec & FONT_ALIGN_FILL) && !(fontspec & SHADOW_MASK))
        {
            int fill_x = (fontspec & FONT_ALIGN_MASK) == FONT_ALIGN_CENTER ? x - line_width / 2
                       : (fontspec & FONT_ALIGN_MASK) == FONT_ALIGN_RIGHT  ? x - line_width
                       : x;
            bmp_fill(BG_COLOR(fontspec), fill_x, y, line_width, rbf_font->hdr.height);
        }

        draw_utf8_line(rbf_font, draw_x, y, str, fontspec, requested_width);
        while (*str && *str != '\n') str++;
        if (*str == '\n')
        {
            str++;
            y += rbf_font->hdr.height;
        }
    }
    return result;
}


//-------------------------------------------------------------------
// Draw a string colored 'c1' with the character at string-position 'c' colored 'c2'.
static int rbf_draw_string_c(font *rbf_font, int x, int y, const char *str, int fontspec1, int c, int fontspec2) {
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
          l+=rbf_draw_char(rbf_font, x+l, y, *str++, (i==c)?fontspec1:fontspec2);
          ++i;
     }
     return l;
}

//-------------------------------------------------------------------
static int rbf_draw_string_simple(font *rbf_font, int x, int y, const char *str, int fontspec) {
    return rbf_draw_string_c(rbf_font, x, y, str, fontspec, -1, fontspec);
}

//-------------------------------------------------------------------
static int rbf_draw_clipped_string(font *rbf_font, int x, int y, const char *str, int fontspec, int maxlen)
{
    int i = 0;
    int l = 0;
    
    int justified = (fontspec & FONT_ALIGN_MASK) == FONT_ALIGN_JUSTIFIED;
    
    if (justified)
    {
        int should_fill = !(fontspec & SHADOW_MASK);
        int bg = FONT_BG(fontspec);
        int len = maxlen;
        int space = len - rbf_str_width(rbf_font, str);
        int is_mono = rbf_char_width(rbf_font, 'm') == rbf_char_width(rbf_font, 'i');
        
        /* divide the space across the chars: a space character can accept 5 times more stretch space than a regular letter */
        /* first non-letter (indent) can't be stretched (to render bullet points correctly) */
        /* monospaced fonts are condensed uniformly */
        int bins = 0;
        char* c = (char*) str;
        int indent = 1;
        while (*c && *(c+1))    /* note: last char should not be stretched */
        {
            if (*c != ' ' && *c != '*') indent = 0;
            bins += is_mono ? 3 : indent ? 0 : space < 0 ? 1 : *c == ' ' ? 10 : 2;
            c++;
        }
        
        /* use Bresenham line-drawing algorithm to divide space with integer-only math */
        /* http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm#Algorithm_with_Integer_Arithmetic */
        /* x: from 0 to bins */
        /* y: space accumulated (from 0 to space) */
        int dx = bins;
        int dy = ABS(space);
        
        /* if it requires too much stretching, just stretch as much as we can */
        dy = MIN(dy, dx);
        
        int D = 2*dy - dx;

        // Draw chars from string up to max pixel length
        indent = 1;
        int condensed = 0;
        while (*str && l+rbf_char_width(rbf_font, *str)<=maxlen)
        {
            l += rbf_draw_char(rbf_font, x+l, y, *str, fontspec | (condensed ? FONT_CONDENSED : 0) );
            int l0 = l;

            if (*(str+1))
            {
                /* Bresenham step */
                if (*str != ' ' && *str != '*') indent = 0;
                int repeat = is_mono ? 3 : indent ? 0 : space < 0 ? 1 : *c == ' ' ? 10 : 2;
                condensed = 0;
                for (int i = 0; i < repeat; i++)
                {
                    if (D > 0)
                    {
                        l += SGN(space);
                        D = D + (2*dy - 2*dx);
                        if (space < 0) condensed = 1;
                    }
                    else
                    {
                        D = D + 2*dy;
                    }
                }
            }
            
            if (should_fill && l > l0)
            {
                bmp_fill(bg, x+l0, y, l-l0, rbf_font->hdr.height);
            }
            
            i++; str++;
        }
    }
    else
    {
        // Draw chars from string up to max pixel length
        while (*str && l+rbf_char_width(rbf_font, *str)<=maxlen)
        {
            l+=rbf_draw_char(rbf_font, x+l, y, *str++, fontspec);
            i++;
        }
    }
    return l;
}

//-------------------------------------------------------------------
static int rbf_draw_string_single_line(font *rbf_font, int x, int y, const char *str, int fontspec, int maxlen) {
    
    /* how much space do we have to draw the string? */
    int len = maxlen;
    
    // Calulate amount of padding needed
    int padding = len - rbf_str_clipped_width(rbf_font, str, len);

    /* is that padding on the left? on the right? or both? */
    int padding_left = 0;
    int padding_right = 0;
    
    switch (fontspec & FONT_ALIGN_MASK)
    {
        case FONT_ALIGN_LEFT:
            padding_right = padding;
            break;
        
        case FONT_ALIGN_CENTER:
            x -= len/2;
            padding_left = padding/2;
            padding_right = padding - padding/2;
            break;
        
        case FONT_ALIGN_RIGHT:
            padding_left = padding;
            x -= len;
            break;
        
        case FONT_ALIGN_JUSTIFIED:
            padding_left = 0;
            padding_right = 0;
    }
    
    int bg = FONT_BG(fontspec);
    int should_fill = fontspec & FONT_ALIGN_FILL;

    // Fill left padding with background color
    if (should_fill && padding_left)
        bmp_fill(bg, x, y, padding_left, rbf_font->hdr.height);

    // Draw chars
    rbf_draw_clipped_string(rbf_font, x + padding_left, y, str, fontspec, len - padding_right - padding_left);

    // Fill right padding with background color
    if (should_fill && padding_right)
        bmp_fill(bg, x+len-padding_right, y, padding_right, rbf_font->hdr.height);

    return len;
}

int rbf_draw_string(font *rbf_font, int x, int y, const char *str, int fontspec) {

    /* how much space do we have to draw the string? */
    int len = FONT_GET_TEXT_WIDTH(fontspec);

    /* choose some reasonable defaults for text width, if it's not specified */
    if (len == 0)
    {
        if ((fontspec & FONT_ALIGN_MASK) == FONT_ALIGN_LEFT && (fontspec & FONT_ALIGN_FILL) == 0)
        {
            /* no fancy alignment */
            return rbf_draw_string_simple(rbf_font, x, y, str, fontspec);
        }
        else
        {
            /* use natural string length */
            len = rbf_str_width(rbf_font, str);
        }
    }

    char* start = (char*) str;
    char* end = start;

    /* for each line in string */
    while (*start)
    {
        /* where does this line end? */
        while (*end && *end != '\n')
            end++;
        
        /* chop the string here */
        char old = *end;
        *end = 0;
        
        /* draw this line */
        rbf_draw_string_single_line(rbf_font, x, y, start, fontspec, len);
        
        /* finished? */
        if (old == 0)
            break;
        
        /* undo chopping */
        *end = old;
        
        /* okay, let's go to next line */
        y += rbf_font->hdr.height;
        start = end = end+1;
    }
    
    return len;
}


/* for compatibility with existing code */
struct font font_small;
struct font font_med;
struct font font_med_large;
struct font font_large;
struct font font_canon;


#ifdef CONFIG_NO_BFNT
//-------------------------------------------------------------------
// Load bitmap.bfn from card
static int bfnt_load_from_card()
{
    char filename[128];
    //TODO? make it configurable?
    snprintf(filename, sizeof(filename), "ML/FONTS/bitmap.bfn");
    uint32_t size;
    if( FIO_GetFileSize( filename, &size ) != 0 )
        return 1;

    DryosDebugMsg(0, 15, "File '%s' size %d bytes", filename, size);

    BFNT_FONT = (bfnt_font*)read_entire_file(filename, (int*)&size);
    if( BFNT_FONT == NULL ){
        DryosDebugMsg(0, 15, "BFNT read failed.");
        return 2;
    }
    DryosDebugMsg(0, 15, "read ok");

    if( BFNT_FONT->magic != 0x00544e46) { // "FNT\0"
        DryosDebugMsg(0, 15, "Font magic incorrect: 0x%08x", BFNT_FONT->magic);
        free(BFNT_FONT);
        return 3;
    }

    return 0;
}
#endif

/* must be called before menu_init, otherwise it can't measure strings */
void _load_fonts()
{
    /* tolerate multiple calls, but only run the first */
    static int fonts_loaded = 0;
    if (fonts_loaded)
        return;
    fonts_loaded = 1;

    zh_load_from_card();

    int bfnt_status = -1;

    #ifdef CONFIG_NO_BFNT
    //Try to load BFNT from card on newer generations
    bfnt_status = bfnt_load_from_card();
    if ( bfnt_status == 0) {
        DryosDebugMsg(0, 15, "bfnt read OK: %08x %s",  BFNT_FONT, BFNT_FONT->name);
        DryosDebugMsg(0, 15, "hdr %08x char %08x", BFNT_FONT->charmap_offset, BFNT_FONT->charmap_size);

        //setup variables that would be constants from ROM on old generations
        BFNT_CHAR_CODES    = (uint8_t *)BFNT_FONT + BFNT_FONT->charmap_offset;
        BFNT_BITMAP_OFFSET = BFNT_CHAR_CODES + BFNT_FONT->charmap_size;
        BFNT_BITMAP_DATA   = BFNT_BITMAP_OFFSET + BFNT_FONT->charmap_size;

        DryosDebugMsg(0, 15, "%08x %08x %08x", BFNT_CHAR_CODES, BFNT_BITMAP_OFFSET, BFNT_BITMAP_DATA);
    }
    else{
        DryosDebugMsg(0, 15, "bfnt read fail: %d", bfnt_status);
    }
    #else
    //If constants are not zero - assume BFNT is available from ROM
    if(BFNT_CHAR_CODES && BFNT_BITMAP_OFFSET && BFNT_BITMAP_DATA)
        bfnt_status = 0;
    #endif

    /* fake font for Canon BFNT font backend, with the same metrics */
    font *canon_font = new_font();
    canon_font->hdr.height = 40;
    for (int i = 0; i < 256; i++)
        canon_font->wTable[i] = bfnt_char_get_width(i);

    /* Try to load some RBF fonts */
    font_by_name("term12", COLOR_BLACK, COLOR_WHITE);
    font_by_name("term20", COLOR_BLACK, COLOR_WHITE);
    #ifdef CONFIG_LOW_RESOLUTION_DISPLAY
    font_by_name("arghlf22", COLOR_BLACK, COLOR_WHITE);
    #else
    font_by_name("argnor23", COLOR_BLACK, COLOR_WHITE);
    #endif
    font_by_name("argnor28", COLOR_BLACK, COLOR_WHITE);
    font_by_name("argnor32", COLOR_BLACK, COLOR_WHITE);

    if (bfnt_status == 0) {
        /* use BFNT font as fallback */
        for (int i = dyn_fonts; i <= MAX_DYN_FONTS; i++)
        {
            font_dynamic[i].bitmap = (void*) canon_font;
            font_dynamic[i].height = 40;
            font_dynamic[i].width = rbf_char_width((void*)font_dynamic[i].bitmap, '0');
        }

        font_canon = *fontspec_font(FONT_CANON);
    }
    else {
        /* use last loaded RBF font as a fallback */
        font_by_name("argnor32", COLOR_BLACK, COLOR_WHITE);
        if(dyn_fonts)
        {
            for (int i = dyn_fonts; i <= MAX_DYN_FONTS; i++)
                font_dynamic[i] = font_dynamic[dyn_fonts - 1];
        }

        /* Replace missing FONT_CANON with FONT_LARGE */
        font_canon = *fontspec_font(FONT_LARGE);
    }

    font_small = *fontspec_font(FONT_SMALL);
    font_med = *fontspec_font(FONT_MED);
    font_med_large = *fontspec_font(FONT_MED_LARGE);
    font_large = *fontspec_font(FONT_LARGE);
}
