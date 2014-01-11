#define CONFIG_CONSOLE
#define _file_man_c_

#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include "file_man.h"


//Definitions
#define MAX_PATH_LEN 0x80
struct file_entry
{
    struct file_entry * next;
    struct menu_entry menu_entry;
    char name[MAX_PATH_LEN];
    unsigned int size;
    unsigned int type: 2;
    unsigned int timestamp;
};

typedef struct _multi_files
{
    struct _multi_files *next;
    char name[MAX_PATH_LEN];
}FILES_LIST;


#define TYPE_DIR 0
#define TYPE_FILE 1
#define TYPE_ACTION 2

enum _FILER_OP {
    FILE_OP_NONE,
    FILE_OP_COPY,
    FILE_OP_MOVE,
    FILE_OP_PREVIEW
};

#define MAX_FILETYPE_HANDLERS 32
struct filetype_handler
{
    char *extension;
    char *type;
    filetype_handler_func handler;
};

//Global values
static char gPath[MAX_PATH_LEN];
static char gStatusMsg[60];
static unsigned int op_mode;

static int cf_present;
static int sd_present;

static int view_file = 0; /* view file mode */
static struct file_entry * file_entries = 0;
static FILES_LIST *mfile_root;
static struct semaphore * mfile_sem = 0; /* exclusive access to the list of selected files (can't change while copying, for example) */

/**
 * file copying and moving are background tasks;
 * if there is such an operation in progress, it requires exclusive access to mfile list (selected files)
 * simple error handler: just beep
 */
#define MFILE_SEM(x) \
{ \
    if (take_semaphore(mfile_sem, 200) == 0) \
    { \
        x; \
        give_semaphore(mfile_sem); \
    } \
    else beep(); \
}

static int fileman_filetype_registered = 0;

//Prototypes
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
static unsigned int mfile_add_tail(char* path);
static unsigned int mfile_clean_all();
static int mfile_is_regged(char *fname);
struct filetype_handler fileman_filetypes[MAX_FILETYPE_HANDLERS];

/**********************************
 ** code start from here
 **********************************/

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

static struct filetype_handler *fileman_find_filetype(char *extension)
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
        .name = "File Manager",
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
        menu_remove("File Manager", &(file_entries->menu_entry), 1);
        //console_printf("%s\n", file_entries->name);
        FreeMemory(file_entries);
        file_entries = next;
    }
}

static struct file_entry * add_file_entry(char* txt, int type, int size, int timestamp)
{
    struct file_entry * fe = AllocateMemory(sizeof(struct file_entry));
    if (!fe) return 0;
    memset(fe, 0, sizeof(struct file_entry));
    snprintf(fe->name, sizeof(fe->name), "%s", txt);
    fe->size = size;
    fe->timestamp = timestamp;

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

// Comparison used for the Mergesort on the filenames
// Returns true if a should be ordered before b
static bool ordered_file_entries(struct file_entry *a, struct file_entry *b)
{
    // If either file type is an action, don't change the order
    if (a->type == TYPE_ACTION || b->type == TYPE_ACTION) return true;

    // Directories are grouped before files
    if (a->type != b->type) return a->type < b->type;

    // If the file types are the same, order alphabetically
    int result = strcmp(a->name, b->name);
    return (result < 0) || (result == 0 && strlen(a->name) <= strlen(b->name));
}

static void build_file_menu()
{
    int start_time = get_ms_clock_value();

    // Mergesort on a linked list
    // e.g., http://www.chiark.greenend.org.uk/~sgtatham/algorithms/listsort.html

    struct file_entry *list = file_entries;
    struct file_entry *p, *q, *smallest, *tail;

    int length = 1;
    int nmerges, psize, qsize, i;

    do {
        p = list;
        list = NULL; // points to the first element in the list of sorted 2*length chunks
        tail = NULL; // points to the last element in the list of sorted 2*length chunks

        nmerges = 0; // counts the number of merges in this pass

        while (p) { // if some of the list has not yet been chunked...
            nmerges++;

            // p points to the first chunk of (up to) length elements
            // q points to the following chunk of (up to) length elements, if it exists
            q = p;
            psize = 0;
            for (i = 0; i < length; i++) {
                psize++;
                q = q->next;
                if (q == NULL) break;
            }
            qsize = length; // q may be shorted than qsize, so checking for NULL will be necessary

            while (psize > 0 || (qsize > 0 && q)) {
                // determine the smallest unsorted element
                if (psize == 0) { // p is empty
                    smallest = q;
                    q = q->next;
                    qsize--;
                } else if (qsize == 0 || q == NULL) { // q is empty
                    smallest = p;
                    p = p->next;
                    psize--;
                } else if (ordered_file_entries(p,q)) { // first element of p is lower than (or the same as) the first element of q
                    smallest = p;
                    p = p->next;
                    psize--;
                } else { // first element of q is lower than first element of p
                    smallest = q;
                    q = q->next;
                    qsize--;
                }

                // adds the smallest unsorted element to the end of the sorted list
                if (tail) {
                    tail->next = smallest;
                } else {
                    list = smallest;
                }
                tail = smallest;
            }

            // move the pointer past the end of the sorted chunks
            p = q;
        }

        tail->next = NULL;

        length *= 2;

    } while ((nmerges > 1) && (get_ms_clock_value() - start_time < 3000)); // Allows 3 seconds for the Mergesort

    file_entries = list;

    for (struct file_entry * fe = file_entries; fe; fe = fe->next)
        menu_add("File Manager", &(fe->menu_entry), 1);
}

static struct semaphore * scandir_sem = 0;

/* this is called from file copy/move tasks as well as from GUI task, so it needs to be thread safe */
static void ScanDir(char *path)
{
    take_semaphore(scandir_sem, 0);

    clear_file_menu();

    if (strlen(path) == 0)
    {
        add_file_entry("A:/", TYPE_DIR, 0, 0);
        add_file_entry("B:/", TYPE_DIR, 0, 0);
        build_file_menu();
        give_semaphore(scandir_sem);
        return;
    }

    struct fio_file file;
    struct fio_dirent * dirent = 0;

    dirent = FIO_FindFirstEx( path, &file );
    if( IS_ERROR(dirent) )
    {
        add_file_entry("../", TYPE_DIR, 0, 0);
        build_file_menu();
        give_semaphore(scandir_sem);
        return;
    }

    int n = 0;
    do
    {
        if (file.name[0] == 0) continue;        /* on ExFat it may return empty entries */ 
        if (file.name[0] == '.') continue;
        n++;
        if (file.mode & ATTR_DIRECTORY)
        {
            int len = strlen(file.name);
            snprintf(file.name + len, sizeof(file.name) - len, "/");
            add_file_entry(file.name, TYPE_DIR, 0, 0);
        }
        else
        {
            add_file_entry(file.name, TYPE_FILE, file.size, file.timestamp);
        }
    }
    while( FIO_FindNextEx( dirent, &file ) == 0);

    if (!n)
    {
        /* nothing here, add this so menu won't crash */
        add_file_entry("../", TYPE_DIR, 0, 0);
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

            e = add_file_entry("*** Cancel OP ***", TYPE_ACTION, 0, 0);
            if (e) e->menu_entry.select = FileOpCancel;

            switch (op_mode)
            {
                case FILE_OP_COPY:
                    e = add_file_entry("*** Copy Here ***", TYPE_ACTION, 0, 0);
                    if (e) e->menu_entry.select = FileCopyStart;
                    break;
                case FILE_OP_MOVE:
                    e = add_file_entry("*** Move Here ***", TYPE_ACTION, 0, 0);
                    if (e) e->menu_entry.select = FileMoveStart;
                    break;
            }
    }

    build_file_menu();

    FIO_FindClose(dirent);
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
    view_file = 0;

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

static void
FileCopy(void *unused)
{
MFILE_SEM (
    char fname[MAX_PATH_LEN];
    char tmpdst[MAX_PATH_LEN];
    char dstfile[MAX_PATH_LEN];
    size_t totallen = 0;
    FILES_LIST *mf = mfile_root;
    strcpy(tmpdst,gPath);

    while(mf->next){
        mf = mf->next;
        dstfile[0] = 0;
        fname[0] = 0;
        totallen = strlen(mf->name);
        char *p = mf->name + totallen;
        while (p > mf->name && *p != '/') p--;
        strcpy(fname,p+1);
        
        snprintf(dstfile,MAX_PATH_LEN,"%s%s",tmpdst,fname);
        if(streq(mf->name,dstfile)) continue; // src and dst are idential.skip this transaction.
        
        snprintf(gStatusMsg, sizeof(gStatusMsg), "Copying %s to %s...", mf->name, tmpdst);
        int err = FIO_CopyFile(mf->name,dstfile);
        if (err) snprintf(gStatusMsg, sizeof(gStatusMsg), "Copy error (%d)", err);
        else gStatusMsg[0] = 0;
    }

    mfile_clean_all();

    /* are we still in the same dir? rescan */
    if(!strcmp(gPath,tmpdst)) ScanDir(gPath);
)
}

static void
FileMove(void *unused)
{
MFILE_SEM (
    
    char fname[MAX_PATH_LEN];
    char tmpdst[MAX_PATH_LEN];
    char dstfile[MAX_PATH_LEN];
    size_t totallen = 0;
    FILES_LIST *mf = mfile_root;
    strcpy(tmpdst,gPath);

    while(mf->next){
        mf = mf->next;
        dstfile[0] = 0;
        fname[0] = 0;
        totallen = strlen(mf->name);
        char *p = mf->name + totallen;
        while (p > mf->name && *p != '/') p--;
        strcpy(fname,p+1);
        
        snprintf(dstfile,MAX_PATH_LEN,"%s%s",tmpdst,fname);
        if(streq(mf->name,dstfile)) continue; // src and dst are idential.skip this transaction.
        
        snprintf(gStatusMsg, sizeof(gStatusMsg), "Moving %s to %s...", mf->name, tmpdst);
        int err = FIO_MoveFile(mf->name,dstfile);
        if (err) snprintf(gStatusMsg, sizeof(gStatusMsg), "Move error (%d)", err);
        else gStatusMsg[0] = 0;
    }

    mfile_clean_all();
    ScanDir(gPath);
)
}


static MENU_SELECT_FUNC(FileCopyStart)
{
MFILE_SEM (
    task_create("filecopy_task", 0x1b, 0x4000, FileCopy, 0);
    op_mode = FILE_OP_NONE;
)
    ScanDir(gPath);
}

static MENU_SELECT_FUNC(FileMoveStart)
{
MFILE_SEM (
    task_create("filemove_task", 0x1b, 0x4000, FileMove, 0);
    op_mode = FILE_OP_NONE;
)
    ScanDir(gPath);
}

static MENU_SELECT_FUNC(FileOpCancel)
{
MFILE_SEM (
    mfile_clean_all();
    op_mode = FILE_OP_NONE;
    ScanDir(gPath);
)
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

static const char * format_date_size( unsigned size, unsigned timestamp )
{
    static char str[32];
    static char datestr [11];
    int year=1970;                   // Unix Epoc begins 1970-01-01
    int month=11;                    // This will be the returned MONTH NUMBER.
    int day;                         // This will be the returned day number. 
    int dayInSeconds=86400;          // 60secs*60mins*24hours
    int daysInYear=365;              // Non Leap Year
    int daysInLYear=daysInYear+1;    // Leap year
    int days=timestamp/dayInSeconds; // Days passed since UNIX Epoc
    int tmpDays=days+1;              // If passed (timestamp < dayInSeconds), it will return 0, so add 1

    while(tmpDays>=daysInYear)       // Start adding years to 1970
	{      
        year++;
        if ((year)%4==0&&((year)%100!=0||(year)%400==0)) tmpDays-=daysInLYear; else tmpDays-=daysInYear;
    }

    int monthsInDays[12] = {-1,30,59,90,120,151,181,212,243,273,304,334};
    if (!(year)%4==0&&((year)%100!=0||(year)%400==0))  // The year is not a leap year
	{
        monthsInDays[0] = 0;
		monthsInDays[1] =31;
    }

    while (month>0)
    {
        if (tmpDays>monthsInDays[month]) break;       // month+1 is now the month number.
        month--;
    }
    day=tmpDays-monthsInDays[month];                  // Setup the date
    month++;                                          // Increment by one to give the accurate month
    if (day==0) {year--; month=12; day=31;}			  // Ugly hack but it works, eg. 1971.01.00 -> 1970.12.31
	
    if (date_format==DATE_FORMAT_YYYY_MM_DD)          // Use the date format of the camera to format the date string
        snprintf( datestr, sizeof(datestr), "%d.%02d.%02d ", year, month, day);
    else if (date_format==DATE_FORMAT_MM_DD_YYYY)
        snprintf( datestr, sizeof(datestr), "%02d/%02d/%d ", month, day, year);
    else  
        snprintf( datestr, sizeof(datestr), "%02d/%02d/%d ", day, month, year);

    if ( size >= 1000*1024*1024-512*1024/10 ) // transition from "999.9MB" to " 0.98GB"
    {
        int size_gb = (size/1024/1024 * 100 + 512)  / 1024;
        snprintf( str, sizeof(str), "%s %s%2d.%02dGB", datestr, FMT_FIXEDPOINT2(size_gb));
    }
    else if ( size >= 10*1024*1024-512*1024/100 ) // transition from " 9.99MB" to " 10.0MB"
    {
        int size_mb = (size/1024 * 10 + 512) / 1024;
        snprintf( str, sizeof(str), "%s %s%3d.%01dMB", datestr, FMT_FIXEDPOINT1(size_mb));
    }
    else if ( size >= 1000*1024-512/10 ) // transition from "999.9kB" to " 0.98MB"
    {
        int size_mb = (size/1024 * 100 + 512) / 1024;
        snprintf( str, sizeof(str), "%s %s%2d.%02dMB", datestr, FMT_FIXEDPOINT2(size_mb));
    }
    else if ( size >= 10*1024-512/100 ) // transition from " 9.99kB" to " 10.0kB"
    {
        int size_kb = (size * 10 + 512) / 1024;
        snprintf( str, sizeof(str), "%s %s%3d.%01dkB", datestr, FMT_FIXEDPOINT1(size_kb));
    }
    else if ( size >= 1000 ) // transition from "  999 B" to " 0.98kB"
    {
        int size_kb = (size * 100 + 512) / 1024;
        snprintf( str, sizeof(str), "%s %s%2d.%02dkB", datestr, FMT_FIXEDPOINT2(size_kb));
    }
    else
    {
        snprintf( str, sizeof(str), "%s   %3d B", datestr, size);
    }

    return str;
}

static MENU_SELECT_FUNC(CopyFile)
{
MFILE_SEM (
    if (!mfile_root->next) /* nothing selected, operate on current file */
        mfile_add_tail(gPath);

    op_mode = FILE_OP_COPY;
    BrowseUp();
)
}

static MENU_SELECT_FUNC(MoveFile)
{
MFILE_SEM (
    if (!mfile_root->next) /* nothing selected, operate on current file */
        mfile_add_tail(gPath);

    op_mode = FILE_OP_MOVE;
    BrowseUp();
)
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
    
    BrowseUp();
    view_file = !view_file;
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
MFILE_SEM (

    if (!mfile_root->next) /* nothing selected, operate on current file */
        mfile_add_tail(gPath);

    if (!delete_confirm_flag)
    {
        delete_confirm_flag = get_ms_clock_value();
        beep();
    }
    else
    {
        delete_confirm_flag = 0;
        
        for (FILES_LIST *mf = mfile_root->next; mf; mf = mf->next)
        {
            if (streq(mf->name+1, ":/AUTOEXEC.BIN"))
            {
                beep();
                continue;
            }
            FIO_RemoveFile(mf->name);
        }
        
        mfile_clean_all();
        BrowseUp();
    }

)
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
    FILES_LIST *mf = mfile_root;
    while(mf->next)
    {
        mf = mf->next;
        if(streq(mf->name,fname))
        { //match
            return 1;
        }
    }
    return 0;
}

static unsigned int 
mfile_find_remove(char* path)
{
    FILES_LIST *prevmf;
    FILES_LIST *mf = mfile_root;
    while(mf->next)
    {
        prevmf = mf;
        mf = mf->next;
        if(!strcmp(mf->name,path))
        { //match
            prevmf->next = mf->next;
            FreeMemory((void *)mf);
            return 1;
        }
    }
    return 0;
}

static unsigned int 
mfile_add_tail(char* path)
{
    FILES_LIST *newmf;
    FILES_LIST *mf = mfile_root;
    while(mf->next)
        mf = mf->next;

    newmf = AllocateMemory(sizeof(FILES_LIST));
    memset(newmf,0,sizeof(FILES_LIST));
    strcpy(newmf->name, path);
    newmf->next = NULL;
    mf->next = newmf;

    return 0;
}

static unsigned int 
mfile_clean_all()
{
    FILES_LIST *prevmf;
    FILES_LIST *mf = mfile_root;
    while(mf->next)
    {
        prevmf = mf;
        mf = mf->next;
        prevmf->next = mf->next;
        FreeMemory((void *)mf);
        mf = prevmf;
    }
    return 0;
}

static int mfile_get_count()
{
    int count = 0;
    for (FILES_LIST *mf = mfile_root->next; mf; mf = mf->next)
        count++;

    return count;
}

static int path_strip_last_item(char* dst, int maxlen, char* src)
{
    snprintf(dst, maxlen, "%s", src);
    char* p = dst + strlen(dst) - 2;
    while (p > dst && *p != '/') p--;
    if (*p == '/')
    {
        *(p+1) = 0;
        return 1;
    }
    return 0;
}

/* how many different directories are in the list of selected files? */
/* (each file is identified by full path) */
static int mfile_get_dir_count()
{
    int count = 0;
    for (FILES_LIST *mf = mfile_root->next; mf; mf = mf->next)
    {
        char dir[MAX_PATH_LEN];
        if (!path_strip_last_item(dir, sizeof(dir), mf->name)) continue;
        count++;
        
        for (FILES_LIST * mf2 = mf->next; mf2; mf2 = mf2->next)
        {
            char dir2[MAX_PATH_LEN];
            if (!path_strip_last_item(dir2, sizeof(dir2), mf2->name)) continue;
            if (streq(dir, dir2))
            {
                count--;
                break;
            }
        }
    }

    return count;
}

static MENU_SELECT_FUNC(mfile_clear_all_selected_menu)
{
MFILE_SEM (
    mfile_clean_all();
    BrowseUp();
)
}

static MENU_SELECT_FUNC(select_multi_first)
{
MFILE_SEM (
    //Find already registerd entry (toggle reg/rem)
    if(mfile_find_remove(gPath) == 0)
        mfile_add_tail(gPath);

    if(mfile_root->next == NULL) op_mode = FILE_OP_NONE;

    BrowseUp();
)
}

static MENU_SELECT_FUNC(select_multi)
{
MFILE_SEM (
    char filename[MAX_PATH_LEN];
    struct file_entry * fe = (struct file_entry *) priv;
    if (!fe) return;
    snprintf(filename, sizeof(filename), "%s%s", gPath, fe->name);

    if(mfile_find_remove(filename) == 0)
        mfile_add_tail(filename);

    if(mfile_root->next == NULL) op_mode = FILE_OP_NONE;
)
}

static MENU_SELECT_FUNC(select_by_extension)
{
MFILE_SEM (
    char* ext = gPath + strlen(gPath) - 1;
    while (ext > gPath && *ext != '/' && *ext != '.') ext--;
    if (*ext == '.')
    {
        /* we might lose gPath when browsing up, so backup the extension here */
        char Ext[5];
        snprintf(Ext, sizeof(Ext), "%s", ext);
        
        BrowseUp();
        
        for (struct file_entry * fe = file_entries; fe; fe = fe->next)
        {
            char* fe_ext = fe->name + strlen(fe->name) - strlen(Ext);
            if (streq(Ext, fe_ext))
            {
                char path[MAX_PATH_LEN];
                snprintf(path, sizeof(path), "%s%s", gPath, fe->name);
                mfile_find_remove(path);
                mfile_add_tail(path);
            }
        }
    }
    else beep();
)
}

static MENU_SELECT_FUNC(file_menu)
{
    struct file_entry * fe = (struct file_entry *) priv;
    if (!fe) return;

    /* fe will be freed in clear_file_menu; backup things that we are going to reuse */
    char name[MAX_PATH_LEN];
    snprintf(name, sizeof(name), "%s", fe->name);
    int size = fe->size;
    int timestamp = fe->timestamp;
	
    STR_APPEND(gPath, "%s", name);

    clear_file_menu();
    /* at this point, fe was freed and is no longer valid */
    fe = 0;

    struct file_entry * e;
    
    /* note: need to add these in reverse order */

    int sel = mfile_get_count();

    {
        char* ext = name;
        while (*ext && *ext != '.') ext++;
        if (*ext == '.')
        {
            char msg[100];
            snprintf(msg, sizeof(msg), "Select *%s", ext);
            e = add_file_entry(msg, TYPE_ACTION, 0, 0);
            if (e)
            {
                e->menu_entry.select = select_by_extension;
                e->menu_entry.help = "Press PLAY to select individual files.";
            }
        }
    }

    if (sel)
    {
        e = add_file_entry("Clear selection", TYPE_ACTION, 0, 0);
        if (e) e->menu_entry.select = mfile_clear_all_selected_menu;
    }

    e = add_file_entry("Delete", TYPE_ACTION, 0, 0);
    if (e)
    {
        e->menu_entry.select = delete_file;
        e->menu_entry.update = delete_confirm;
    }

    e = add_file_entry("Move", TYPE_ACTION, 0, 0);
    if (e)
    {
        e->menu_entry.select = MoveFile;
        //e->menu_entry.update = MoveFileProgress;
    }

    e = add_file_entry("Copy", TYPE_ACTION, 0, 0);
    if (e)
    {
        e->menu_entry.select = CopyFile;
        //e->menu_entry.update = CopyFileProgress;
    }

    if (!sel)
    {
        e = add_file_entry("View", TYPE_ACTION, 0, 0);
        if (e)
        {
            e->menu_entry.select = viewfile_toggle;
            e->menu_entry.update = viewfile_update;
        }
    }

    if (sel == 0)
    {
        e = add_file_entry(name, TYPE_FILE, size, timestamp);
        if (e)
        {
            e->menu_entry.select = BrowseUpMenu;
            e->menu_entry.priv = e;
        }
    }
    else
    {
        int dirs = mfile_get_dir_count();
        char msg[100];
        if (dirs > 1)
            snprintf(msg, sizeof(msg), "%d files from %d folders", sel, dirs);
        else
            snprintf(msg, sizeof(msg), "%d selected files", sel);
        e = add_file_entry(msg, TYPE_ACTION, 0, 0);
        if (e)
        {
            e->menu_entry.icon_type = IT_BOOL,
            e->menu_entry.select = BrowseUpMenu;
            e->menu_entry.priv = e;
        }
    }

    build_file_menu();
}

static MENU_SELECT_FUNC(select_file)
{
    if (view_file) { view_file = 0; return; }

    if (delta < 0)
    {
        select_multi(priv, delta);
    }
    else
    {
        file_menu(priv, delta);
    }
}

static MENU_UPDATE_FUNC(update_status)
{
    if (!entry->help)
        MENU_SET_HELP(gPath);

    if (gStatusMsg[0])
    {
        MENU_SET_WARNING(MENU_WARN_INFO, "%s", gStatusMsg);
    }
    else
    {
        int n = mfile_get_count();
        
        if (n)
        {
            char msg[70];
            snprintf(
                msg, sizeof(msg), "%s ",
                op_mode == FILE_OP_COPY ? "Copy" :
                op_mode == FILE_OP_MOVE ? "Move" :
                                          "Selected"
            );

            if (n == 1)
            {
                STR_APPEND(msg, mfile_root->next->name);
            }
            else
            {
                STR_APPEND(msg, "%d files", n);
                int dirs = mfile_get_dir_count();
                if (dirs > 1)
                    STR_APPEND(msg, " from %d folders", dirs);
            }
            
            MENU_SET_WARNING(MENU_WARN_INFO, "%s", msg);
        }
    }
}

static MENU_UPDATE_FUNC(update_file)
{
    struct file_entry * fe = (struct file_entry *) entry->priv;
    MENU_SET_VALUE("");
    MENU_SET_RINFO("%s", format_date_size(fe->size,fe->timestamp));

    char filename[MAX_PATH_LEN];
    snprintf(filename, sizeof(filename), "%s%s", gPath, fe->name);

    MENU_SET_ICON(mfile_is_regged(filename) ? MNI_ON : MNI_OFF, 0);
    update_status(entry, info);
    
    static int dirty = 0;
    if (!view_file) dirty = 1;
    
    if (entry->selected && view_file)
    {
        static int last_updated = 0;
        int t = get_ms_clock_value();
        if (t - last_updated > 1000) dirty = 1;

        static char prev_filename[MAX_PATH_LEN];
        if (!streq(prev_filename, filename)) dirty = 1;
        snprintf(prev_filename, sizeof(prev_filename), "%s", filename);

        if (dirty)
        {
            dirty = 0;
            info->custom_drawing = CUSTOM_DRAW_THIS_MENU;
            bmp_fill(COLOR_BLACK, 0, 0, 720, 480);

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
            
            if (status != 1) dirty = 1;
        }
        else
        {
            /* nothing changed, keep previous screen */
            info->custom_drawing = CUSTOM_DRAW_DO_NOT_DRAW;
        }
        last_updated = get_ms_clock_value();
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

static unsigned int fileman_init()
{
    scandir_sem = create_named_semaphore("scandir", 1);
    mfile_sem = create_named_semaphore("mfile", 1);
    menu_add("Debug", fileman_menu, COUNT(fileman_menu));
    op_mode = FILE_OP_NONE;
    mfile_root = AllocateMemory(sizeof(FILES_LIST));
    memset(mfile_root,0,sizeof(FILES_LIST));
    mfile_root->next = NULL;
    InitRootDir();
    
    return 0;
}

static unsigned int fileman_deinit()
{
    //experimental. maybe not working yet.
    if(mfile_root->next) return -1;

    //FUTURE TODO: release semaphore here.
    clear_file_menu();
    mfile_clean_all();
    FreeMemory(mfile_root);
    menu_remove("Debug", fileman_menu, COUNT(fileman_menu));
    return 0;
}


MODULE_INFO_START()
    MODULE_INIT(fileman_init)
    MODULE_DEINIT(fileman_deinit)
MODULE_INFO_END()
