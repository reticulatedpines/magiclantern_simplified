#define CONFIG_CONSOLE

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

#define MAX_PATH_LEN 0x80
char gPath[MAX_PATH_LEN];

struct file_entry
{
    struct file_entry * next;
    struct menu_entry menu_entry;
    char name[MAX_PATH_LEN];
    unsigned int size;
    unsigned int is_dir: 1;
    unsigned int added: 1;
};

static struct file_entry * file_entries = 0;

static MENU_SELECT_FUNC(selectdir);
static MENU_UPDATE_FUNC(updatedir);
static MENU_SELECT_FUNC(selectfile);
static MENU_UPDATE_FUNC(updatefile);

static struct menu_entry fileman_menu[] =
{
    {
        .name = "File Browser",
        .select = menu_open_submenu,
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            MENU_EOL,
        }
    }
};

static void clear_file_menu()
{
    while (file_entries)
    {
        struct file_entry * next = file_entries->next;
        menu_remove("File Browser", &(file_entries->menu_entry), 1);
        console_printf("%s\n", file_entries->name);
        FreeMemory(file_entries);
        file_entries = next;
    }
}

static void add_file_entry(char* txt, int size)
{
    struct file_entry * fe = AllocateMemory(sizeof(struct file_entry));
    if (!fe) return;
    memset(fe, 0, sizeof(struct file_entry));
    snprintf(fe->name, sizeof(fe->name), "%s", txt);
    fe->size = size;
    
    fe->menu_entry.name = fe->name;
    fe->menu_entry.priv = fe;
    
    int dir = txt[strlen(txt)-1] == '/';
    fe->is_dir = dir;
    if (fe->is_dir)
    {
        fe->menu_entry.select = selectdir;
        fe->menu_entry.update = updatedir;
    }
    else
    {
        fe->menu_entry.select = selectfile;
        fe->menu_entry.select_Q = selectfile;
        fe->menu_entry.update = updatefile;
    }
    fe->next = file_entries;
    file_entries = fe;
}

static void build_file_menu(struct file_entry * fe)
{
    /* HaCKeD Sort */
    struct file_entry * fe0 = fe;
    int done = 0;
    while (!done)
    {
        done = 1;
        
        for (struct file_entry * fe = fe0; fe; fe = fe->next)
        {
            if (!fe->added)
            {
                /* are there any entries that should be before "fe" ? */
                /* if yes, skip "fe", add those entries, and try again */
                int should_skip = 0;
                for (struct file_entry * e = fe0; e; e = e->next)
                {
                    if (!e->added && e != fe)
                    {
                        if (e->is_dir && !fe->is_dir) { should_skip = 1; break; }
                        if ((e->is_dir == fe->is_dir) && strcmp(e->name, fe->name) < 0) { should_skip = 1; break; }
                    }
                }
                
                if (!should_skip)
                {
                    menu_add("File Browser", &(fe->menu_entry), 1);
                    fe->added = 1;
                }
                else done = 0;
            }
        }
    }
}

static void ScanDir(char *path)
{
    clear_file_menu();
    add_file_entry("../", 0);
    
    struct fio_file file;
    struct fio_dirent * dirent = 0;

    dirent = FIO_FindFirstEx( path, &file );
    if( IS_ERROR(dirent) ) return;
        
    do
    {
        if (file.name[0] == '.') continue;
        if (file.mode & ATTR_DIRECTORY)
        {
            int len = strlen(file.name);
            snprintf(file.name + len, sizeof(file.name) - len, "/");
            add_file_entry(file.name, 0);
        }
        else
        {
            add_file_entry(file.name, file.size);
        }
    }
    while( FIO_FindNextEx( dirent, &file ) == 0);
    
    build_file_menu(file_entries);
    
    FIO_CleanupAfterFindNext_maybe(dirent);
}

static void Browse(char* path)
{
    snprintf(gPath, sizeof(gPath), path);
    ScanDir(gPath);
}

static void BrowseDown(char* path)
{
    STR_APPEND(gPath, "%s", path);
    ScanDir(gPath);
}

static void BrowseUp(char* path)
{
    char* p = gPath + strlen(gPath) - 2;
    while (p > gPath && *p != '/') p--;
    if (*p == '/')
    {
        char old_dir[MAX_PATH_LEN];
        snprintf(old_dir, sizeof(old_dir), p+1);
        *(p+1) = 0;
        ScanDir(gPath);
        
        /* move menu selection back to the old dir */
        for (struct file_entry * fe = file_entries; fe; fe = fe->next)
        {
            if (streq(fe->name, old_dir))
            {
                fe->menu_entry.selected = 1;
                for (struct file_entry * e = file_entries; e; e = e->next)
                    if (e != fe) e->menu_entry.selected = 0;
                break;
            }
        }
    }
    else
    {
        menu_close_submenu();
    }
}

static MENU_SELECT_FUNC(selectdir)
{
    struct file_entry * fe = (struct file_entry *) priv;
    char* name = (char*) fe->name;
    if ((name[0] == '.' &&
         name[1] == '.' &&
         name[2] == '/')
         || (delta < 0)
        )
    {
       BrowseUp(name);
    }
    else
    {
       BrowseDown(name);
    }
}

static MENU_SELECT_FUNC(selectfile)
{
    beep();
}

static MENU_UPDATE_FUNC(updatedir)
{
    MENU_SET_VALUE("");
    MENU_SET_ICON(MNI_AUTO, 0);
    MENU_SET_HELP(gPath);
}

const char * format_size( unsigned size)
{
    static char str[ 32 ];

    if( size > 1024*1024*1024 )
    {
        int size_gb = (size/1024 + 5 * 10) / 1024 / 1024;
        snprintf( str, sizeof(str), "%d.%dGB", size_gb/10, size_gb%10);
    }
    else if( size > 1024*1024 )
    {
        int size_mb = (size/1024 + 5) * 10 / 1024;
        snprintf( str, sizeof(str), "%d.%dMB", size_mb/10, size_mb%10);
    }
    else if( size > 1024 )
    {
        int size_kb = (size/1024 + 5) * 10;
        snprintf( str, sizeof(str), "%d.%dkB", size_kb/10, size_kb%10);
    }
    else
    {
        snprintf( str, sizeof(str), "%db", size);
    }

    return str;
}

static MENU_UPDATE_FUNC(updatefile)
{
    struct file_entry * fe = (struct file_entry *) entry->priv;
    MENU_SET_VALUE("");
    
    MENU_SET_RINFO("%s", format_size(fe->size));
    MENU_SET_ICON(MNI_OFF, 0);
    MENU_SET_HELP(gPath);
}

static int InitRootDir()
{
    int cf_present = is_dir("A:/");
    int sd_present = is_dir("B:/");

    if (cf_present && !sd_present)
    {
        Browse("A:/");
    }
    else if (sd_present && !cf_present)
    {
        Browse("B:/");
    }
    else if (sd_present && cf_present)
    {
        add_file_entry("A:/", 0);
        add_file_entry("B:/", 0);
    }
    else return -1;
    
    return 0;
}

unsigned int fileman_init()
{
    menu_add("Debug", fileman_menu, COUNT(fileman_menu));
    InitRootDir();
    return 0;
}

unsigned int fileman_deinit()
{
    return 0;
}


MODULE_INFO_START()
    MODULE_INIT(fileman_init)
    MODULE_DEINIT(fileman_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Author", "ML dev.")
    MODULE_STRING("License", "GPL")
    MODULE_STRING("Description", "File browser")
MODULE_STRINGS_END()
