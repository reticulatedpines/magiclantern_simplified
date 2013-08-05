/** 
 * Routines for custom file prefix (e.g. IMG_1234.CR2 -> ABCD1234.CR2)
 **/

#include "dryos.h"
#include "bmp.h"
#include "property.h"

static char file_prefix[8] = "IMG_";
static char old_prefix[8] = "IMG_";
static int custom_file_prefix = 0;
static int prop_handler_check = 0;
static int check_key = 0;

PROP_HANDLER(PROP_FILE_PREFIX)
{
    snprintf(file_prefix, sizeof(file_prefix), "%s", (const char *)buf);
    prop_handler_check = 1;
}

char* get_file_prefix()
{
    return file_prefix;
}

/* only one module (task, whatever) can set a custom prefix; all other requests will be denied */
/* returns a "key" with which you can restore the file prefix (so it won't get restored by mistake) */

int file_prefix_set(char* new_prefix)
{
    if (!prop_handler_check)
    {
        /* prop handler doesn't work; avoid flooding with requests */
        return 0;
    }

    /* too lazy to do a proper test and set (patch welcome) */
    int old_stat = cli();
    
    if (custom_file_prefix)
    {
        /* already set by someone else? */
        sei(old_stat);
        return 0;
    }

    custom_file_prefix = 1;
    sei(old_stat);

    /* save the old prefix, so we know what to restore */
    snprintf(old_prefix, sizeof(old_prefix), "%s", file_prefix);

    /* request the new prefix */
    char buf[8];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%s", new_prefix);
    lens_wait_readytotakepic(64);
    int ok = prop_request_change_wait(PROP_FILE_PREFIX, buf, 8, 1000);
    
    /* did it work? */
    if (ok)
    {
        check_key = rand();
        return check_key + ~(*(int*)buf);
    }
    else
    {
        custom_file_prefix = 0;
        return 0;
    }
}

int file_prefix_reset(int key)
{
    if (custom_file_prefix)
    {
        int expected_key = check_key + ~(*(int*)file_prefix);
        if (key == expected_key)
        {
            lens_wait_readytotakepic(64);
            int ok = prop_request_change_wait(PROP_FILE_PREFIX, old_prefix, 8, 1000);
            if (ok)
            {
                custom_file_prefix = 0;
                return 1;
            }
        }
    }
    return 0;
}
