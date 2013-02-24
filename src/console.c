// script console

#include "bmp.h"
#include "dryos.h"
#include "menu.h"
#include "gui.h"
#include "property.h"
#include "config.h"

#undef CONSOLE_DEBUG // logs things to file etc

int console_printf(const char* fmt, ...); // how to replace the normal printf?
#define printf console_printf

#define CONSOLE_W 58
#define CONSOLE_H 21
#define CONSOLE_FONT FONT_MED

// buffer is circular and filled with spaces
#define BUFSIZE (CONSOLE_H * CONSOLE_W)
static char* console_buffer = 0;
static char* console_puts_buffer = 0; // "normal" copy of the circular buffer
static int console_buffer_index = 0;

int console_visible = 0;

#ifdef CONSOLE_DEBUG
static FILE* console_log_file = 0;
#endif

static char console_help_text[40];
static char console_status_text[40];

void console_show()
{
    console_visible = 1;
    redraw();
}
void console_hide()
{
    console_visible = 0;
    msleep(100);
    canon_gui_enable_front_buffer(1);

    #ifdef CONSOLE_DEBUG
    FIO_CloseFile(console_log_file);
    console_log_file = 0;
    #endif
}

void console_toggle()
{
    if (console_visible) console_hide();
    else console_show();
}

void console_set_help_text(char* msg)
{
    snprintf(console_help_text, sizeof(console_help_text), "%s", msg);
}

void console_set_status_text(char* msg)
{
    snprintf(console_status_text, sizeof(console_status_text), "%s", msg);
}
static void
console_toggle_menu( void * priv, int delta )
{
    if (console_visible) console_hide();
    else console_show();
}

#ifdef CONSOLE_DEBUG
static void
console_test( void * priv )
{
    console_visible = 1;
    printf("Hello World!\n");
    printf("The quick brown fox jumps over the lazy dog. Computer programs expand so as to fill the core available. El trabajo y la economia son la mejor loteria. \n");
}

static void
console_print( void * priv, int x, int y, int selected )
{
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Debug Console : %s",
        *(unsigned*) priv ? "ON " : "OFF"
    );
}

static struct menu_entry script_menu[] = {
    {
        .priv		= &console_visible,
        .display    = console_print,
        .select     = console_toggle_menu,
		.min		= 0,
		.max		= 1,
    },
};
#endif

void console_clear()
{
    if (!console_buffer) return;
    int i;
    for (i = 0; i < BUFSIZE; i++)
        console_buffer[i] = ' ';
}

static void console_init()
{
    console_buffer = SmallAlloc(BUFSIZE+32);
    console_puts_buffer = SmallAlloc(BUFSIZE+32);

    console_clear();

    #ifdef CONSOLE_DEBUG
    menu_add( "Debug", script_menu, COUNT(script_menu) );

	msleep(500);

	if (!console_log_file) {
	    console_log_file = FIO_CreateFileEx(CARD_DRIVE "ML/LOGS/console.log");
	}
    #endif
}

void console_puts(const char* str) // don't DebugMsg from here!
{
    #define NEW_CHAR(c) console_buffer[mod(console_buffer_index++, BUFSIZE)] = (c)

    #ifdef CONSOLE_DEBUG
    if (console_log_file)
        my_fprintf( console_log_file, "%s", str );
    #endif

    if (!console_buffer) return;
    const char* c = str;
    while (*c)
    {
        if (*c == '\n')
        {
            if (mod(console_buffer_index, CONSOLE_W) == 0)
                NEW_CHAR(' ');
            while (mod(console_buffer_index, CONSOLE_W) != 0)
                NEW_CHAR(' ');
        }
        else if (*c == '\t')
        {
            NEW_CHAR(' ');
            while (mod(mod(console_buffer_index, CONSOLE_W), 4) != 0)
                NEW_CHAR(' ');
        }
        else if (*c == 8)
        {
            console_buffer_index = mod(console_buffer_index - 1, BUFSIZE);
            console_buffer[mod(console_buffer_index, BUFSIZE)] = 0;
        }
        else
            NEW_CHAR(*c);
        c++;
    }
    console_buffer[mod(console_buffer_index, BUFSIZE)] = 0;
}

int console_printf(const char* fmt, ...) // don't DebugMsg from here!
{
    char buf[256];
    va_list         ap;
    va_start( ap, fmt );
    int len = vsnprintf( buf, 255, fmt, ap );
    va_end( ap );
    console_puts(buf);
	return len;
}

// used from Lua
int console_vprintf(const char* fmt, va_list ap) // don't DebugMsg from here!
{
    char buf[256];
    int len = vsnprintf( buf, 255, fmt, ap );
    console_puts(buf);
	return len;
}

static void console_show_status()
{
    int fnt = FONT(CONSOLE_FONT,60, COLOR_BLACK);
    bmp_printf(fnt, 0, 480 - font_med.height + 2, console_status_text);
    if (console_visible) bmp_printf(fnt, 720 - font_med.width * strlen(console_help_text), 480 - font_med.height + 2, console_help_text);
}

static void console_draw()
{
    if (!console_buffer) return;
    if (!console_puts_buffer) return;
    unsigned x0 =  720/2 - fontspec_font(CONSOLE_FONT)->width * CONSOLE_W/2;
    unsigned y0 =  480/2 - fontspec_font(CONSOLE_FONT)->height * CONSOLE_H/2;
    //unsigned w = fontspec_font(CONSOLE_FONT)->width * CONSOLE_W;
    //unsigned h = fontspec_font(CONSOLE_FONT)->height * CONSOLE_H;
    int i;

    int found_cursor = 0;
    for (i = 0; i < BUFSIZE; i++)
    {
        // last character should be on last line => this ensures proper scrolling
        int cbpos = mod((console_buffer_index / CONSOLE_W) * CONSOLE_W  + CONSOLE_W + i, BUFSIZE);
        if (console_buffer[cbpos] == 0) // end of data
        {
            if (!found_cursor)
            {
                console_puts_buffer[i] = '_';
                found_cursor = 1;
                continue;
            }
        }
        console_puts_buffer[i] = found_cursor ? ' ' : console_buffer[cbpos];
    }
    console_puts_buffer[BUFSIZE] = 0;
    
    canon_gui_disable_front_buffer();
    int xa = (x0 & ~3) - 1;
    int ya = (y0-1);
    int w = fontspec_font(CONSOLE_FONT)->width * CONSOLE_W + 2;
    int h = fontspec_font(CONSOLE_FONT)->height * CONSOLE_H + 2;
    int fnt = FONT(CONSOLE_FONT,COLOR_WHITE, (lv || PLAY_OR_QR_MODE) ? COLOR_BG_DARK : COLOR_ALMOST_BLACK);
    bmp_draw_rect(60, xa, ya, w, h);
    bmp_puts_w(fnt, &x0, &y0, CONSOLE_W, console_puts_buffer);
}


static void
console_task( void* unused )
{
    console_init();
    TASK_LOOP
    {
        if (console_visible && !gui_menu_shown())
        {
            console_draw();
        }

        if (!gui_menu_shown() && strlen(console_status_text))
        {
            console_show_status();
        }

        msleep(200);
    }
}

TASK_CREATE( "console_task", console_task, 0, 0x1f, 0x1000 );
