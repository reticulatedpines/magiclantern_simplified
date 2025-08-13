// dumper for Device IDs

#include "module.h"
#include "dryos.h"
#include "compiler.h"
#include "menu.h"
#include "console.h"
#include "bmp.h"

struct Resource
{
    uint16_t device_ID;
    uint16_t blockNum;
};
SIZE_CHECK_STRUCT(Resource, 4);

// Used in a doubly linked, non-circular list.
// When next or prev point to the ResInfo_block item that pointed to the
// head of this linked list, there are no more items in that direction.
// NB, this means these can point to two different types of item.
struct ResInfo_entry
{
    struct ResInfo_entry *next;
    struct ResInfo_entry *prev;
    struct Resource resource;
    uint32_t unk_04; // I only see it assigned 0 or 1 via static analysis.
                     // Lock status possibly?
};
SIZE_CHECK_STRUCT(ResInfo_entry, 0x10);

// DryOS has an array of these, 55 (0x37) items for most cams
struct ResInfo_block
{
    struct ResInfo_entry *head;
    struct ResInfo_entry *tail;
};
SIZE_CHECK_STRUCT(ResInfo_block, 8);

static uint32_t blockMax = 0;
static struct ResInfo_block *ResBlocks = NULL;

static void dump_task()
{
    gui_stop_menu();
    msleep(200);
    
    printf("%x\n", ResBlocks);
    printf("%x\n", ResBlocks[0].head);
    printf("%x\n", ResBlocks[0].head->resource);

    FILE *fp = NULL;
    char log_name[60];
    char line[80];
    static uint32_t run_count = 0;
    for (uint32_t i = 0; i < blockMax; i++)
    {
        snprintf(log_name, sizeof(log_name), "ML/LOGS/b_%d_%d.log", run_count, i);
        fp = FIO_CreateFile(log_name);

        snprintf(line, sizeof(line), "Run %d, block %d\n", run_count, i);
        FIO_WriteFile(fp, line, strlen(line));
        struct ResInfo_entry *entry = ResBlocks[i].head;
        if (entry != NULL)
        {
            uint32_t j = 0;
            // Instead of using NULL sentinels or a circular list,
            // the end of list marker is when it points to the "parent" ResInfo_block.
            // This is also used in the array: when a block contains no devices yet,
            // head and tail point to head.
            //
            // The check against j is to stop infinite loops - this has never
            // been observed.
            while (entry != (struct ResInfo_entry *)(&ResBlocks[i])
                   && j < 999)
            {
                snprintf(line, sizeof(line), "0x%04x ", entry->resource.device_ID);
                FIO_WriteFile(fp, line, strlen(line));
                if (j % 4 == 3)
                    FIO_WriteFile(fp, "\n", 2);
                entry = entry->next;
                j++;
            }
        }

        FIO_WriteFile(fp, "\n", 2);
        FIO_CloseFile(fp);
    }
    run_count++;
}

static struct menu_entry dump_menu[] =
{
    {
        .name   = "Dump device ID blocks",
        .select = run_in_separate_task,
        .priv   = dump_task,
        .icon_type = IT_ACTION,
    }
};

static unsigned int dump_init()
{
    if (is_camera("200D", "1.0.1"))
    {
        blockMax = 55;
        ResBlocks = (struct ResInfo_block *)(*(uint32_t *)0x10998);
    }
    else if (is_camera("650D", "1.0.4"))
    {
        blockMax = 55;
        ResBlocks = (struct ResInfo_block *)(*(uint32_t *)0x316fc);
    }
    else if (is_camera("700D", "1.1.5"))
    {
        blockMax = 55;
        ResBlocks = (struct ResInfo_block *)(*(uint32_t *)0x317cc);
    }
    else if (is_camera("5D3", "1.2.3"))
    {
        blockMax = 55;
        ResBlocks = (struct ResInfo_block *)(*(uint32_t *)0x29c48);
    }
    else if (is_camera("5D3", "1.1.3"))
    {
        blockMax = 55;
        ResBlocks = (struct ResInfo_block *)(*(uint32_t *)0x29cc8);
    }
    else if (is_camera("R", "1.8.0"))
    {
        blockMax = 10;
        ResBlocks = (struct ResInfo_block *)(*(uint32_t *)0x1092c);
    }
    else
    {
        bmp_printf(FONT_MED, 20, 20, "Device ID constants not defined for this cam");
        return CBR_RET_ERROR;
    }
    
    menu_add("Debug", dump_menu, COUNT(dump_menu));
    return 0;
}

static unsigned int dump_deinit()
{
    menu_remove("Debug", dump_menu, COUNT(dump_menu));
    return 0;
}


MODULE_INFO_START()
    MODULE_INIT(dump_init)
    MODULE_DEINIT(dump_deinit)
MODULE_INFO_END()

