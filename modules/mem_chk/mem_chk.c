#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <mem.h>

/* variables used for simple thread communication */
static uint32_t mem_chk_errors = 0;
static uint32_t mem_chk_threads = 0;
static uint32_t mem_chk_abort = 0;
static char *mem_chk_persist_state = "idle";
static char *mem_chk_flood_state = "idle";

/* parameters to persistence checks */
static uint32_t mem_chk_persist_size = 16 * 1024 * 1024;
static uint32_t mem_chk_persist_blocksize = 256 * 1024;

/* parameters for malloc/free flood tests */
static uint32_t mem_chk_flood_size = 8 * 1024 * 1024;

/* allocate and free memory all the time */
static void mem_chk_flood()
{
    char text_str[32];
    uint32_t loops = 0;
    
    util_atomic_inc(&mem_chk_threads);
    
    while(!mem_chk_abort)
    {
        uint32_t size = MAX(1, rand() % mem_chk_flood_size);
        char *buf = NULL;
        
        loops++;
        snprintf(text_str, sizeof(text_str), "check: %d", loops);
        mem_chk_flood_state = text_str;
        
        buf = malloc(size);
        
        if(!buf)
        {
            util_atomic_inc(&mem_chk_errors);
            NotifyBox(20000, "Memory allocation error");
            mem_chk_flood_state = "alloc error";
            beep();
            break;
        }
        
        free(buf);
        
        msleep(250);
    }
    
    mem_chk_flood_state = "finished";
    util_atomic_dec(&mem_chk_threads);
}

/* allocate a buffer and check if its content is changing */
static void mem_chk_persist()
{
    char text_str[32];
    char *buf = NULL;
    
    util_atomic_inc(&mem_chk_threads);
    
    mem_chk_persist_state = "malloc";
    buf = malloc(mem_chk_persist_size);
    
    if(buf)
    {
        uint32_t pos = 0;
        uint32_t byte_offset = 0;
        uint32_t cur_blockpos = 0;
        
        mem_chk_persist_state = "memset...";
        for(pos = 0; pos < mem_chk_persist_size; pos += 8)
        {
            ((uint64_t *)buf)[pos / 8] = 0x5555555555555555ULL;
        }
        
        mem_chk_persist_state = "checking...";
        
        while(!mem_chk_abort)
        {
            snprintf(text_str, sizeof(text_str), "check: %d/%d", cur_blockpos / mem_chk_persist_blocksize, mem_chk_persist_size / mem_chk_persist_blocksize);
            mem_chk_persist_state = text_str;
            
            for(pos = 0; pos < mem_chk_persist_blocksize - 16; pos += 16)
            {
                if(buf[cur_blockpos + pos + byte_offset] != 0x55)
                {
                    util_atomic_inc(&mem_chk_errors);
                    NotifyBox(20000, "Memory content changed unexpectedly");
                    mem_chk_persist_state = "CHANGED";
                    beep();
                    break;
                }
            }
            cur_blockpos += mem_chk_persist_blocksize;
            cur_blockpos %= mem_chk_persist_size;
            
            /* one byte offset after every block */
            if(!cur_blockpos)
            {
                byte_offset++;
                byte_offset %= 16;
            }
            msleep(250);
        }
        
        mem_chk_persist_state = "finished";
    }
    else
    {
        mem_chk_persist_state = "alloc error";
        util_atomic_inc(&mem_chk_errors);
        NotifyBox(20000, "Failed to allocate memory");
        beep();
    }
    
    free(buf);
    util_atomic_dec(&mem_chk_threads);
}

static void mem_chk_abort_threads()
{
    mem_chk_abort = 1;
    while(mem_chk_threads)
    {
        msleep(100);
    }
    mem_chk_abort = 0;
}

static MENU_SELECT_FUNC(mem_chk_persist_select)
{
    task_create("mem_chk_persist", 0x1e, 0x1000, mem_chk_persist, NULL);
}

static MENU_UPDATE_FUNC(mem_chk_persist_update)
{
    MENU_SET_VALUE(mem_chk_persist_state);
}

static MENU_SELECT_FUNC(mem_chk_flood_select)
{
    task_create("mem_chk_flood", 0x1e, 0x1000, mem_chk_flood, NULL);
}

static MENU_UPDATE_FUNC(mem_chk_flood_update)
{
    MENU_SET_VALUE(mem_chk_flood_state);
}


static MENU_UPDATE_FUNC(mem_chk_abort_update)
{
    MENU_SET_VALUE("Errors: %d", mem_chk_errors);
}

static MENU_SELECT_FUNC(mem_chk_abort_select)
{
    mem_chk_abort_threads();
}


static struct menu_entry mem_chk_menu[] =
{
    {
        .name = "Memory backend checks",
        .select = menu_open_submenu,
        .children =  (struct menu_entry[]) {
            {
                .name = "Persistence",
                .select = &mem_chk_persist_select,
                .update = &mem_chk_persist_update,
            },
            {
                .name = "malloc-flood",
                .select = &mem_chk_flood_select,
                .update = &mem_chk_flood_update,
            },
            {
                .name = "Abort tests",
                .select = mem_chk_abort_select,
                .update = &mem_chk_abort_update,
            },
            MENU_EOL,
        }
    }
};

unsigned int mem_chk_init()
{
    menu_add("Debug", mem_chk_menu, COUNT(mem_chk_menu));
    return 0;
}

unsigned int mem_chk_deinit()
{
    mem_chk_abort_threads();
    menu_remove("Debug", mem_chk_menu, COUNT(mem_chk_menu));
    return 0;
}




MODULE_INFO_START()
    MODULE_INIT(mem_chk_init)
    MODULE_DEINIT(mem_chk_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
MODULE_CBRS_END()

MODULE_PARAMS_START()
MODULE_PARAMS_END()

MODULE_PROPHANDLERS_START()
MODULE_PROPHANDLERS_END()
