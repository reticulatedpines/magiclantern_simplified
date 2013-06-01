#define CONFIG_CONSOLE

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>

#define MAX_PATH_LEN 0x80
static char gPath[MAX_PATH_LEN];
static char gSrcFile[MAX_PATH_LEN];
static unsigned int op_mode;

static int cf_present;
static int sd_present;

struct file_entry
{
    struct file_entry * next;
    struct menu_entry menu_entry;
    char name[MAX_PATH_LEN];
    unsigned int size;
    unsigned int type: 2;
    unsigned int added: 1;
};

#define TYPE_DIR 0
#define TYPE_FILE 1
#define TYPE_ACTION 2

enum _FILER_OP {
    FILE_OP_NONE,
    FILE_OP_COPY,
    FILE_OP_MOVE,
    FILE_OP_PREVIEW
};

static struct file_entry * file_entries = 0;

/* view file mode */
static int view_file = 0;

static MENU_SELECT_FUNC(select_dir);
static MENU_UPDATE_FUNC(update_dir);
static MENU_SELECT_FUNC(select_file);
static MENU_UPDATE_FUNC(update_file);
static MENU_SELECT_FUNC(default_select_action);
static MENU_UPDATE_FUNC(update_action);
static MENU_SELECT_FUNC(BrowseUpMenu);

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

static struct file_entry * add_file_entry(char* txt, int type, int size)
{
    struct file_entry * fe = AllocateMemory(sizeof(struct file_entry));
    if (!fe) return 0;
    memset(fe, 0, sizeof(struct file_entry));
    snprintf(fe->name, sizeof(fe->name), "%s", txt);
    fe->size = size;

    fe->menu_entry.name = fe->name;
    fe->menu_entry.priv = fe;

    fe->type = type;
    fe->menu_entry.select_Q = BrowseUpMenu;
    if (fe->type == TYPE_DIR)
    {
        fe->menu_entry.select = select_dir;
        fe->menu_entry.update = update_dir;
    }
    else if (fe->type == TYPE_FILE)
    {
        fe->menu_entry.select = select_file;
        fe->menu_entry.update = update_file;
    }
    else if (fe->type == TYPE_ACTION)
    {
        fe->menu_entry.select = default_select_action;
        fe->menu_entry.update = update_action;
        fe->menu_entry.icon_type = IT_ACTION;
    }
    fe->next = file_entries;
    file_entries = fe;
    return fe;
}

static void build_file_menu()
{
    /* HaCKeD Sort */
    int done = 0;
    while (!done)
    {
        done = 1;

        for (struct file_entry * fe = file_entries; fe; fe = fe->next)
        {
            if (!fe->added)
            {
                /* are there any entries that should be before "fe" ? */
                /* if yes, skip "fe", add those entries, and try again */
                int should_skip = 0;
                for (struct file_entry * e = file_entries; e; e = e->next)
                {
                    if (!e->added && e != fe)
                    {
                        if (e->type < fe->type) { should_skip = 1; break; }
                        if ((e->type == fe->type) && strcmp(e->name, fe->name) < 0) { should_skip = 1; break; }
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

    if (strlen(path) == 0)
    {
        add_file_entry("A:/", TYPE_DIR, 0);
        add_file_entry("B:/", TYPE_DIR, 0);
        build_file_menu();
        return;
    }

    if(op_mode != FILE_OP_NONE)
    {
        console_printf("ScanDir\n");
        char srcpath[MAX_PATH_LEN];
        strcpy(srcpath,gSrcFile);
        char *p = srcpath+strlen(srcpath);
        while (p > srcpath && *p != '/') p--;
        *(p+1) = 0;

        console_printf("src: %s\n",srcpath);
        console_printf("dst: %s\n",path);


        if(strcmp(path,srcpath) != 0){
            add_file_entry("***Select Here***", TYPE_DIR, 0);
            add_file_entry("*** Cancel OP ***", TYPE_DIR, 0);
        }
    }


    add_file_entry("../", TYPE_DIR, 0);

    struct fio_file file;
    struct fio_dirent * dirent = 0;

    dirent = FIO_FindFirstEx( path, &file );
    if( IS_ERROR(dirent) )
    {
        build_file_menu();
        return;
    }

    do
    {
        if (file.name[0] == '.') continue;
        if (file.mode & ATTR_DIRECTORY)
        {
            int len = strlen(file.name);
            snprintf(file.name + len, sizeof(file.name) - len, "/");
            add_file_entry(file.name, TYPE_DIR, 0);
        }
        else
        {
            add_file_entry(file.name, TYPE_FILE, file.size);
        }
    }
    while( FIO_FindNextEx( dirent, &file ) == 0);

    build_file_menu();

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

static void restore_menu_selection(char* old_dir)
{
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

static void BrowseUp()
{
    char* p = gPath + strlen(gPath) - 2;
    while (p > gPath && *p != '/') p--;

    if (*p == '/') /* up one level */
    {
        char old_dir[MAX_PATH_LEN];
        snprintf(old_dir, sizeof(old_dir), p+1);
        *(p+1) = 0;
        ScanDir(gPath);
        restore_menu_selection(old_dir);
    }
    else if (cf_present && sd_present && strlen(gPath) > 0) /* two cards: show A:/ and B:/ in menu */
    {
        char old_dir[MAX_PATH_LEN];
        snprintf(old_dir, sizeof(old_dir), "%s", gPath);
        gPath[0] = 0;
        ScanDir("");
        restore_menu_selection(old_dir);
    }
    else /* already at the top, close the file browser */
    {
        menu_close_submenu();
    }
}

static int
ML_FIO_CopyFile(char *src,char *dst){
    const int bufsize = 128*1024;
    void* buf = alloc_dma_memory(bufsize);
    if (!buf) return 1;

    FILE* f = FIO_Open(src, O_RDONLY | O_SYNC);
    if (f == INVALID_PTR) return 1;

    FILE* g = FIO_CreateFileEx(dst);
    if (g == INVALID_PTR) { FIO_CloseFile(f); return 1; }

    int r = 0;
    while ((r = FIO_ReadFile(f, buf, bufsize)))
        FIO_WriteFile(g, buf, r);

    FIO_CloseFile(f);
    FIO_CloseFile(g);
    msleep(1000); // this decreases the chances of getting corrupted files (fig    ure out why!)
    free_dma_memory(buf);
    return 0;
}

static int
ML_FIO_MoveFile(char *src,char *dst){

    ML_FIO_CopyFile(src,dst);
    FIO_RemoveFile(src);
    return 0;
}

static void
FileCopy(void *unused)
{
    char fname[MAX_PATH_LEN],tmpdst[MAX_PATH_LEN];
    strcpy(tmpdst,gPath);
    size_t totallen = strlen(gSrcFile);
    char *p = gSrcFile + totallen;
    while (p > gSrcFile && *p != '/') p--;
    strcpy(fname,p+1);

    char dstfile[MAX_PATH_LEN];
    snprintf(dstfile,MAX_PATH_LEN,"%s%s",gPath,fname);

    ML_FIO_CopyFile(gSrcFile,dstfile);

    if(!strcmp(gPath,tmpdst)) ScanDir(gPath);
}

static void
FileMove(void *unused)
{
    char fname[MAX_PATH_LEN],tmpdst[MAX_PATH_LEN];
    strcpy(tmpdst,gPath);
    size_t totallen = strlen(gSrcFile);
    char *p = gSrcFile + totallen;
    while (p > gSrcFile && *p != '/') p--;
    strcpy(fname,p+1);

    char dstfile[MAX_PATH_LEN];
    snprintf(dstfile,MAX_PATH_LEN,"%s%s",gPath,fname);

    console_printf("Move\n");
    console_printf("src: %s\n",gSrcFile);
    console_printf("dst: %s\n",dstfile);
    ML_FIO_MoveFile(gSrcFile,dstfile);

    if(!strcmp(gPath,tmpdst)) ScanDir(gPath);
}


static void FileOperation(){

    switch(op_mode){
    case FILE_OP_COPY:
        task_create("FileCopy_task", 0x1b, 0x4000, FileCopy, 0);
        break;
    case FILE_OP_MOVE:
        task_create("FileMove_task", 0x1b, 0x4000, FileMove, 0);
        break;
    case FILE_OP_PREVIEW:
        break;
    }
    //cleanup
    op_mode = FILE_OP_NONE;

    ScanDir(gPath);
}

static void FileOpCancel(){
    gSrcFile[0] = 0;
    op_mode = FILE_OP_NONE;
    ScanDir(gPath);
}

static MENU_SELECT_FUNC(BrowseUpMenu)
{
    BrowseUp();
}

static MENU_SELECT_FUNC(select_dir)
{
    struct file_entry * fe = (struct file_entry *) priv;
    char* name = (char*) fe->name;
    if(!strcmp(name,"***Select Here***")){
        FileOperation();
    }else if(!strcmp(name,"*** Cancel OP ***")){
        FileOpCancel();
    }else if (!strcmp(name,"../") || (delta < 0))
        {
            BrowseUp();
        }
    else
        {
            BrowseDown(name);
        }
}

static MENU_UPDATE_FUNC(update_dir)
{
    MENU_SET_VALUE("");
    MENU_SET_ICON(MNI_AUTO, 0);
    MENU_SET_HELP(gPath);
    if (entry->selected) view_file = 0;
}

const char * format_size( unsigned size)
{
    static char str[ 32 ];

    if( size > 1024*1024*1024 )
    {
        int size_gb = (size/1024 + 5) * 10 / 1024 / 1024;
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

static MENU_SELECT_FUNC(CopyFile)
{
    strcpy(gSrcFile,gPath);
    console_printf("Copysrc: %s\n",gSrcFile);
    op_mode = FILE_OP_COPY;
}

static MENU_UPDATE_FUNC(CopyFileProgress)
{

}

static MENU_SELECT_FUNC(MoveFile)
{
    strcpy(gSrcFile,gPath);
    console_printf("Movesrc: %s\n",gSrcFile);
    op_mode = FILE_OP_MOVE;
}

static MENU_UPDATE_FUNC(MoveFileProgress)
{

}

static MENU_SELECT_FUNC(viewfile_toggle)
{
    view_file = !view_file;
}

static MENU_UPDATE_FUNC(viewfile_show)
{
    if (view_file)
    {
        static char buf[1025];
        FILE * file = FIO_Open( gPath, O_RDONLY | O_SYNC );
        if (file != INVALID_PTR)
        {
            int r = FIO_ReadFile(file, buf, sizeof(buf)-1);
            FIO_CloseFile(file);
            buf[r] = 0;
            info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
            clrscr();
            big_bmp_printf(FONT_MED, 0, 0, "%s", buf);
        }
        else
        {
            MENU_SET_WARNING(MENU_WARN_ADVICE, "Error reading %s", gPath);
            view_file = 0;
        }
    }
    else
    {
        update_action(entry, info);
    }
}

static int delete_confirm_flag = 0;

static MENU_SELECT_FUNC(delete_file)
{
    if (streq(gPath+1, ":/AUTOEXEC.BIN"))
    {
        beep();
        return;
    }

    if (!delete_confirm_flag)
    {
        delete_confirm_flag = get_ms_clock_value();
        beep();
    }
    else
    {
        delete_confirm_flag = 0;
        FIO_RemoveFile(gPath);
        BrowseUp();
    }
}

static MENU_UPDATE_FUNC(delete_confirm)
{
    update_action(entry, info);

    /* delete confirmation timeout after 2 seconds */
    if (get_ms_clock_value() > delete_confirm_flag + 2000)
        delete_confirm_flag = 0;

    /* no question mark in in our font, fsck! */
    if (delete_confirm_flag)
        MENU_SET_RINFO("Press SET to confirm");
}

static MENU_SELECT_FUNC(select_file)
{
    struct file_entry * fe = (struct file_entry *) priv;

    /* fe will be freed in clear_file_menu; backup things that we are going to reuse */
    char name[MAX_PATH_LEN];
    snprintf(name, sizeof(name), "%s", fe->name);
    int size = fe->size;
    STR_APPEND(gPath, "%s", name);

    clear_file_menu();
    /* at this point, fe was freed and is no longer valid */
    fe = 0;

    struct file_entry * e = add_file_entry(name, TYPE_FILE, size);
    if (!e) return;
    e->menu_entry.select = BrowseUpMenu;
    e->menu_entry.select_Q = BrowseUpMenu;
    e->menu_entry.priv = e;

    e = add_file_entry("Copy", TYPE_ACTION, 0);
    e->menu_entry.select = CopyFile;
    e->menu_entry.update = CopyFileProgress;

    e = add_file_entry("Move", TYPE_ACTION, 0);
    e->menu_entry.select = MoveFile;
    e->menu_entry.update = MoveFileProgress;

    e = add_file_entry("View", TYPE_ACTION, 0);
    e->menu_entry.select = viewfile_toggle;
    e->menu_entry.update = viewfile_show;

    e = add_file_entry("Delete", TYPE_ACTION, 0);
    e->menu_entry.select = delete_file;
    e->menu_entry.update = delete_confirm;

    //~ e = add_file_entry("Copy", TYPE_ACTION, 0);
    //~ e = add_file_entry("Rename", TYPE_ACTION, 0);

    build_file_menu();
}

static MENU_UPDATE_FUNC(update_file)
{
    struct file_entry * fe = (struct file_entry *) entry->priv;
    MENU_SET_VALUE("");

    MENU_SET_RINFO("%s", format_size(fe->size));
    MENU_SET_ICON(MNI_OFF, 0);
    MENU_SET_HELP(gPath);
    if (entry->selected) view_file = 0;
}

static MENU_SELECT_FUNC(default_select_action)
{
    beep();
}

static MENU_UPDATE_FUNC(update_action)
{
    MENU_SET_VALUE("");
    MENU_SET_HELP(gPath);
    if (entry->selected) view_file = 0;
}

static int InitRootDir()
{
    cf_present = is_dir("A:/");
    sd_present = is_dir("B:/");

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
        Browse("");
    }
    else return -1;

    return 0;
}

unsigned int fileman_init()
{
    menu_add("Debug", fileman_menu, COUNT(fileman_menu));
    op_mode = FILE_OP_NONE;
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
