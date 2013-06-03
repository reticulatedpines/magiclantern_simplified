#define CONFIG_CONSOLE
#define _file_man_c_

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include "file_man.h"

#define MAX_PATH_LEN 0x80
static char gPath[MAX_PATH_LEN];
static char gStatusMsg[60];
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

typedef struct _multi_files
{
    struct _multi_files *next;
    char name[MAX_PATH_LEN];
}FILES_LIST;

static FILES_LIST *mfile_root;


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
static MENU_UPDATE_FUNC(update_status);
static MENU_SELECT_FUNC(BrowseUpMenu);
static MENU_SELECT_FUNC(FileCopyStart);
static MENU_SELECT_FUNC(FileMoveStart);
static MENU_SELECT_FUNC(FileOpCancel);
static unsigned int mfile_add_tail();
static unsigned int mfile_clean_all();
static int mfile_is_regged(char *fname);

#define MAX_FILETYPE_HANDLERS 32

struct filetype_handler
{
    char *extension;
    char *type;
    filetype_handler_func handler;
};

int fileman_filetype_registered = 0;
struct filetype_handler fileman_filetypes[MAX_FILETYPE_HANDLERS];

/* this function has to be public so that other modules can register file types for viewing this file */
unsigned int fileman_register_type(char *ext, char *type, filetype_handler_func handler)
{
    if(fileman_filetype_registered < MAX_FILETYPE_HANDLERS)
    {
        fileman_filetypes[fileman_filetype_registered].extension = ext;
        fileman_filetypes[fileman_filetype_registered].type = type;
        fileman_filetypes[fileman_filetype_registered].handler = handler;
        fileman_filetype_registered++;
    }
    return 0;
}

struct filetype_handler *fileman_find_filetype(char *extension)
{
    for(int pos = 0; pos < fileman_filetype_registered; pos++)
    {
        if(!strcmp(extension, fileman_filetypes[pos].extension))
        {
            return &fileman_filetypes[pos];
        }
    }
    
    return NULL;
}

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
                    if (!e->added && e != fe && fe->type != TYPE_ACTION && e->type != TYPE_ACTION)
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

static struct semaphore * scandir_sem = 0;

/* this is called from file copy/move tasks as well as from GUI task, so it needs to be thread safe */
static void ScanDir(char *path)
{
    take_semaphore(scandir_sem, 0);

    clear_file_menu();

    if (strlen(path) == 0)
    {
        add_file_entry("A:/", TYPE_DIR, 0);
        add_file_entry("B:/", TYPE_DIR, 0);
        build_file_menu();
        give_semaphore(scandir_sem);
        return;
    }

    struct fio_file file;
    struct fio_dirent * dirent = 0;

    dirent = FIO_FindFirstEx( path, &file );
    if( IS_ERROR(dirent) )
    {
        add_file_entry("../", TYPE_DIR, 0);
        build_file_menu();
        give_semaphore(scandir_sem);
        return;
    }

    int n = 0;
    do
    {
        if (file.name[0] == '.') continue;
        n++;
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

    if (!n)
    {
        /* nothing here, add this so menu won't crash */
        add_file_entry("../", TYPE_DIR, 0);
    }

    if(op_mode != FILE_OP_NONE)
    {
        /*        char srcpath[MAX_PATH_LEN];
        strcpy(srcpath,gSrcFile);
        char *p = srcpath+strlen(srcpath);
        while (p > srcpath && *p != '/') p--;
        *(p+1) = 0;

        console_printf("src: %s\n",srcpath);
        console_printf("dst: %s\n",path);

        if(strcmp(path,srcpath) != 0)
        {
        */
            struct file_entry * e;
            
            /* need to add these in reverse order */

            e = add_file_entry("*** Cancel OP ***", TYPE_ACTION, 0);
            if (e) e->menu_entry.select = FileOpCancel;

            switch (op_mode)
            {
                case FILE_OP_COPY:
                    e = add_file_entry("*** Copy Here ***", TYPE_ACTION, 0);
                    if (e) e->menu_entry.select = FileCopyStart;
                    break;
                case FILE_OP_MOVE:
                    e = add_file_entry("*** Move Here ***", TYPE_ACTION, 0);
                    if (e) e->menu_entry.select = FileMoveStart;
                    break;
            }
            //        }
    }

    build_file_menu();

    FIO_CleanupAfterFindNext_maybe(dirent);
    give_semaphore(scandir_sem);
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

    int err = 0;
    int r = 0;
    while ((r = FIO_ReadFile(f, buf, bufsize)))
    {
        int w = FIO_WriteFile(g, buf, r);
        if (w != r)
        {
            /* copy failed; abort and delete the incomplete file */
            err = 1;
            break;
        }
    }

    FIO_CloseFile(f);
    FIO_CloseFile(g);
    msleep(1000); // this decreases the chances of getting corrupted files (fig    ure out why!)
    free_dma_memory(buf);
    
    if (err)
    {
        FIO_RemoveFile(dst);
        return 1;
    }
    
    /* all OK */
    return 0;
}

static int
ML_FIO_MoveFile(char *src,char *dst){

    int err = ML_FIO_CopyFile(src,dst);
    if (!err)
    {
        /* file copied, we can remove the old one */
        FIO_RemoveFile(src);
        return 0;
    }
    else
    {
        /* something went wrong; keep the old file and return error code */
        return err;
    }
}

static void
FileCopy(void *unused)
{
    char fname[MAX_PATH_LEN],tmpdst[MAX_PATH_LEN];
    char dstfile[MAX_PATH_LEN];
    size_t totallen = 0;
    FILES_LIST *tmpmf = mfile_root;
    strcpy(tmpdst,gPath);

    while(tmpmf->next){
        tmpmf = tmpmf->next;
        dstfile[0] = 0;
        fname[0] = 0;
        totallen = strlen(tmpmf->name);
        char *p = tmpmf->name + totallen;
        while (p > tmpmf->name && *p != '/') p--;
        strcpy(fname,p+1);
        
        snprintf(dstfile,MAX_PATH_LEN,"%s%s",gPath,fname);
        
        snprintf(gStatusMsg, sizeof(gStatusMsg), "Copying %s to %s...", tmpmf->name, gPath);
        int err = ML_FIO_CopyFile(tmpmf->name,dstfile);
        if (err) snprintf(gStatusMsg, sizeof(gStatusMsg), "Copy error (%d)", err);
        else gStatusMsg[0] = 0;
    }

    mfile_clean_all();
    if(!strcmp(gPath,tmpdst)) ScanDir(gPath);
}

static void
FileMove(void *unused)
{
    char fname[MAX_PATH_LEN],tmpdst[MAX_PATH_LEN];
    char dstfile[MAX_PATH_LEN];
    size_t totallen = 0;
    FILES_LIST *tmpmf = mfile_root;
    strcpy(tmpdst,gPath);

    while(tmpmf->next){
        tmpmf = tmpmf->next;
        dstfile[0] = 0;
        fname[0] = 0;
        totallen = strlen(tmpmf->name);
        char *p = tmpmf->name + totallen;
        while (p > tmpmf->name && *p != '/') p--;
        strcpy(fname,p+1);
        
        snprintf(dstfile,MAX_PATH_LEN,"%s%s",gPath,fname);
        
        snprintf(gStatusMsg, sizeof(gStatusMsg), "Moving %s to %s...", tmpmf->name, gPath);
        int err = ML_FIO_CopyFile(tmpmf->name,dstfile);
        if (err) snprintf(gStatusMsg, sizeof(gStatusMsg), "Move error (%d)", err);
        else gStatusMsg[0] = 0;
    }

    mfile_clean_all();
    if(!strcmp(gPath,tmpdst)) ScanDir(gPath);
}


static MENU_SELECT_FUNC(FileCopyStart)
{
    task_create("filecopy_task", 0x1b, 0x4000, FileCopy, 0);
    op_mode = FILE_OP_NONE;
    ScanDir(gPath);
}

static MENU_SELECT_FUNC(FileMoveStart)
{
    task_create("filemove_task", 0x1b, 0x4000, FileMove, 0);
    op_mode = FILE_OP_NONE;
    ScanDir(gPath);
}

static MENU_SELECT_FUNC(FileOpCancel)
{
    mfile_clean_all();
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
    if (!strcmp(name,"../") || (delta < 0))
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
    update_status(entry, info);
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
    console_printf("CopyFile()\n");
    if(mfile_is_regged("")==0){
        console_printf("addreg: %s\n",gPath);
        mfile_add_tail();
    }
    op_mode = FILE_OP_COPY;
    BrowseUp();
}

static MENU_UPDATE_FUNC(CopyFileProgress)
{

}

static MENU_SELECT_FUNC(MoveFile)
{
    if(mfile_is_regged("")==0){
        mfile_add_tail();
    }
    op_mode = FILE_OP_MOVE;

    BrowseUp();

}

static MENU_UPDATE_FUNC(MoveFileProgress)
{

}

static char *fileman_get_extension(char *filename)
{
    int pos = strlen(filename) - 1;
    while(pos > 0 && filename[pos] != '.')
    {
        pos--;
    }
    
    /* does the file have an extension */
    if(pos > 0)
    {
        return &filename[pos + 1];
    }
    
    return NULL;
}

FILETYPE_HANDLER(text_handler)
{
    if (cmd != FILEMAN_CMD_VIEW_IN_MENU)
        return 0; /* this handler only knows to show things in menu */
    
    char* buf = alloc_dma_memory(1025);
    if (!buf) return 0;
    
    FILE * file = FIO_Open( filename, O_RDONLY | O_SYNC );
    if (file != INVALID_PTR)
    {
        int r = FIO_ReadFile(file, buf, 1024);
        FIO_CloseFile(file);
        buf[r] = 0;
        for (int i = 0; i < r; i++)
            if (buf[i] == 0) buf[i] = ' ';
        big_bmp_printf(FONT_MED, 0, 0, "%s", buf);
        free_dma_memory(buf);
        return 1;
    }
    else
    {
        free_dma_memory(buf);
        return 0;
    }
}

static MENU_SELECT_FUNC(viewfile_toggle)
{
    char *ext = fileman_get_extension(gPath);
    
    if(ext)
    {
        struct filetype_handler *filetype = fileman_find_filetype(ext);
        if(filetype)
        {
            int status = filetype->handler(FILEMAN_CMD_VIEW_OUTSIDE_MENU, gPath, NULL);
            if (status > 0)
            {
                /* file is being viewed outside menu */
                return;
            }
            else if (status < 0)
            {
                /* error */
                beep();
            }
            /* else, we should display the file without leaving the menu */
        }
    }
    
    view_file = !view_file;
    BrowseUp();
}

static MENU_UPDATE_FUNC(viewfile_update)
{
    char *ext = fileman_get_extension(gPath);
    struct filetype_handler *filetype = NULL;
    if (ext) filetype = fileman_find_filetype(ext);
    if(filetype) MENU_SET_RINFO("Type: %s", filetype->type);
    update_action(entry, info);
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

static int 
mfile_is_regged(char *fname)
{
    char tmpfname[MAX_PATH_LEN];
    strcpy(tmpfname,gPath);
    if(strlen(fname))
        STR_APPEND(tmpfname,fname);

    FILES_LIST *tmpmf = mfile_root;
    while(tmpmf->next)
    {
        tmpmf = tmpmf->next;
        if(!strcmp(tmpmf->name,tmpfname))
        { //match
            return 1;
        }
    }
    return 0;
}

static unsigned int 
mfile_find_remove()
{
    FILES_LIST *prevmf;
    FILES_LIST *tmpmf = mfile_root;
    while(tmpmf->next)
    {
        prevmf = tmpmf;
        tmpmf = tmpmf->next;
        if(!strcmp(tmpmf->name,gPath))
        { //match
            prevmf->next = tmpmf->next;
            FreeMemory((void *)tmpmf);
            return 1;
        }
    }
    return 0;
}

static unsigned int 
mfile_add_tail()
{
    console_printf("mf_add %s\n",gPath);
    FILES_LIST *newmf;
    FILES_LIST *tmpmf = mfile_root;
    while(tmpmf->next)
        tmpmf = tmpmf->next;

    newmf = AllocateMemory(sizeof(FILES_LIST));
    memset(newmf,0,sizeof(FILES_LIST));
    strcpy(newmf->name, gPath);
    newmf->next = NULL;
    tmpmf->next = newmf;

    return 0;
}

static unsigned int 
mfile_clean_all()
{
    FILES_LIST *prevmf;
    FILES_LIST *tmpmf = mfile_root;
    while(tmpmf->next)
    {
        prevmf = tmpmf;
        tmpmf = tmpmf->next;
        prevmf->next = tmpmf->next;
        FreeMemory((void *)tmpmf);
        tmpmf = prevmf;
    }
    return 0;
}

static MENU_SELECT_FUNC(mfile_clear_all_selected_menu)
{
    mfile_clean_all();
    BrowseUp();
}

static MENU_SELECT_FUNC(select_multi)
{
    console_printf("select_multi %s\n",gPath);
    //Find already registerd entry (toggle reg/rem)
    if(mfile_find_remove() == 0)
        mfile_add_tail();

    if(mfile_root->next == NULL) op_mode = FILE_OP_NONE;

    BrowseUp();
}

static MENU_SELECT_FUNC(select_file)
{
    if (view_file) { view_file = 0; return; }

    struct file_entry * fe = (struct file_entry *) priv;

    /* fe will be freed in clear_file_menu; backup things that we are going to reuse */
    char name[MAX_PATH_LEN];
    snprintf(name, sizeof(name), "%s", fe->name);
    int size = fe->size;
    STR_APPEND(gPath, "%s", name);

    clear_file_menu();
    /* at this point, fe was freed and is no longer valid */
    fe = 0;

    struct file_entry * e;
    
    /* note: need to add these in reverse order */
    e = add_file_entry("Clear all selected files", TYPE_ACTION, 0);
    if (e)
    {
        e->menu_entry.select = mfile_clear_all_selected_menu;
    }

    e = add_file_entry("Delete", TYPE_ACTION, 0);
    if (e)
    {
        e->menu_entry.select = delete_file;
        e->menu_entry.update = delete_confirm;
    }

    e = add_file_entry("Move", TYPE_ACTION, 0);
    if (e)
    {
        e->menu_entry.select = MoveFile;
        //e->menu_entry.update = MoveFileProgress;
    }

    e = add_file_entry("Copy", TYPE_ACTION, 0);
    if (e)
    {
        e->menu_entry.select = CopyFile;
        //e->menu_entry.update = CopyFileProgress;
    }

    e = add_file_entry("View", TYPE_ACTION, 0);
    if (e)
    {
        e->menu_entry.select = viewfile_toggle;
        e->menu_entry.update = viewfile_update;
    }

    e = add_file_entry(name, TYPE_FILE, size);
    if (e)
    {
        e->menu_entry.select = select_multi;
        e->menu_entry.select_Q = BrowseUpMenu;
        e->menu_entry.priv = e;
    }

    build_file_menu();
}

static MENU_UPDATE_FUNC(update_status)
{
    MENU_SET_HELP(gPath);
    if (op_mode != FILE_OP_NONE)
    {
        FILES_LIST *tmpmf = mfile_root;
        int i = 0;
        while(tmpmf->next)
        {
            tmpmf = tmpmf->next;
            i++;
        }
        if(i == 1)
        {
            MENU_SET_WARNING(MENU_WARN_INFO, "%s %s", op_mode == FILE_OP_COPY ? "Copy" : "Move", tmpmf->name);
        }else
        {
            MENU_SET_WARNING(MENU_WARN_INFO, "%s MultipleFiles", op_mode == FILE_OP_COPY ? "Copy" : "Move");
        }            
    }else if (gStatusMsg[0])
        MENU_SET_WARNING(MENU_WARN_INFO, "%s", gStatusMsg);
}

static MENU_UPDATE_FUNC(update_file)
{
    struct file_entry * fe = (struct file_entry *) entry->priv;
    MENU_SET_VALUE("");

    MENU_SET_RINFO("%s", format_size(fe->size));
    if(mfile_is_regged(fe->name)){
        MENU_SET_ICON(MNI_ON, 0);
    }else{
        MENU_SET_ICON(MNI_OFF, 0);
    }
    update_status(entry, info);
    
    if (entry->selected && view_file)
    {
        info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
        bmp_fill(COLOR_BLACK, 0, 0, 720, 480);
        
        char filename[MAX_PATH_LEN];
        snprintf(filename, sizeof(filename), "%s%s", gPath, fe->name);

        int status = 0;

        /* custom handler? try it first */
        char *ext = fileman_get_extension(filename);
        struct filetype_handler *filetype = fileman_find_filetype(ext);
        if (filetype)
            status = filetype->handler(FILEMAN_CMD_VIEW_IN_MENU, filename, NULL);
        
        /* custom handler doesn't know how to display the file? try the default handler */
        if (status == 0) status = text_handler(FILEMAN_CMD_VIEW_IN_MENU, filename, NULL);
        
        /* error? */
        if (status <= 0) bmp_printf(FONT_MED, 0, 460, "Error viewing %s (%s)", gPath, filetype->type);
        else bmp_printf(FONT_MED, 0, 460, "%s", filename);
    }
}

static MENU_SELECT_FUNC(default_select_action)
{
    beep();
}

static MENU_UPDATE_FUNC(update_action)
{
    MENU_SET_VALUE("");
    update_status(entry, info);
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
    scandir_sem = create_named_semaphore("scandir", 1);
    menu_add("Debug", fileman_menu, COUNT(fileman_menu));
    op_mode = FILE_OP_NONE;
    mfile_root = AllocateMemory(sizeof(FILES_LIST));
    memset(mfile_root,0,sizeof(FILES_LIST));
    mfile_root->next = NULL;
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
