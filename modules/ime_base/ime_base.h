
#ifndef _ime_base_h_
#define _ime_base_h_

#ifndef _ime_base_c_
#include <beep.h>
#define IME_WEAK(x) WEAK_FUNC(x)
#else
#define IME_WEAK(x)
#endif

#define IME_MAX_METHODS 32

/* for now UTF-8 is the only method that is obligatory */
#define IME_UTF8     0
#define IME_CP437    1
#define IME_CP819    2

/* allowed charsets - basic categories */
#define IME_CHARSET_ALPHA       0x01
#define IME_CHARSET_NUMERIC     0x02
#define IME_CHARSET_PUNCTUATION 0x04
#define IME_CHARSET_SPECIAL     0x08
#define IME_CHARSET_MATH        0x10
#define IME_CHARSET_MAIL        0x20
#define IME_CHARSET_FILENAME    0x40
#define IME_CHARSET_HEX         0x80

/* all characters are allowed, passing parameter 0x00 will also map to this */
#define IME_CHARSET_ANY         0xFFFFFFFF

/* return codes */
#define IME_OK           0
#define IME_CANCEL       1
#define IME_ERR_UNAVAIL  2
#define IME_ERR_UNKNOWN  3


/* called whenever the string or the cursor position has changed.
   when x/y/w/h parameters for ime_base_start were other than zero, this function has to handle text update.
   this function is also used to validate the entered string.
   if it returns IME_OK, the string is valid,
   if it returns != 0, the IME knows that the string is invalid and grays out the OK functionality
   
   'selection_length' specifies how many characters starting from caret_pos are selected. 0 if none (plain cursor)
 */
typedef uint32_t (*t_ime_update_cbr)(void *ctx, char *text, int32_t caret_pos, int32_t selection_length);
#define IME_UPDATE_FUNC(func) uint32_t func(void *ctx, char *text, int32_t caret_pos, int32_t selection_length)

/* this callback is called when the dialog box was accepted or cancelled
   when the string was entered, status will be IME_OK.
   if the user aborted input, status will be IME_CANCEL.
 */
typedef uint32_t (*t_ime_done_cbr)(void *ctx, uint32_t status, char *text);
#define IME_DONE_FUNC(func) uint32_t func(void *ctx, uint32_t status, char *text)

/* call this function to start the IME system - this is asynchronous if done_cbr is != NULL.
   it will call 'update' if (update != NULL) periodically or on any update_cbr (caret pos or string) and done_cbr when the dialog is finished.
   return the context of the dialog if it was started. this is a paramete for future functions and used to identify the exact dialog.
   
   the passed text buffer must reserve max_length characters, including the null byte. e.g. you can pass sizeof(buffer) if it is an array.
   
   in case of any other error (e.g. unavailability of some resource) it will return NULL.
   if an error occured, the error message will be placed in the 'text' pointer given, so make sure you use a separate buffer.
   
   when the IME method supports this feature (i.e. non-fullscreen methods) the parameters x, y, w, h specify where the caller
   placed its text field that should not be overwritten. your update cbr must handle displaying the string in this case. 
   if you dont care about this, pass all values as zero. 
 */
extern IME_WEAK(ret_0) void * ime_base_start (char *caption, char *text, int32_t max_length, int32_t codepage, int32_t charset, t_ime_update_cbr update_cbr, t_ime_done_cbr done_cbr, int32_t x, int32_t y, int32_t w, int32_t h );
typedef void * (*t_ime_start) (char *caption, char *text, int32_t max_length, int32_t codepage, int32_t charset, t_ime_update_cbr update_cbr, t_ime_done_cbr done_cbr, int32_t x, int32_t y, int32_t w, int32_t h );

/* this structure is passed when registering */
typedef struct 
{
    char *name;
    char *description;
    t_ime_start start;
    void (*configure)();
} t_ime_handler;

/* IME modules call this function to register themselves. threadsafe */
#ifndef _ime_base_c_
static uint32_t ime_base_unavail(t_ime_handler *handler)
{
    bmp_printf(FONT_MED, 30, 190, "IME Handler %s installed, but 'ime_base' missing.", handler->name);
    beep();
    msleep(2000);
    return IME_ERR_UNAVAIL;
}

extern IME_WEAK(ime_base_unavail) uint32_t ime_base_register(t_ime_handler *handler);

#endif


#endif
