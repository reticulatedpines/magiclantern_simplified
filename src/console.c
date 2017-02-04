// script console

#include "bmp.h"
#include "dryos.h"
#include "menu.h"
#include "gui.h"
#include "property.h"
#include "config.h"
#include "zebra.h"
#include "shoot.h"
#include "alloca.h"

#ifndef CONFIG_CONSOLE
#error Something went wrong CONFIg_CONSOLE should be defined
#endif

#undef CONSOLE_DEBUG // logs things to file etc

#define CONSOLE_W 80
#define CONSOLE_H 21
#define CONSOLE_FONT FONT_MONO_20

// buffer is circular and filled with spaces
#define BUFSIZE (CONSOLE_H * CONSOLE_W)
static char console_buffer[BUFSIZE]
    #ifndef PYCPARSER
    = {[0 ... BUFSIZE-1] = ' '}
    #endif
;
static int console_buffer_index = 0;
#define CONSOLE_BUFFER(i) console_buffer[MOD((i), BUFSIZE)]

int console_visible = 0;

void console_show()
{
    console_visible = 1;
    redraw();
}
void console_hide()
{
    console_visible = 0;
    msleep(100);
    redraw();
}

void console_toggle()
{
    if (console_visible) console_hide();
    else console_show();
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
    #ifdef CONSOLE_DEBUG
    menu_add( "Debug", script_menu, COUNT(script_menu) );
    FIO_RemoveFile("ML/LOGS/console.log");
    #endif
}

void console_puts(const char* str) // don't DebugMsg from here!
{
    #define NEW_CHAR(c) CONSOLE_BUFFER(console_buffer_index++) = (c)

    /* this only runs when compiling with CONFIG_QEMU */
    qprintf("%s", str);

    #ifdef CONSOLE_DEBUG
    bmp_printf(FONT_MED, 0, 0, "%s ", str);

    FILE* f = FIO_CreateFileOrAppend("ML/LOGS/console.log");
    if (f)
    {
        FIO_WriteFile( f, str, strlen(str) );
        FIO_CloseFile(f);
    }
    //~ msleep(100);         /* uncomment this to troubleshoot things that lockup the camera - to make sure FIO tasks actually flushed everything */
    #endif

    /* for handling carriage returns */
    static int cr = 0;
    
    const char* c = str;
    while (*c)
    {
        if (*c == '\n')
        {
            if (MOD(console_buffer_index, CONSOLE_W) == 0)
                NEW_CHAR(' ');
            while (MOD(console_buffer_index, CONSOLE_W) != 0)
                NEW_CHAR(' ');
            cr = 0;
        }
        else if (*c == '\t')
        {
            NEW_CHAR(' ');
            while (MOD(console_buffer_index, 4) != 0)
                NEW_CHAR(' ');
        }
        else if (*c == '\b')
        {
            /* only erase on current line */
            if (MOD(console_buffer_index, CONSOLE_W) != 0)
            {
                console_buffer_index--;
                CONSOLE_BUFFER(console_buffer_index) = ' ';
            }
        }
        else if (*c == '\r')
        {
            cr = 1; /* will handle it later */
        }
        else
        {
            if (cr) /* need to handle a carriage return without line feed */
            {
                while (MOD(console_buffer_index, CONSOLE_W))
                    console_buffer_index--;
                cr = 0;
            }
            NEW_CHAR(*c);
        }
        c++;
    }
    
    console_buffer_index = MOD(console_buffer_index, BUFSIZE);
}

static void console_draw(int tiny)
{
    int cbpos0 = MOD((console_buffer_index / CONSOLE_W) * CONSOLE_W  + CONSOLE_W, BUFSIZE);
    
    /* display last two lines that actually contain something (don't display the cursor-only line) */
    if (1 && console_buffer_index % CONSOLE_W == 0)
    {
        cbpos0 -= CONSOLE_W;
    }

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
    
    if (1)
    {
        skipped_lines = MAX(skipped_lines, CONSOLE_H - (tiny ? 3 : 15));
    }
    
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
    
    //if (skipped_lines < 5) skipped_lines = 0;
    if (chopped_columns < 5) chopped_columns = 0;

    /* top-left corner of "full" console (without lines/columns skipped) */
    unsigned x0 =  (chopped_columns < 7) ? 0 : 8;
    //unsigned y0 =  480/2 - fontspec_font(CONSOLE_FONT)->height * CONSOLE_H/2;

    /* correct y to account for skipped lines */
    int yc = 476 - fontspec_font(CONSOLE_FONT)->height * (CONSOLE_H - skipped_lines);

    if (lv && !gui_menu_shown())
    {
        /* align with ML info bars */
        extern int get_ml_topbar_pos();
        extern int get_ml_bottombar_pos();
        int yt = get_ml_topbar_pos();
        int yb = get_ml_bottombar_pos();
        if (yt > os.y0 + os.y_ex / 2)
        {
            yb = yt;
        }
        yc -= (490 - yb) ;
    }

    int fnt = FONT(CONSOLE_FONT,COLOR_WHITE, (lv || PLAY_OR_QR_MODE) ? COLOR_BG_DARK : COLOR_ALMOST_BLACK);

    int w = MIN((chopped_columns < 7) ? 720 : 704, fontspec_font(fnt)->width * (CONSOLE_W - chopped_columns) + 2);
    int h = fontspec_font(fnt)->height * (CONSOLE_H - skipped_lines);

    /* did the console shrink? if so, redraw Canon GUI around it */
    static int prev_w = 0;
    static int prev_h = 0;
    if (w < prev_w || h < prev_h)
    {
        redraw();
        prev_w = w;
        prev_h = h;
        return; // better luck next time :)
    }
    prev_w = w;
    prev_h = h;

    /* display each line */
    int found_cursor = 0;
    int printed_width = 0;
    for (int i = skipped_lines; i < CONSOLE_H; i++)
    {
        char buf[CONSOLE_W+1];
        int cbpos = cbpos0 + i * CONSOLE_W;
        for (int j = 0; j < CONSOLE_W; j++)
        {
            // last character should be on last line => this ensures proper scrolling
            if (MOD(cbpos+j, BUFSIZE) == MOD(console_buffer_index, BUFSIZE)) // end of data
            {
                if (!found_cursor)
                {
                    buf[j] = '_';
                    found_cursor = 1;
                    continue;
                }
            }
            buf[j] = found_cursor ? ' ' : CONSOLE_BUFFER(cbpos+j);
            if (buf[j] == 0) buf[j] = '?';
        }
        buf[CONSOLE_W - chopped_columns] = 0;
        int y = yc + fontspec_font(fnt)->height * (i - skipped_lines);
        printed_width = bmp_printf(fnt | FONT_ALIGN_JUSTIFIED | FONT_TEXT_WIDTH(w), x0, y, "%s", buf);
    }
    
    bmp_draw_rect(60, x0-1, yc-1, printed_width+2, h+2);
    bmp_draw_rect(COLOR_BLACK, x0-2, yc-2, printed_width+4, h+4);

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
            redraw();
            dirty = 0;
        }
        else if (console_visible && !gui_menu_shown())
        {
            console_draw(1);
        }

        msleep(200);
    }
}

TASK_CREATE( "console_task", console_task, 0, 0x1d, 0x1000 );

/* some functions from standard I/O */

int printf(const char* fmt, ...)
{
    /* when called from init_task, 512 bytes are enough to cause stack overflow */
    int buf_size = (streq(current_task->name, "init")) ? 64 : 512;
    char* buf = alloca(buf_size);
    
    va_list         ap;
    va_start( ap, fmt );
    int len = vsnprintf( buf, buf_size-1, fmt, ap );
    va_end( ap );
    console_puts(buf);
    return len;
}

int puts(const char * fmt)
{
    console_puts(fmt);
    console_puts("\n");
    return 0;
}
