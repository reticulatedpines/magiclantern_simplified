#include "dryos.h"
#include "menu.h"
#include "bmp.h"
#include "config.h"

#include "picoc.h"

static int script_running = 0;
static int script_preview_flag = 0;
static char script_preview[1000] = "";
static int script_preview_dirty = 1;

/* script functions */
#define MAX_SCRIPT_NUM 9
#define LINE_BUF_SIZE 15
#define PICOC_HEAP_SIZE (30*1024)

static char script_list[MAX_SCRIPT_NUM][LINE_BUF_SIZE];
static CONFIG_INT("script.selected", script_selected, 0);
static int script_cnt = 0;

/* modify from is_valid_cropmark_filename */
static int is_valid_script_filename(char* filename)
{
    int n = strlen(filename);
    if ((n > 2) && (streq(filename + n - 2, ".C") || streq(filename + n - 2, ".c")) && (filename[0] != '.') && (filename[0] != '_'))
        return 1;
    return 0;
}

static void find_scripts(void)
{
    // only try to run once
    static int scripts_found = 0;
    if (scripts_found) return;
    scripts_found = 1;
    
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( CARD_DRIVE "ML/SCRIPTS/", &file );
    if( IS_ERROR(dirent) )
    {
        NotifyBox(2000, "Scripts dir missing" );
        return;
    }
    script_cnt = 0;
    do {
        if ((file.mode & 0x20) && is_valid_script_filename(file.name)) {
            
            snprintf(script_list[script_cnt++], LINE_BUF_SIZE, "%s", file.name);

            if (script_cnt >= MAX_SCRIPT_NUM)
            {
                NotifyBox(2000, "Too many scripts" );
                break;
            }
        }
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);
}

static void
script_select_display( void * priv, int x, int y, int selected )
{
    find_scripts();
    script_selected = COERCE(script_selected, 0, script_cnt - 1);

    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Select script: %s",
        script_list[script_selected]
    );
    menu_draw_icon(x, y, MNI_DICE, (script_cnt<<16) + script_selected);
}

static void
script_run_display( void * priv, int x, int y, int selected )
{
    if (script_preview_flag)
    {
        menu_draw_icon(x, y, -1, 0);
        return;
    }

    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        script_running ? "Script running..." : "Run script"
    );
    
    if (script_running) menu_draw_icon(x, y, MNI_WARNING, 0);
}

static char* get_current_script_path()
{
    static char path[50];
    snprintf(path, sizeof(path), CARD_DRIVE"ML/SCRIPTS/%s", script_list[script_selected]);
    return path;
}

static void script_copy_window(char* dst, int bufsize, char* src, int line0, int col0, int maxlines, int maxcols)
{
    int l = 0;
    int c = 0;
    char* d_end = dst + bufsize;
    char* s = src;
    char* d = dst;
    for ( ; *s != 0 && d < d_end-1; s++)
    {
        // our character falls within our window? if so, copy it
        int line_condition = (l >= line0 && l < line0 + maxlines);
        int column_condition = (c >= col0 && c < col0 + maxcols) || (*s == '\n');
        if (line_condition && column_condition)
        {
            *d = *s;
            d++;
        }
        else if (c >= col0 + maxcols)
        {
            *(d-1) = *(d-2) = *(d-3) = '.';
        }
        
        // update current position in source string
        if (*s == '\n') { l++; c=0; } else c++;
    }
    *d = 0;
}

static void
script_print( void * priv, int x, int y, int selected )
{
    if (!script_preview_flag || !selected)
    {
        bmp_printf(
            MENU_FONT,
            x, y,
            "Show script"
        );
        script_preview_flag = 0;
        return;
    }

    if (script_preview_dirty)
    {
        int size;
        char* p = get_current_script_path();
        char* f = (char*)read_entire_file(p, &size);
        if (f)
        {
            script_copy_window(script_preview, sizeof(script_preview), f, 0, 0, 20, 60);
            free_dma_memory(f);
        }
        else
        {
            snprintf(script_preview, sizeof(script_preview), "Could not read '%s'", p);
        }
        script_preview_dirty = 0;
    }

    bmp_fill(40, 0, 0, 720, 430);
    int fnt = FONT(FONT_MED, COLOR_WHITE, 40);
    big_bmp_printf(fnt, 10, 10, "%s", script_preview);
    menu_draw_icon(x, y, -1, 0);
}

static void script_select(void* priv, int delta)
{
    script_selected = mod(script_selected + delta, script_cnt);
    script_preview_dirty = 1;
}

static void run_script(const char *script)
{
    script_running = 1;

    msleep(1000);
    console_show();
    msleep(500);
    extern int PicocExitBuf[];
    PicocInitialise(PICOC_HEAP_SIZE);
    PicocExitValue = 0;
    setjmp(PicocExitBuf); // in case of error, code will jump back here
    if (PicocExitValue)
    {
        console_puts(  "=============  :(  ===========\n\n");
        msleep(1000);
    }
    else
    {
        PicocPlatformScanFile(get_current_script_path());
    }
    PicocCleanup();
    beep();
    script_running = 0;
    
    for (int i = 0; i < 1000; i++)
    {
        msleep(100);
        if (gui_menu_shown()) break;
        if (get_halfshutter_pressed()) break;
    }
    console_hide();
}

static void script_run_fun(void* priv, int delta)
{
    if (script_running) return;
    gui_stop_menu();
    task_create("run_script", 0x1c, 0x4000, run_script, 0);
}

/* end script functions */

extern void menu_open_submenu();

static struct menu_entry picoc_menu[] = {
    {
        .name = "PicoC scripts...",
        .select = menu_open_submenu,
        .help = "Run small C-like scripts. http://code.google.com/p/picoc/",
        .children =  (struct menu_entry[]) {
            {
                .name = "Select script",
                .priv = &script_selected,
                .display    = script_select_display,
                .select        = script_select,
                .help = "Place your scripts under ML/SCRIPTS directory.",
            },
            {
                .name = "Show script",
                .priv = &script_preview_flag,
                .max = 1,
                .icon_type = IT_ACTION,
                .display    = script_print,
                .help = "Display the contents of the selected script.",
            },
            {
                .name = "Run script",
                .display    = script_run_display,
                .select        = script_run_fun,
                .help = "Execute the selected script.",
            },
            MENU_EOL,
        }
    },
};

static void picoc_init()
{
    menu_add("Debug", picoc_menu, COUNT(picoc_menu));
}

INIT_FUNC(__FILE__, picoc_init);
