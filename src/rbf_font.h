#ifndef RBF_FONT_H
#define RBF_FONT_H

// CHDK Font interface

// Note: used in modules and platform independent code. 
// Do not add platform dependent stuff in here (#ifdef/#endif compile options or camera dependent values)


//-------------------------------------------------------------------
#define RBF_MAX_NAME        64

/* kitor: BFNT font header, as seen in Digic5 and below.
 * Taken from Trammells post:
 * https://groups.google.com/g/ml-devel/c/rsTIDu8c3Wo/m/jftkwmD0ABkJ
 *
 * Perhaps we should update all the old models code to pointer to this header
 * instead of using direct pointers to specific parts and hardcoded sizes... */
typedef struct
{
    uint32_t magic;           // 0x464e5400 (0x00544e46 LE) == "FNT\0"
    uint16_t off_0x4;         // 0xffe2 in most of them?
    uint16_t font_width;      // off_0x6;
    uint32_t charmap_offset;  // off_0x8, typicaly 0x24
    uint32_t charmap_size;    // off_0xc
    uint32_t bitmap_size;     // off_0x10
    const char name[16];
} bfnt_font;

#ifdef CONFIG_NO_BFNT
/* kitor: Those replace entries in platform consts.h for DIGIC6+
 *        that does not use BMP fonts anymore.  */
extern uint8_t *BFNT_CHAR_CODES;
extern uint8_t *BFNT_BITMAP_OFFSET;
extern uint8_t *BFNT_BITMAP_DATA;

extern bfnt_font* BFNT_FONT;
#endif

//-------------------------------------------------------------------
// Format of header block for each character in the 'font_data' array
// This is immediately followed by 'size' bytes of character data.
typedef struct {
    unsigned char charcode[2];      // Don't change this to a short as the data is not aligned in memory
    unsigned char offset;
    unsigned char size;
} FontData;


// Header as seperate structure so it can be directly loaded from the font file easily
// structure layout maps to file layout - do not change !
typedef struct {
    int magic1, magic2;         // header magic numbers to identify correct font file
    char name[RBF_MAX_NAME];    // name of font (max 64 characters)
    int charSize;               // # of bytes used to store each character
    int points;                 // font size in points
    int height;                 // font height in pixels
    int maxWidth;               // width of widest character
    int charFirst;              // first character #
    int charLast;               // last character #
    int _unknown4;              // ?
    int _wmapAddr;              // offset in font file of wTable array
    int _cmapAddr;              // offset in font file of cTable array
    int descent;                // font descent (not used)
    int intline;                // interline spacing (not used)
} font_hdr;

typedef struct _font {
    font_hdr hdr;

    // calculated values (after font is loaded)
    int charCount;              // count of chars containing in font
    int width;                  // font element width in pixels

    // Width table
    // List of character widths. Elements of list is width of char 
    char wTable[256];

    // Character data
    // List of chars. Element of list is a bytecode string, contains pixels representation of char
    char *cTable;

    // Current size of the cTable data
    int cTableSize;
    int cTableSizeMax;                // max size of cTable (max size currently allocated)
} font;

//-------------------------------------------------------------------
// private functions for bmp.c; for user code, please use BMP and fontspec API (bmp_printf, bmp_string_width, FONT_MED...)
extern int rbf_str_width(font *rbf_font, const char *str);
extern int rbf_str_clipped_width(font *rbf_font, const char *str, int maxlen);
extern int rbf_strlen_clipped(font *rbf_font, const char *str, int maxlen);
extern int rbf_draw_string(font *rbf_font, int x, int y, const char *str, int cl);
//-------------------------------------------------------------------

/* to be called at startup, before init funcs */
void _load_fonts();

#endif
