#include "shoot.h"
#include "config-defines.h"

#include "dryos.h"
#include "bmp.h"
#include "version.h"
#include "config.h"
#include "menu.h"
#include "property.h"
#include "lens.h"
#include "gui.h"
#include "math.h"
#include "raw.h"
#include "histogram.h"
#include "fileprefix.h"
#include "module.h"

static CONFIG_INT("post.deflicker", post_deflicker, 0);
static CONFIG_INT("post.deflicker.sidecar", post_deflicker_sidecar_type, 1);
static CONFIG_INT("post.deflicker.prctile", post_deflicker_percentile, 50);
static CONFIG_INT("post.deflicker.level", post_deflicker_target_level, -4);

static int deflicker_last_correction_x100 = 0;
static struct semaphore * deflicker_sem = 0;
static volatile int deflicker_waiting = 0;

static char* xmp_template =
"<x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"Magic Lantern\">\n"
" <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
"  <rdf:Description rdf:about=\"\"\n"
"    xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"
"    xmlns:photoshop=\"http://ns.adobe.com/photoshop/1.0/\"\n"
"    xmlns:crs=\"http://ns.adobe.com/camera-raw-settings/1.0/\"\n"
"   photoshop:DateCreated=\"2050-01-01T00:00:00:00\"\n"
"   photoshop:EmbeddedXMPDigest=\"\"\n"
"   crs:ProcessVersion=\"6.7\"\n"
"   crs:Exposure2012=\"%s%d.%05d\">\n"
"   <dc:subject>\n"
"    <rdf:Bag>\n"
"     <rdf:li>ML Post-Deflicker</rdf:li>\n"
"    </rdf:Bag>\n"
"   </dc:subject>\n"
"  </rdf:Description>\n"
" </rdf:RDF>\n"
"</x:xmpmeta>\n";

static char* ufraw_template =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
"<UFRaw Version='7'>\n"
"<InputFilename>%s</InputFilename>\n"
"<OutputFilename>%s</OutputFilename>\n"
"<Exposure>%s%d.%05d</Exposure>\n"
"<ExposureNorm>1</ExposureNorm>\n"
"<ClipHighlights>film</ClipHighlights>\n"
"<OutputType>4</OutputType>\n"
"</UFRaw>\n";

static void post_deflicker_save_sidecar_file(int type, char* photo_filename, float ev)
{
    /* find and strip extension */
    char* ext = photo_filename + strlen(photo_filename) - 1;
    while (ext > photo_filename && *ext != '/' && *ext != '.') ext--;
    if (*ext != '.') return;
    *ext = 0;
    
    /* find and strip base filename (e.g. IMG_1234) */
    char* p = ext;
    while (p > photo_filename && *p != '/') p--;
    if (*p != '/') return;
    *p = 0;
    
    /* path components */
    char* dir = photo_filename; /* A:/DCIM/100CANON */
    char* basename = p+1;       /* IMG_1234 */
    char* extension = ext+1;    /* CR2 */
    
    //~ NotifyBox(2000, "'%s'\n'%s'\n'%s'", dir, basename, extension);
    
    char sidecar[100];
    snprintf(sidecar, sizeof(sidecar), "%s/%s.%s", dir, basename, type ? "UFR" : "XMP");

    FILE* f = FIO_CreateFile(sidecar);
    if (f == INVALID_PTR) return;
    if (type == 0)
    {
        /* not sure */
        int evi = ev * 100000;
        
        my_fprintf(f, xmp_template, FMT_FIXEDPOINT5S(evi));
    }
    else if (type == 1)
    {
        char raw[100];
        char jpg[100];
        snprintf(raw, sizeof(raw), "%s.%s", basename, extension);
        snprintf(jpg, sizeof(jpg), "%s.JPG", basename);
        ev = COERCE(ev, -6, 6);
        int evi = ev * 100000;
        my_fprintf(f, ufraw_template, raw, jpg, FMT_FIXEDPOINT5(evi));
    }
    FIO_CloseFile(f);
}

static void post_deflicker_save_sidecar_file_for_cr2(int type, int file_number, float ev)
{
    char fn[100];
    snprintf(fn, sizeof(fn), "%s/%s%04d.CR2", get_dcim_dir(), get_file_prefix(), file_number);

    post_deflicker_save_sidecar_file(type, fn, ev);
}

static void post_deflicker_task()
{
    /* not quite correct in burst mode, but at least only one task will run at a time */
    /* so at least the last deflicker in a burst sequence should be correct */
    deflicker_waiting++;
    take_semaphore(deflicker_sem, 0);
    deflicker_waiting--;
    
    int raw_fast = raw_hist_get_percentile_level(post_deflicker_percentile*10, GRAY_PROJECTION_GREEN, 4);
    //~ console_printf("fast deflick: %d\n", raw_fast);
    int raw = raw_fast;
        
    /* no rush? do a precise deflicker */
    for (int i = 0; i < 10; i++)
    {
        msleep(100);
        if (deflicker_waiting) break;
    }
    if (!deflicker_waiting)
    {
        int raw_precise = raw_hist_get_percentile_level(post_deflicker_percentile*10, GRAY_PROJECTION_GREEN, 0);
        //~ console_printf("precise deflick: %d\n", raw_precise);
        if (raw_precise > 0 && raw_precise < 16384) raw = raw_precise;
    }
    //~ else console_printf("hurry, hurry\n");
    
    if (raw <= 0 || raw >= 16384)
    {
        deflicker_last_correction_x100 = 0;
        give_semaphore(deflicker_sem);
        return;
    }
    float ev = raw_to_ev(raw);
    float correction = post_deflicker_target_level - ev;
    deflicker_last_correction_x100 = (int)roundf(correction * 100);

    console_printf("deflick corr: %s%d.%02d\n", FMT_FIXEDPOINT2S(deflicker_last_correction_x100));
    post_deflicker_save_sidecar_file_for_cr2(post_deflicker_sidecar_type, get_shooting_card()->file_number, correction);
    give_semaphore(deflicker_sem);
}

static void post_deflicker_step()
{
    if (!post_deflicker) return;

    /* not a really good idea to slow down the property task */
    /* must have a lower priority than clock_task */
    task_create("deflicker_task", 0x1a, 0x1000, post_deflicker_task, (void*) 0);
}

static MENU_UPDATE_FUNC(post_deflicker_update)
{
    if (!can_use_raw_overlays_photo())
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Photo RAW data not available.");

    if (is_hdr_bracketing_enabled())
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Not compatible with HDR bracketing.");

    if (image_review_time == 0)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Enable image review from Canon menu.");
    
    if (is_continuous_drive())
        MENU_SET_WARNING(MENU_WARN_ADVICE, "Not fully compatible with continuous drive.");

    if (post_deflicker)
    {
        MENU_SET_VALUE(post_deflicker_sidecar_type ? "UFRaw" : "XMP");
        MENU_SET_RINFO("%dEV/%d%%", post_deflicker_target_level, post_deflicker_percentile);
    }
    
    if (post_deflicker && post_deflicker_sidecar_type==1)
        MENU_SET_WARNING(MENU_WARN_INFO, "You must rename *.UFR to *.ufraw: rename 's/UFR$/ufraw' *");
}

PROP_HANDLER(PROP_GUI_STATE)
{
    int* data = buf;
    if (data[0] == GUISTATE_QR)
    {
        post_deflicker_step();
    }
}

static struct menu_entry post_deflicker_menu[] = {
    {
        .name = "Post Deflicker", 
        .priv = &post_deflicker, 
        .max = 1,
        .update = post_deflicker_update,
        .help  = "Create sidecar files with exposure compensation,",
        .help2 = "so all your pics look equally exposed, without flicker.",
        .works_best_in = DEP_PHOTO_MODE,
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name = "Sidecar file type",
                .priv = &post_deflicker_sidecar_type,
                .max = 1,
                .choices = CHOICES("Adobe XMP", "UFRaw"),
                .help = "Sidecar file format, for deflicker metadata.",
            },
            {
                .name = "Deflicker percentile",
                .priv = &post_deflicker_percentile,
                .min = 20,
                .max = 80,
                .unit = UNIT_PERCENT,
                .help  = "Where to meter for deflickering. Recommended: 50% (median).",
                .help2 = "Try 75% if you get black borders (e.g. Samyang 8mm on 5D).",
            },
            {
                .name = "Deflicker target level",
                .priv = &post_deflicker_target_level,
                .min = -8,
                .max = -1,
                .choices = CHOICES("-8 EV", "-7 EV", "-6 EV", "-5 EV", "-4 EV", "-3 EV", "-2 EV", "-1 EV"),
                .help = "Desired exposure level for processed pics. 0=overexposed.",
            },
            MENU_EOL,
        },
    },
};

static unsigned int post_deflicker_init()
{
    deflicker_sem = create_named_semaphore("deflicker_sem", 1);
    menu_add("Shoot", post_deflicker_menu, COUNT(post_deflicker_menu));
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(post_deflicker_init)
MODULE_INFO_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(post_deflicker)
    MODULE_CONFIG(post_deflicker_sidecar_type)
    MODULE_CONFIG(post_deflicker_percentile)
    MODULE_CONFIG(post_deflicker_target_level)
MODULE_CONFIGS_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_GUI_STATE)
MODULE_PROPHANDLERS_END()
