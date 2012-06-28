// script console

#include "bmp.h"
#include "dryos.h"
#include "menu.h"
#include "gui.h"
#include "property.h"
#include "config.h"

int console_printf(const char* fmt, ...); // how to replace the normal printf?
#define printf console_printf

#define CONSOLE_W 55
#define CONSOLE_H 15
#define CONSOLE_FONT FONT_MED

// buffer is circular and filled with spaces
#define BUFSIZE (CONSOLE_H * CONSOLE_W)
char* console_buffer = 0;

char* console_puts_buffer = 0; // "normal" copy of the circular buffer

int console_buffer_index = 0;

CONFIG_INT("debug.console.visible",console_visible,0);

FILE* console_log_file = 0;
void console_show()
{
	console_visible = 1;
}
void console_hide()
{
    console_visible = 0;
    msleep(500);
    clrscr();
    FIO_CloseFile(console_log_file);
    console_log_file = 0;
}

static void
console_toggle( void * priv, int delta )
{
    if (console_visible) console_hide();
    else console_show();
}

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
        .select     = console_toggle,
		.min		= 0,
		.max		= 1,
    },
};

void console_clear()
{
    if (!console_buffer) return;
    int i;
    for (i = 0; i < BUFSIZE; i++)
        console_buffer[i] = ' ';
}

void console_init()
{
    console_buffer = AllocateMemory(BUFSIZE+32);
    console_puts_buffer = AllocateMemory(BUFSIZE+32);

    console_clear();

    menu_add( "Debug", script_menu, COUNT(script_menu) );

	msleep(500);

	if (!console_log_file) {
	    console_log_file = FIO_CreateFileEx(CARD_DRIVE "ML/LOGS/console.log");
	}
}

void console_puts(const char* str) // don't DebugMsg from here!
{
    #define NEW_CHAR(c) console_buffer[mod(console_buffer_index++, BUFSIZE)] = (c)

    if (console_log_file)
        my_fprintf( console_log_file, "%s", str );

    if (!console_buffer) return;
    const char* c = str;
    while (*c)
    {
        if (*c == '\n')
            while (mod(console_buffer_index, CONSOLE_W) != 0)
                NEW_CHAR(' ');
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
    int len = vsnprintf( buf, 256, fmt, ap );
    va_end( ap );
    console_puts(buf);
	return len;
}

int console_vprintf(const char* fmt, va_list ap) // don't DebugMsg from here!
{
    char buf[256];
    int len = vsnprintf( buf, 256, fmt, ap );
    console_puts(buf);
	return len;
}


void console_draw()
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
    bmp_puts_w(FONT(CONSOLE_FONT,COLOR_WHITE,COLOR_BG_DARK), &x0, &y0, CONSOLE_W, console_puts_buffer);
}


static void
console_task( void* unused )
{
    console_init();
    while(1)
    {
        if (console_visible && !gui_menu_shown())
        {
            console_draw();
        }
        msleep(200);
    }
}

TASK_CREATE( "console_task", console_task, 0, 0x1f, 0x1000 );
