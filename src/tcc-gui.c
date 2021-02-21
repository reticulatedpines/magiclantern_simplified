#include "dryos.h"
#include "menu.h"
#include "bmp.h"
#include "config.h"

static int script_state = 0;
#define SCRIPT_RUNNING 1
#define SCRIPT_JUST_FINISHED -1
#define SCRIPT_IDLE 0

int script_stop_requested = 0; // fixme: not used in tcc

static int script_preview_flag = 0;
static char script_preview[1000] = "";

/* script functions */
#define MAX_SCRIPT_NUM 15
#define FILENAME_SIZE 15
#define SCRIPT_TITLE_SIZE 21

static char script_list[MAX_SCRIPT_NUM][FILENAME_SIZE];
static char script_titles[MAX_SCRIPT_NUM][SCRIPT_TITLE_SIZE];

static int script_selected = 0;
static int script_cnt = 0;

void script_reset_params();

static char* get_script_path(int script_index)
{
    static char path[50];
    snprintf(path, sizeof(path), "ML/SCRIPTS/%s", script_list[script_index]);
    return path;
}

static void guess_script_title_from_first_line(char* src, char* dst, int maxsize)
{
    while (isspace(*src) || *src == '/' || *src == '*') // skip comments
        src++;

    snprintf(dst, maxsize, "%s", src);
    
    // trim at first newline
    for (char* c = dst; *c; c++)
        if (*c == '\n') { *c = 0; break; }
    
    // skip comment chars at the end of line
    char* last = dst + strlen(dst) - 1;
    while (isspace(*last) || *last == '/' || *last == '*')
        last--;
    last++;
    if (last > dst)
        *last = 0;
}
static void script_parse_header(int script_index)
{
    script_selected = script_index; // side effect (!)

    // clear params submenu (hide all unused stuff)
    script_reset_params();

    // try to read the script 
    static char _buf[1024+128];
    char* buf = UNCACHEABLE(_buf)+64;
    char* fn = get_script_path(script_selected);
    int r = read_file(fn, buf, 1024);
    if (r < 0)
    {
        snprintf(script_titles[script_index], SCRIPT_TITLE_SIZE, "Error");
        return;
    }
    buf[r+1] = 0;

    // get some default title
    guess_script_title_from_first_line(buf, script_titles[script_index], SCRIPT_TITLE_SIZE);
    
    // parse CHDK script header
    script_scan(fn, buf);
    
    // update submenu
    script_update_menu();
}

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
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( "ML/SCRIPTS/", &file );
    if( IS_ERROR(dirent) )
    {
        NotifyBox(2000, "Scripts dir missing" );
        return;
    }
    script_cnt = 0;
    do {
        if (file.mode & ATTR_DIRECTORY) continue; // is a directory
        if (is_valid_script_filename(file.name)) {
            
            snprintf(script_list[script_cnt++], FILENAME_SIZE, "%s", file.name);

            if (script_cnt >= MAX_SCRIPT_NUM)
            {
                NotifyBox(2000, "Too many scripts" );
                break;
            }
        }
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_FindClose(dirent);
    
    for (int i = 0; i < script_cnt; i++)
        script_parse_header(i);
}

static char* get_script_status_msg()
{
    return 
        script_state == SCRIPT_IDLE ? "idle" :
        script_state == SCRIPT_RUNNING ? (script_stop_requested ? "stopping" : "running") :
        script_state == SCRIPT_JUST_FINISHED ? "finished" : "err";
}

static MENU_UPDATE_FUNC(script_run_display)
{
    if (script_state)
    {
        MENU_SET_NAME(
            "Script %s...", get_script_status_msg()
        );
        MENU_SET_ICON(MNI_RECORD, 0);
    }
    else 
    {
        MENU_SET_NAME("Run script");
    }
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

static MENU_UPDATE_FUNC(script_print)
{
    if (!script_preview_flag || !entry->selected)
    {
        MENU_SET_NAME("Show script");
        MENU_SET_VALUE("");
        script_preview_flag = 0;
        return;
    }
    
    static int prev_script = -1;

    if (prev_script != script_selected)
    {
        int size;
        char* p = get_script_path(script_selected);
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
    }
    prev_script = script_selected;

    bmp_fill(40, 0, 0, 720, 430);
    int fnt = FONT(FONT_MED, COLOR_WHITE, 40);
    big_bmp_printf(fnt, 10, 10, "%s", script_preview);

    info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
}

/*static void script_select(void* priv, int delta)
{
    script_selected = MOD(script_selected + delta, script_cnt);
}*/

static void run_script(const char *script)
{
    extern int ml_started;
    while (!ml_started) msleep(100); // for startup scripts

    script_state = SCRIPT_RUNNING;

    msleep(500);
    
    console_clear();
    console_show();
    console_set_help_text("SET: show/hide");
    console_set_status_text("Script running...");
    
    int exit_code = tcc_compile_and_run(get_script_path(script_selected));
    if (exit_code == 0)
    {
        console_puts(    "Script finished.\n");
    }
    else
    {
        printf(  "Script error: %d.\n", exit_code);
        msleep(500);
        gui_stop_menu();
        msleep(500);
        console_show();
    }
    
    // restore some settings to normal, if script changed them
    //~ script_cleanup_af();
    bmp_draw_to_idle(0);
    canon_gui_enable_front_buffer(0);
    
    beep();
    script_state = SCRIPT_JUST_FINISHED;
    console_set_status_text("Script finished. ");
    msleep(500);
    redraw();
    msleep(500);
    for (int i = 0; i < 100; i++)
    {
        msleep(100);
        if (gui_menu_shown()) break;
        if (get_halfshutter_pressed()) break;
    }
    console_hide();
    script_state = SCRIPT_IDLE;
    console_set_status_text("");
}

static void script_run_fun(void* priv, int delta)
{
    if (script_state) 
    { 
        script_stop_requested = 1;
    }
    else
    {
        script_stop_requested = 0;
        gui_stop_menu();
        task_create("run_script", 0x1c, 0x4000, run_script, 0);
    }
}

/* end script functions */

/**
 * SET: show/hide console (while running, or 10 seconds after finished)
 * 
 */
int handle_picoc_keys(struct event * event)
{
    if (IS_FAKE(event)) return 1; // only process real buttons, not emulated presses

    if (gui_menu_shown()) return 1;
    
    if (script_state == SCRIPT_RUNNING)
    {
        //~ if (handle_picoc_lib_keys(event)==0) return 0;
    }
    
    extern int console_visible;

    if (script_state != SCRIPT_IDLE) // toggle show/hide
    {
        if (event->param == BGMT_PRESS_SET && display_idle())
        {
            console_toggle();
            return 0;
        }   
    }
    if (script_state == SCRIPT_JUST_FINISHED) // after script finished, hide the console on first key press
    {
        if (event->param == BGMT_UNPRESS_SET)
        {
            return 0;
        }   
        if (console_visible && 
            event->param != GMT_OLC_INFO_CHANGED &&
            #ifdef GMT_GUICMD_PRESS_BUTTON_SOMETHING
            event->param != GMT_GUICMD_PRESS_BUTTON_SOMETHING &&
            #endif
           1)
        {
            console_hide();
            return 1;
        }
    }
    return 1;
}

void script_open_submenu(void* priv, int delta)
{
    if (script_state == SCRIPT_IDLE)
    {
        // update the submenu structure on the fly
        static int prev_selected = -1;
        if (prev_selected != script_selected)
        {
            script_parse_header(script_selected);
        }
        prev_selected = script_selected;

        if (delta < 0) // user pressed PLAY
        {
            script_run_fun(priv, delta);
            return;
        }
   
        // now we can display it :)
        menu_open_submenu();
    }
    else
    {
        if (delta < 0) // user pressed PLAY
        {
            script_run_fun(priv, delta);
            return;
        }

        if (script_selected == (int)priv)
        {
            // display only the submenu for the running script, but not the others
            menu_open_submenu();
        }
    }
}

static MENU_UPDATE_FUNC(script_display)
{
    int script_displayed = (int) entry->priv;
    
    if (entry->selected && script_state == SCRIPT_IDLE) 
        script_selected = script_displayed; // change selected script as we scroll thru menu (if not running, of course)

    int displayed_script_is_idle = (script_state == SCRIPT_IDLE) || (script_selected != script_displayed);

    
    if (displayed_script_is_idle)
    {
        MENU_SET_RINFO(
            script_titles[script_displayed]
        );
        MENU_SET_ICON(MNI_SUBMENU, entry->selected && script_displayed == script_selected);
        MENU_SET_ENABLED(0);
        MENU_SET_VALUE("");
    }
    else
    {
        MENU_SET_ICON(MNI_ON, 0);
        MENU_SET_ENABLED(1);
        MENU_SET_VALUE("(%s)", get_script_status_msg());
    }
}

static struct menu_entry tccgui_submenu[] = {
        {
            .name = "Show script",
            .priv = &script_preview_flag,
            .max = 1,
            .icon_type = IT_ACTION,
            .update    = script_print,
            .help = "Display the contents of the selected script.",
        },
        {
            .name = "Run script",
            .update    = script_run_display,
            .select        = script_run_fun,
            .help = "Execute the selected script.",
        },
        {
            .help = "Script parameter #1",
        },
        {
            .help = "Script parameter #2",
        },
        {
            .help = "Script parameter #3",
        },
        {
            .help = "Script parameter #4",
        },
        {
            .help = "Script parameter #5",
        },
        {
            .help = "Script parameter #6",
        },
        {
            .help = "Script parameter #7",
        },
        {
            .help = "Script parameter #8",
        },
        {
            .help = "Script parameter #9",
        },
        MENU_EOL
};

#define MAX_PARAMS (COUNT(tccgui_submenu) - 3)

static struct menu_entry tccgui_menu[] = {
    /*
    {
        .name = "PicoC scripts...",
        .select = menu_open_submenu,
        .help = "Run small C-like scripts. http://code.google.com/p/picoc/",
        .children =  (struct menu_entry[]) {
            {
                .name = "Select script",
                .priv = &script_selected,
                .update    = script_select_display,
                .select        = script_select,
                .help = "Place your scripts under ML/SCRIPTS directory.",
            },
            {
                .name = "Show script",
                .priv = &script_preview_flag,
                .max = 1,
                .icon_type = IT_ACTION,
                .update    = script_print,
                .help = "Display the contents of the selected script.",
            },
            {
                .name = "Run script",
                .update    = script_run_display,
                .select        = script_run_fun,
                .help = "Execute the selected script.",
            },
            MENU_EOL,
        }
    },
    */

#define SCRIPT_ENTRY(i) \
        { \
            .name = script_list[i], \
            .priv = (void*)i, \
            .select = script_open_submenu, \
            .select_Q = script_open_submenu, \
            .update = script_display, \
            .icon_type = IT_SUBMENU, \
            .submenu_width = 700, \
            .children = tccgui_submenu, \
            .help  = "Run C scripts with TinyCC, http://bellard.org/tcc/.", \
            .help2 = "PLAY: quick start.", \
        },
    
    SCRIPT_ENTRY(0)
    SCRIPT_ENTRY(1)
    SCRIPT_ENTRY(2)
    SCRIPT_ENTRY(3)
    SCRIPT_ENTRY(4)
    SCRIPT_ENTRY(5)
    SCRIPT_ENTRY(6)
    SCRIPT_ENTRY(7)
    SCRIPT_ENTRY(8)
    SCRIPT_ENTRY(9)
    SCRIPT_ENTRY(10)
    SCRIPT_ENTRY(11)
    SCRIPT_ENTRY(12)
    SCRIPT_ENTRY(13)
    SCRIPT_ENTRY(14)
};

void script_setup_param(
    int param_index,  // 0-5
    char* param_name, // e.g. "Number of shots"
    int* param_value, // pointer to param value (priv)
    int min_value, 
    int max_value
    )
{
    if (param_index >= MAX_PARAMS) return;
    struct menu_entry * entry = &(tccgui_menu[script_selected].children[2 + param_index]);
    entry->name = param_name;
    entry->priv = param_value;
    entry->min = min_value;
    entry->max = max_value;
    entry->shidden = 0;
}

void script_reset_params()
{
    for (int i = 0; i < MAX_PARAMS; i++)
    {
        struct menu_entry * entry = &(tccgui_menu[script_selected].children[2 + i]);
        entry->name = 0;
        entry->priv = 0;
        entry->min = 0;
        entry->max = 0;
        entry->icon_type = 0;
        entry->shidden = 1;
    }
}

void script_setup_title(char* script_title)
{
    snprintf(script_titles[script_selected], SCRIPT_TITLE_SIZE, "%s", script_title);
}

// Queue for PicoC key handling

static struct msg_queue * script_key_mq = 0;

int script_key_dequeue()
{
    if (!script_key_mq) return -1;
    
    int msg;
    int err = msg_queue_receive(script_key_mq, (struct event**)&msg, 10);
    if (err) return -1;
    return msg;
}

void script_key_enqueue(int key)
{
    if (!script_key_mq) return;

    msg_queue_post(script_key_mq, key);
}

static void run_startup_script()
{
    for (int i = 0; i < script_cnt; i++)
    {
        if (streq(script_list[i], "AUTORUN.C"))
        {
            script_selected = i;
            script_parse_header(i);
            script_run_fun(0,0);
            return;
        }
    }
}

static void tccgui_init()
{
    script_key_mq = (struct msg_queue *) msg_queue_create("script_key", 10);
    find_scripts();
    run_startup_script();
    menu_add("Scripts", tccgui_menu, MIN(script_cnt, COUNT(tccgui_menu)));
}

INIT_FUNC(__FILE__, tccgui_init);
