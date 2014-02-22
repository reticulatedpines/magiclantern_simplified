// script console

#include "bmp.h"
#include "dryos.h"
#include "menu.h"
#include "gui.h"
#include "property.h"
#include "config.h"

#ifndef CONFIG_CONSOLE
#error Something went wrong CONFIg_CONSOLE should be defined
#endif

#undef CONSOLE_DEBUG // logs things to file etc

int console_printf(const char* fmt, ...); // how to replace the normal printf?
#define printf console_printf

#define CONSOLE_W 58
#define CONSOLE_H 21
#define CONSOLE_FONT FONT_MONO_20

// buffer is circular and filled with spaces
#define BUFSIZE (CONSOLE_H * CONSOLE_W)
static char console_buffer[BUFSIZE];
static int console_buffer_index = 0;
#define CONSOLE_BUFFER(i) console_buffer[mod((i), BUFSIZE)]

int console_visible = 0;

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
}

void console_toggle()
{
    if (console_visible) console_hide();
    else console_show();
}

void console_set_help_text(char* msg)
{
    snprintf(console_help_text, sizeof(console_help_text), "     %s", msg);
}

void console_set_status_text(char* msg)
{
    snprintf(console_status_text, sizeof(console_status_text), "%s%s", msg, strlen(msg) ? "    " : "");
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

static struct menu_entry script_menu[] = {
    {
        .name       = "Debug Console",
        .priv		= &console_visible,
        .select     = console_toggle_menu,
		.min		= 0,
		.max		= 1,
    },
};
#endif

void console_clear()
{
    int i;
    for (i = 0; i < BUFSIZE; i++)
        console_buffer[i] = ' ';
}

static void console_init()
{
    console_clear();

    #ifdef CONSOLE_DEBUG
    menu_add( "Debug", script_menu, COUNT(script_menu) );
    FIO_RemoveFile("ML/LOGS/console.log");
    #endif
}

void console_puts(const char* str) // don't DebugMsg from here!
{
    #define NEW_CHAR(c) CONSOLE_BUFFER(console_buffer_index++) = (c)
    
    #ifdef CONFIG_QEMU
    qprintf("%s", str);
    #endif

    #ifdef CONSOLE_DEBUG
    bmp_printf(FONT_MED, 0, 0, "%s ", str);

    FILE* f = FIO_CreateFileOrAppend("ML/LOGS/console.log");
    FIO_WriteFile( f, str, strlen(str) );
    FIO_CloseFile(f);
    //~ msleep(100);         /* uncomment this to troubleshoot things that lockup the camera - to make sure FIO tasks actually flushed everything */
    #endif

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
            console_buffer[console_buffer_index] = ' ';
        }
        else
            NEW_CHAR(*c);
        c++;
    }
    console_buffer_index = mod(console_buffer_index, BUFSIZE);
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

void console_show_status()
{
    int fnt = FONT(CONSOLE_FONT,60, COLOR_BLACK);
    bmp_printf(fnt, 0, 480 - font_med.height, console_status_text);
    if (console_visible) bmp_printf(fnt, 720 - font_med.width * strlen(console_help_text), 480 - font_med.height, console_help_text);
}

static void console_draw(int tiny)
{
    int cbpos0 = mod((console_buffer_index / CONSOLE_W) * CONSOLE_W  + CONSOLE_W, BUFSIZE);
    
    /* display last two lines that actually contain something (don't display the cursor-only line) */
    if (tiny && console_buffer_index % CONSOLE_W == 0)
        cbpos0 -= CONSOLE_W;

    int skipped_lines = 0;
    int chopped_columns = 0;

    /* skip empty lines at the top */
    for (int i = 0; i < CONSOLE_H; i++)
    {
        int cbpos = cbpos0 + i * CONSOLE_W;
        int empty = 1;
        for (int j = 0; j < CONSOLE_W; j++)
            if (CONSOLE_BUFFER(cbpos + j) != ' ')
                { empty = 0; break; }
        if (empty) skipped_lines++;
        else break;
    }
    
    if (skipped_lines == CONSOLE_H) // nothing to show
        return;
    
    if (tiny)
        skipped_lines = CONSOLE_H - 3;
    
    /* chop empty columns from the right */
    for (int j = CONSOLE_W-1; j > 0; j--)
    {
        int empty = 1;
        for (int i = skipped_lines; i < CONSOLE_H; i++)
            if (CONSOLE_BUFFER(cbpos0 + i*CONSOLE_W + j) != ' ')
                { empty = 0; break; }
        if (empty) chopped_columns++;
        else break;
    }
    chopped_columns = MIN(chopped_columns, CONSOLE_W - (console_buffer_index % CONSOLE_W));
    
    if (skipped_lines < 5) skipped_lines = 0;
    if (chopped_columns < 5) chopped_columns = 0;

    /* top-left corner of "full" console (without lines/columns skipped) */
    unsigned x0 =  720/2 - fontspec_font(CONSOLE_FONT)->width * CONSOLE_W/2;
    unsigned y0 =  480/2 - fontspec_font(CONSOLE_FONT)->height * CONSOLE_H/2;

    /* correct y to account for skipped lines */
    int yc = y0;
    if (tiny)
    {
        yc = gui_menu_shown() || MENU_MODE ? 415 : y0;
    }
    else
    {
        yc = y0 + fontspec_font(CONSOLE_FONT)->height * skipped_lines;
    }

    int fnt = FONT(CONSOLE_FONT,COLOR_WHITE, (lv || PLAY_OR_QR_MODE) ? COLOR_BG_DARK : COLOR_ALMOST_BLACK);

    int xa = (x0 & ~3) - 1;
    int ya = (yc-1);
    int w = fontspec_font(fnt)->width * (CONSOLE_W - chopped_columns) + 2;
    int h = fontspec_font(fnt)->height * (CONSOLE_H - skipped_lines) + 2;

    /* did the console shrink? if so, redraw Canon GUI around it */
    static int prev_w = 0;
    static int prev_h = 0;
    if (w < prev_w || h < prev_h)
    {
        canon_gui_enable_front_buffer(1); // force a redraw
        prev_w = w;
        prev_h = h;
        //return; // better luck next time :)
    }
    else if (!tiny)
        /* fixme: prevent Canon code from drawing over the console (ugly) */
        canon_gui_disable_front_buffer();
    prev_w = w;
    prev_h = h;


    bmp_draw_rect(60, xa, ya, w, h);
    bmp_draw_rect(COLOR_BLACK, xa-1, ya-1, w+2, h+2);

    /* display each line */
    int found_cursor = 0;
    for (int i = skipped_lines; i < CONSOLE_H; i++)
    {
        char buf[CONSOLE_W+1];
        int cbpos = cbpos0 + i * CONSOLE_W;
        for (int j = 0; j < CONSOLE_W; j++)
        {
            // last character should be on last line => this ensures proper scrolling
            if (mod(cbpos+j, BUFSIZE) == mod(console_buffer_index, BUFSIZE)) // end of data
            {
                if (!found_cursor)
                {
                    buf[j] = '_';
                    found_cursor = 1;
                    continue;
                }
            }
            buf[j] = found_cursor ? ' ' : CONSOLE_BUFFER(cbpos+j);
        }
        buf[CONSOLE_W - chopped_columns] = 0;
        int y = yc + fontspec_font(fnt)->height * (i - skipped_lines);
        bmp_printf(fnt, x0, y, buf);
    }
}

void console_draw_from_menu()
{
    if (console_visible)
        console_draw(1);
}

static void
console_task( void* unused )
{
    console_init();
    #ifdef CONSOLE_DEBUG
    console_show();
    #endif
    int dirty = 0;
    TASK_LOOP
    {
        // show the console only when there are no Canon dialogs on the screen
        if (console_visible && (display_idle() || is_pure_play_photo_or_movie_mode()))
        {
            if (dirty) console_draw(0);
            dirty = 1;
        }
        else if (dirty)
        {
            canon_gui_enable_front_buffer(1);
            dirty = 0;
        }
        else if (console_visible && !gui_menu_shown())
            console_draw(1);


        if (!gui_menu_shown() && strlen(console_status_text))
        {
            console_show_status();
        }

        msleep(200);
    }
}

TASK_CREATE( "console_task", console_task, 0, 0x1d, 0x1000 );
