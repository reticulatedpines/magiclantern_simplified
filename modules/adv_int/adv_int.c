//
//  adv_int.c

//  Copyright (c) 2013 David Milligan
//


#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <raw.h>
#include <lens.h>
#include <math.h>
#include <shoot.h>

#define TRUE  1
#define FALSE 0
#define LINE_BUF_SIZE 512
#define MAX_PATH 100
#define STR_INT_MAX_LEN 20
#define FILE_BUF_SIZE 4096
#define SECONDS_IN_DAY 86400
#define MAX_TIME 5000

struct keyframe
{
    struct keyframe * next;
    int time;
    int shutter;
    int aperture;
    int iso;
    int focus;
    int interval_time;
    int kelvin;
    int bulb_duration;
    struct menu_entry menu_entry;
};

static CONFIG_INT("adv_int.enabled", adv_int, 0);
static CONFIG_INT("adv_int.use_global_time", adv_int_use_global_time, 0);
static CONFIG_INT("adv_int.loop_after", adv_int_loop_after, 0);
static CONFIG_INT("adv_int.external", adv_int_external, 0);
static int adv_int_external_pic_count = 0;
static int keyframe_shutter = 0;
static int keyframe_aperture = 0;
static int keyframe_iso = 0;
static int keyframe_focus = 0;
static int keyframe_time = 1;
static int keyframe_interval_time = 0;
static int keyframe_kelvin = 0;
static int keyframe_bulb_duration = 0;

static struct keyframe * keyframes = NULL;

static struct keyframe * current_keyframe = NULL;
static struct keyframe * last_keyframe = NULL;


static MENU_UPDATE_FUNC(update_keyframe_menu)
{
    for(struct keyframe * current = keyframes; current; current = current->next)
    {
        if(&(current->menu_entry) == entry)
        {
            if(adv_int_use_global_time)
                MENU_SET_NAME("T=%02d:%02d:%02d", current->time / 3600 % 24, current->time / 60 % 60, current->time % 60);
            else
                MENU_SET_NAME("T=%d", current->time);
            if(current->shutter)
                MENU_APPEND_RINFO("Tv=%s ",lens_format_shutter(current->shutter));
            if(current->aperture)
                MENU_APPEND_RINFO("Av=%d ", current->aperture);
                //how to convert raw to aperture ??
                //MENU_APPEND_RINFO("Av="SYM_F_SLASH"%d.%d ",raw2aperture(current->aperture) / 10, raw2aperture(current->aperture) % 10);
            if(current->iso)
                MENU_APPEND_RINFO("ISO=%d ",raw2iso(current->iso));
            if(current->focus)
                MENU_APPEND_RINFO("fcs=%d ",current->focus);
            if(current->interval_time)
                MENU_APPEND_RINFO("int=%d ",current->interval_time);
            if(current->kelvin)
                MENU_APPEND_RINFO("wb=%d ",current->kelvin);
            if(current->bulb_duration)
                MENU_APPEND_RINFO("bulb=%d ",current->bulb_duration);
            break;
        }
    }
}

static void add_keyframe_menu(struct keyframe * kfr)
{
    kfr->menu_entry.update = update_keyframe_menu;
    menu_add("List Keyframes", &(kfr->menu_entry), 1);
}

/*
 * Creates a new keyframe struct and inserts it in the list based on it's time (insertion sort)
 */
static struct keyframe * new_keyframe(struct keyframe * list, int time)
{
    if(!keyframes)
    {
        keyframes = malloc(sizeof(struct keyframe));
        if(keyframes)
        {
            memset(keyframes, 0, sizeof(struct keyframe));
            keyframes->time = time;
            add_keyframe_menu(keyframes);
        }
        return keyframes;
    }
    //search for any keyframes with this time, replace it if it already exists
    struct keyframe * current = keyframes;
    while(current)
    {
        if(current->time == time)
        {
            current->aperture = 0;
            current->focus = 0;
            current->iso = 0;
            current->shutter = 0;
            current->kelvin = 0;
            current->interval_time = 0;
            current->bulb_duration = 0;
            return current;
        }
        current = current->next;
    }
    
    struct keyframe * new_kfr = NULL;
    
    if(keyframes && time < keyframes->time)
    {
        //insert as new root
        new_kfr = malloc(sizeof(struct keyframe));
        if(new_kfr)
        {
            memset(new_kfr, 0, sizeof(struct keyframe));
            new_kfr->time = time;
            new_kfr->next = keyframes;
            keyframes = new_kfr;
            add_keyframe_menu(new_kfr);
        }
        return new_kfr;
        
    }
    else
    {
        //find where to insert new keyframe
        current = keyframes;
        while(current->next)
        {
            if(current->next && current->next->time > time)
                break;
            current = current->next;
        }
        new_kfr = malloc(sizeof(struct keyframe));
        if(new_kfr)
        {
            memset(new_kfr, 0, sizeof(struct keyframe));
            new_kfr->time = time;
            new_kfr->next = current->next;
            current->next = new_kfr;
            add_keyframe_menu(new_kfr);
        }
        return new_kfr;
    }
}

static void delete_keyframe(struct keyframe * kfr)
{
    if(kfr)
    {
        menu_remove("List Keyframes", &(kfr->menu_entry), 1);
        delete_keyframe(kfr->next);
        kfr->next = NULL;
        free(kfr);
    }
}

static int keyframe_exists(int time)
{
    for(struct keyframe * current = keyframes; current; current = current->next)
    {
        if(current->time == time)
            return TRUE;
    }
    return FALSE;
}

static MENU_SELECT_FUNC(adv_int_new_keyframe)
{
    //beep();
    struct keyframe * kfr = new_keyframe(keyframes, keyframe_time);
    if(kfr)
    {
        kfr->time = keyframe_time;
        kfr->shutter = keyframe_shutter ? lens_info.raw_shutter : 0;
        kfr->aperture = keyframe_aperture ? lens_info.raw_aperture : 0;
        kfr->iso = keyframe_iso ? lens_info.raw_iso : 0;
        kfr->focus = keyframe_focus;
        kfr->interval_time = keyframe_interval_time;
        kfr->kelvin = keyframe_kelvin;
        kfr->bulb_duration = keyframe_bulb_duration;
        NotifyBox(2000, "Keyframe Created for %d", keyframe_time);
    }
    else
    {
        NotifyBox(2000, "Error: could not create keyframe", keyframe_time);
    }
}

static int get_global_time()
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    return now.tm_sec + (60 * now.tm_min) + (3600 * now.tm_hour);
}

static int read_char(char* source, size_t * source_pos, char* out, size_t source_size)
{
    if (*source_pos >= source_size) return 0;
    *out = source[(*source_pos)++];
    return 1;
}

static int read_line(char* source, size_t * source_pos, size_t source_size, char * buf, size_t buf_size)
{
    size_t len = 0;
    
    while( len < buf_size )
    {
        int rc = read_char(source, source_pos, buf+len, source_size);
        if( rc <= 0 )
            return -1;
        
        if( buf[len] == '\r' )
            continue;
        if( buf[len] == '\n' )
        {
            buf[len] = '\0';
            return len;
        }
        
        len++;
    }
    
    return -1;
}

static int parse_next_int(char* source, size_t max_len)
{
    if(source)
    {
        char temp[STR_INT_MAX_LEN];
        size_t pos = 0;
        while(pos < max_len && isdigit(source[pos]) && pos < STR_INT_MAX_LEN)
        {
            temp[pos] = source[pos];
            pos++;
        }
        temp[pos] = '\0';
        if(strlen(temp) > 0)
            return atoi(temp);
        else
            return 0;
    }
    else
    {
        return 0;
    }
}

//for some reason I can't link to strstr
static char* my_strstr(char* source, const char* search)
{
    if(source && search && strlen(source) > 0 && strlen(search) > 0)
    {
        for (size_t pos = 0; pos < strlen(source) - strlen(search); pos++)
        {
            int found = TRUE;
            for(size_t i = 0; i < strlen(search); i++)
            {
                if(source[pos + i] != search[i])
                {
                    found = FALSE;
                    break;
                }
            }
            if(found)
                return source + pos;
        }
    }
    return NULL;
}

static int parse_property(const char * property, char * source, size_t max_len)
{
    if(source)
    {
        char * loc = my_strstr(source, property);
        return loc ? parse_next_int(loc + strlen(property), max_len) : 0;
    }
    return 0;
}

static MENU_SELECT_FUNC(adv_int_load)
{
    char filename[MAX_PATH];
    char line_temp[LINE_BUF_SIZE];
    int success = FALSE;
    
    snprintf(filename,MAX_PATH,"%sSEQ.TXT",get_config_dir());
    
    FILE* f = FIO_Open(filename, O_RDONLY | O_SYNC);
    if(f != INVALID_PTR)
    {
        char* buffer = fio_malloc(FILE_BUF_SIZE);
        if(buffer)
        {
            if(FIO_ReadFile(f, buffer, FILE_BUF_SIZE - 1))
            {
                size_t buf_pos = 0;
                while(read_line(buffer, &buf_pos, FILE_BUF_SIZE, line_temp, LINE_BUF_SIZE) > 0)
                {
                    int kfr_time = parse_next_int(line_temp, LINE_BUF_SIZE);
                    struct keyframe * new_kfr = new_keyframe(keyframes, kfr_time);
                    if(new_kfr)
                    {
                        new_kfr->shutter = parse_property("tv=", line_temp, LINE_BUF_SIZE);
                        new_kfr->aperture = parse_property("av=", line_temp, LINE_BUF_SIZE);
                        new_kfr->iso = parse_property("iso=", line_temp, LINE_BUF_SIZE);
                        new_kfr->focus = parse_property("fcs=", line_temp, LINE_BUF_SIZE);
                        new_kfr->interval_time = parse_property("int=", line_temp, LINE_BUF_SIZE);
                        new_kfr->kelvin = parse_property("wb=", line_temp, LINE_BUF_SIZE);
                        new_kfr->bulb_duration = parse_property("bulb=", line_temp, LINE_BUF_SIZE);
                    }
                }
                success = TRUE;
            }
            else
                NotifyBox(2000, "Error: Could not read file");
            fio_free(buffer);
             
        }
        else
            NotifyBox(2000, "Error: Could not create buffer");
        
        FIO_CloseFile(f);
    }
    else
        NotifyBox(2000, "Error: Could not open file");
    
    if(success)
        NotifyBox(2000, "Sequence File Loaded");
}

static MENU_SELECT_FUNC(adv_int_save)
{
    char filename[MAX_PATH];
    
    snprintf(filename,MAX_PATH,"%sSEQ.TXT",get_config_dir());
    
    FILE* f = FIO_CreateFile(filename);
    if(f != INVALID_PTR)
    {
        for(struct keyframe * current = keyframes; current; current = current->next)
        {
            my_fprintf(f,"%d:", current->time);
            if(current->shutter)
                my_fprintf(f, "tv=%d ", current->shutter);
            if(current->aperture)
                my_fprintf(f, "av=%d ", current->aperture);
            if(current->iso)
                my_fprintf(f, "iso=%d ", current->iso);
            if(current->focus)
                my_fprintf(f, "fcs=%d ", current->focus);
            if(current->interval_time)
                my_fprintf(f, "int=%d ", current->interval_time);
            if(current->kelvin)
                my_fprintf(f, "wb=%d ", current->kelvin);
            if(current->bulb_duration)
                my_fprintf(f, "bulb=%d ", current->bulb_duration);
            my_fprintf(f,"\n");
            
        }
        FIO_CloseFile(f);
        beep();
        NotifyBox(2000, "Keyframe Sequence Saved");
    }
    else
    {
        NotifyBox(2000, "Error saving file");
    }
}

static MENU_SELECT_FUNC(adv_int_clear)
{
    delete_keyframe(keyframes);
    keyframes = NULL;
    NotifyBox(2000, "All Keyframes Cleared");
}

static MENU_UPDATE_FUNC(time_menu_update)
{
    if(adv_int_use_global_time)
    {
        entry->unit = UNIT_TIME;
        MENU_SET_VALUE("%02d:%02d:%02d ", keyframe_time / 60 / 60 % 24, keyframe_time / 60 % 60, keyframe_time % 60);
        int now = get_global_time();
        MENU_SET_RINFO("%02d:%02d:%02d", now / 60 / 60 % 24, now / 60 % 60, now % 60);
    }
    else
    {
        entry->unit = UNIT_DEC;
        int seconds = get_config_var("interval.time") * keyframe_time;
        static char msg[50];
        
        msg[0] = '\0';
        if (seconds >= 3600)
        {
            STR_APPEND(msg, "%dh", seconds / 3600);
            seconds = seconds % 3600;
        }
        
        if (seconds >= 60)
        {
            STR_APPEND(msg, "%dm", seconds / 60);
            seconds = seconds % 60;
        }
        
        if (seconds || !msg[0])
        {
            STR_APPEND(msg, "%ds", seconds);
        }
        
        MENU_SET_RINFO("%s", msg);
        
    }
    if(keyframe_exists(keyframe_time))
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING,"This keyframe already exists, will be overwritten");
}

static MENU_SELECT_FUNC(new_keyframe_menu_select)
{
    if(adv_int_use_global_time && keyframe_time < get_global_time())
        keyframe_time = get_global_time();
    
    menu_open_submenu(priv, delta);
}


static MENU_UPDATE_FUNC(shutter_menu_update)
{
    MENU_SET_RINFO("%s", lens_format_shutter(lens_info.raw_shutter));
}

static MENU_UPDATE_FUNC(aperture_menu_update)
{
    //copied/modified from shoot.c
    
    int a = lens_info.aperture;
    if (!a || !lens_info.name[0]) // for unchipped lenses, always display zero
        a = 0;
    MENU_SET_RINFO(SYM_F_SLASH"%d.%d", a / 10, a % 10);
    
    if (!lens_info.aperture)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, lens_info.name[0] ? "Aperture is automatic - cannot adjust manually." : "Manual lens - cannot adjust aperture.");
    }
}

static MENU_UPDATE_FUNC(iso_menu_update)
{
    MENU_SET_RINFO("ISO%d", lens_info.iso);
}

/************************************************************/
/* From shoot.c */

static MENU_UPDATE_FUNC(shutter_display)
{
    MENU_SET_VALUE("%s", lens_format_shutter(lens_info.raw_shutter));
    if (!menu_active_but_hidden())
    {
        int Tv = APEX_TV(lens_info.raw_shutter) * 10/8;
        if (lens_info.raw_shutter) MENU_SET_RINFO("Tv%s%d.%d",FMT_FIXEDPOINT1(Tv));
    }
    
    if (lens_info.raw_shutter)
    {
        MENU_SET_ICON(MNI_PERCENT, (lens_info.raw_shutter - codes_shutter[1]) * 100 / (codes_shutter[COUNT(codes_shutter)-1] - codes_shutter[1]));
        MENU_SET_ENABLED(1);
    }
    else 
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Shutter speed is automatic - cannot adjust manually.");
    
    MENU_SET_SHORT_NAME(" "); // obvious from value
}

static MENU_UPDATE_FUNC(aperture_display)
{
    int a = lens_info.aperture;
    int av = APEX_AV(lens_info.raw_aperture) * 10/8;
    if (!a || !lens_info.name[0]) // for unchipped lenses, always display zero
        a = av = 0;
    MENU_SET_VALUE(SYM_F_SLASH"%d.%d",a / 10,a % 10,av / 8,(av % 8) * 10/8);
    
    if (!menu_active_but_hidden())
    {
        if (a) MENU_SET_RINFO("Av%s%d.%d",FMT_FIXEDPOINT1(av));
    }
    if (!lens_info.aperture)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, lens_info.name[0] ? "Aperture is automatic - cannot adjust manually." : "Manual lens - cannot adjust aperture.");
        MENU_SET_ICON(MNI_PERCENT_OFF, 0);
    }
    else
    {
        MENU_SET_ICON(MNI_PERCENT, (lens_info.raw_aperture - lens_info.raw_aperture_min) * 100 / (lens_info.raw_aperture_max - lens_info.raw_aperture_min));
        MENU_SET_ENABLED(1);
    }
    
    MENU_SET_SHORT_NAME(" "); // obvious from value
}

static MENU_UPDATE_FUNC(iso_icon_update)
{
    if (lens_info.iso)
        MENU_SET_ICON(MNI_PERCENT, (lens_info.raw_iso - codes_iso[1]) * 100 / (codes_iso[COUNT(codes_iso)-1] - codes_iso[1]));
    else
        MENU_SET_ICON(MNI_AUTO, 0);
}

static MENU_UPDATE_FUNC(iso_display)
{
    MENU_SET_VALUE("%s", lens_info.iso ? "" : "Auto");
    if (lens_info.iso)
    {
        if (lens_info.raw_iso == lens_info.iso_equiv_raw)
        {
            MENU_SET_VALUE("%d", raw2iso(lens_info.iso_equiv_raw));
            
            if (!menu_active_but_hidden())
            {
                int Sv = APEX_SV(lens_info.iso_equiv_raw) * 10/8;
                MENU_SET_RINFO("Sv%s%d.%d", FMT_FIXEDPOINT1(Sv));
            }
        }
        else
        {
            int dg = lens_info.iso_equiv_raw - lens_info.raw_iso;
            dg = dg * 10/8;
            MENU_SET_VALUE("%d",raw2iso(lens_info.iso_equiv_raw));
            MENU_SET_RINFO("%d,%s%d.%dEV",raw2iso(lens_info.raw_iso),FMT_FIXEDPOINT1S(dg));
        }
    }
    iso_icon_update(entry, info);
    MENU_SET_SHORT_NAME(" "); // obvious from value
}

/************************************************************/

static MENU_SELECT_FUNC(kelvin_menu_select)
{
    int * val = (int*)priv;
    if(delta < 0)
    {
        if(*val == 0)
            *val = KELVIN_MAX;
        else if(*val <= KELVIN_MIN)
            *val = 0;
        else
            *val -= KELVIN_STEP;
    }
    else if(delta > 0)
    {
        if(*val == 0)
            *val = KELVIN_MIN;
        else if(*val >= KELVIN_MAX)
            *val = 0;
        else
            *val += KELVIN_STEP;
    }
    
}

static MENU_UPDATE_FUNC(kelvin_menu_update)
{
    if(keyframe_kelvin)
    {
        MENU_SET_VALUE("%dK", keyframe_kelvin);
        MENU_SET_ENABLED(TRUE);
    }
    else
    {
        MENU_SET_VALUE("OFF");
        MENU_SET_ENABLED(FALSE);
    }
}

static MENU_UPDATE_FUNC(loop_after_menu_update)
{
    if(adv_int_use_global_time)
    {
        MENU_SET_VALUE("OFF");
        MENU_SET_ENABLED(FALSE);
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "This feature does not work with global time keyframes");
    }
    else if(!adv_int_loop_after)
    {
        MENU_SET_VALUE("OFF");
        MENU_SET_ENABLED(FALSE);
    }
    else
    {
        MENU_SET_ENABLED(TRUE);
    }
    
}

static void step_focus(int mf_steps)
{
    if (mf_steps && !is_manual_focus())
    {
        while (lens_info.job_state) msleep(100);
        msleep(300);
        get_out_of_play_mode(500);
        if (!lv)
        {
            msleep(500);
            if (!lv) force_liveview();
        }
        set_lv_zoom(5);
        msleep(1000);
        NotifyBox(1000, "Focusing...");
        lens_focus_enqueue_step(-mf_steps);
        msleep(1000);
        set_lv_zoom(1);
        msleep(500);
    }
}

static int current_focus_offset = 0;
static void focus_to(int offset)
{
    if(offset - current_focus_offset != 0)
    {
        NotifyBox(2000, "Focusing %d steps to offset %d",offset - current_focus_offset, offset);
        step_focus(offset - current_focus_offset);
        current_focus_offset = offset;
    }
}

static int compute_ramp(int start_val, int end_val, int start_time, int end_time, int time)
{
    return (time - start_time) * (end_val - start_val) / (end_time - start_time) + start_val;
}

static unsigned int adv_int_cbr()
{
    if(adv_int && keyframes)
    {
        int current_time = adv_int_external ? adv_int_external_pic_count++ : get_interval_count();
        if(!adv_int_use_global_time && adv_int_loop_after)
            current_time = (current_time - 1) % adv_int_loop_after + 1;
        //the sequence just started
        if(current_time == 1)
        {
            current_focus_offset = 0;
            current_keyframe = keyframes;
            last_keyframe = keyframes;
        }
        if(adv_int_use_global_time)
        {
            if(current_time == 1)
            {
                current_time = get_global_time();
                //we need to search the keyframe list in case we start at a time past the first keyframe
                while(current_keyframe && current_time > current_keyframe->time)
                {
                    last_keyframe = current_keyframe;
                    current_keyframe = current_keyframe->next;
                }
            }
            else
            {
                current_time = get_global_time();
            }
        }
        
        //we are inbetween keyframes so ramp from previous keyframe to the next
        if(last_keyframe && current_keyframe &&
           current_time >= last_keyframe->time)
        {
            int ramp_time = MIN(current_time, current_keyframe->time);
            NotifyBox(2000,"Ramping %d%%", compute_ramp(0, 100, last_keyframe->time,current_keyframe->time, ramp_time));
            //ramp values from previous keyframe to next keyframe
            if(current_keyframe->shutter && last_keyframe->shutter)
            {
                int computed = compute_ramp(last_keyframe->shutter, current_keyframe->shutter, last_keyframe->time,current_keyframe->time, ramp_time);
                lens_set_rawshutter(round_shutter(computed, 16));
            }
            if(current_keyframe->aperture && last_keyframe->aperture)
            {
                int computed = compute_ramp(last_keyframe->aperture, current_keyframe->aperture, last_keyframe->time,current_keyframe->time, ramp_time);
                lens_set_rawaperture(round_aperture(computed));
            }
            if(current_keyframe->iso && last_keyframe->iso)
            {
                int computed = compute_ramp(last_keyframe->iso, current_keyframe->iso, last_keyframe->time,current_keyframe->time, ramp_time);
                lens_set_rawiso(computed / 8 * 8); //round to nearest analog ISO
            }
            if(current_keyframe->focus != last_keyframe->focus)
            {
                int computed = compute_ramp(last_keyframe->focus, current_keyframe->focus, last_keyframe->time,current_keyframe->time, ramp_time);
                focus_to(computed);
            }
            if(current_keyframe->interval_time && last_keyframe->interval_time)
            {
                int computed = compute_ramp(last_keyframe->interval_time, current_keyframe->interval_time, last_keyframe->time,current_keyframe->time, ramp_time);
                set_config_var("interval.time", computed);
            }
            if(current_keyframe->kelvin && last_keyframe->kelvin)
            {
                int computed = compute_ramp(last_keyframe->kelvin, current_keyframe->kelvin, last_keyframe->time,current_keyframe->time, ramp_time);
                lens_set_kelvin(computed);
            }
            if(current_keyframe->bulb_duration && last_keyframe->bulb_duration)
            {
                int computed = compute_ramp(last_keyframe->bulb_duration, current_keyframe->bulb_duration, last_keyframe->time,current_keyframe->time, ramp_time);
                set_config_var("bulb.duration", computed);
            }
        }
        
        //we reached the keyframe so go to next
        while(current_keyframe && current_time >= current_keyframe->time)
        {
            last_keyframe = current_keyframe;
            current_keyframe = current_keyframe->next;
        }
    }
    return 0;
}

static int running = 0;

static void adv_int_task()
{
    running = 1;
    adv_int_cbr();
    running = 0;
}

PROP_HANDLER(PROP_GUI_STATE)
{
    int* data = buf;
    if (data[0] == GUISTATE_QR)
    {
        if (adv_int_external && !running)
            task_create("adv_int_task", 0x1c, 0x1000, adv_int_task, (void*)0);
    }
}

static struct menu_entry adv_int_menu[] =
{
    {
        .name = "Advanced Intervalometer",
        .priv = &adv_int,
        .select = menu_open_submenu,
        .max = 1,
        .works_best_in = DEP_M_MODE,
        .help = "Advanced intervalometer ramping",
        .children =  (struct menu_entry[])
        {
            {
                .name = "Enabled",
                .priv = &adv_int,
                .max = 1,
                .help = "Enable advanced intervalometer ramping"
            },
            {
                .name = "Use Global Time",
                .priv = &adv_int_use_global_time,
                .max = 1,
                .help = "Set keyframes by global time (time of day)",
                .help2 = "You should clear all keyframes when switching this setting"
            },
            {
                .name = "Loop After",
                .priv = &adv_int_loop_after,
                .update = loop_after_menu_update,
                .max = 5000,
                .icon_type = IT_BOOL,
                .help = "Loops keyframe sequence after x frames"
            },
            {
                .name = "External Source",
                .priv = &adv_int_external,
                .max = 1,
                .icon_type = IT_BOOL,
                .help = "Use this module with an external intervalometer"
            },
            {
                .name = "List Keyframes",
                .select = menu_open_submenu,
                .submenu_width = 710,
                .help = "Lists all keyframes",
                .children =  (struct menu_entry[]) {
                    MENU_EOL
                }
            },
            {
                .name = "Load...",
                .select = adv_int_load,
                .help = "Load keyframes from file"
            },
            {
                .name = "Save...",
                .select = adv_int_save,
                .help = "Save current keyframes to file"
            },
            {
                .name = "Clear",
                .select = adv_int_clear,
                .help = "Clears all keyframes"
            },
            {
                .name = "New Keyframe...",
                .select = new_keyframe_menu_select,
                .submenu_width = 710,
                .icon_type = IT_ACTION,
                .help = "Create a new keyframe from the current camera settings",
                .children =  (struct menu_entry[])
                {
                    {
                        .name = "Create Keyframe",
                        .select = adv_int_new_keyframe,
                        .help = "Create a new keyframe from the current camera settings"
                    },
                    {
                        .name = "Keyframe Time",
                        .priv = &keyframe_time,
                        .update = time_menu_update,
                        .min = 1,
                        .max = SECONDS_IN_DAY,
                        .unit = UNIT_DEC,
                        .help = "The frame at which to apply this keyframe",
                        .help2 = "* Computed time inacurate if ramping interval time"
                    },
                    {
                        .name = "Shutter ",
                        .priv = &keyframe_shutter,
                        .select = menu_open_submenu,
                        .update = shutter_menu_update,
                        .max = 1,
                        .icon_type = IT_BOOL,
                        .works_best_in = DEP_M_MODE,
                        .help = "Include current Shutter in Keyframe",
                        .children = (struct menu_entry[])
                        {
                            {
                                .name = "Enabled",
                                .priv = &keyframe_shutter,
                                .max = 1,
                                .icon_type = IT_BOOL,
                                .help = "Include current Shutter in Keyframe"
                            },
                            {
                                .name = "Adjust Shutter",
                                .update     = shutter_display,
                                .select     = shutter_toggle,
                                .icon_type  = IT_PERCENT,
                                .help = "Fine-tune shutter value. Displays APEX Tv or degrees equiv.",
                                .edit_mode = EM_MANY_VALUES_LV,
                            },
                            MENU_EOL
                        }
                    },
                    {
                        .name = "Aperture ",
                        .priv = &keyframe_aperture,
                        .select = menu_open_submenu,
                        .update = aperture_menu_update,
                        .max = 1,
                        .icon_type = IT_BOOL,
                        .works_best_in = DEP_M_MODE,
                        .help = "Include current Aperture in Keyframe",
                        .children = (struct menu_entry[])
                        {
                            {
                                .name = "Enabled",
                                .priv = &keyframe_aperture,
                                .max = 1,
                                .icon_type = IT_BOOL,
                                .help = "Include current Aperture in Keyframe"
                            },
                            {
                                .name = "Adjust Aperture",
                                .update     = aperture_display,
                                .select     = aperture_toggle,
                                .icon_type  = IT_PERCENT,
                                .help = "Adjust aperture. Also displays APEX aperture (Av) in stops.",
                                .depends_on = DEP_CHIPPED_LENS,
                                .edit_mode = EM_MANY_VALUES_LV,
                            },
                            MENU_EOL
                        }
                    },
                    {
                        .name = "ISO ",
                        .priv = &keyframe_iso,
                        .select = menu_open_submenu,
                        .update = iso_menu_update,
                        .max = 1,
                        .icon_type = IT_BOOL,
                        .help = "Include current ISO in Keyframe",
                        .children = (struct menu_entry[])
                        {
                            {
                                .name = "Enabled",
                                .priv = &keyframe_iso,
                                .max = 1,
                                .icon_type = IT_BOOL,
                                .help = "Include current ISO in Keyframe"
                            },
                            {
                                .name = "Adjust ISO",
                                .update = iso_display,
                                .select = iso_toggle,
                                .help  = "Adjust and fine-tune ISO. Also displays APEX Sv value.",
                                .edit_mode = EM_MANY_VALUES_LV,
                            },
                            MENU_EOL
                        }
                    },
                    {
                        .name = "Focus",
                        .priv = &keyframe_focus,
                        .min = -5000,
                        .max = 5000,
                        .unit = UNIT_DEC,
                        .help = "The focus offset in steps",
                        .depends_on = DEP_AUTOFOCUS,
                        .works_best_in = DEP_LIVEVIEW,
                    },
                    {
                        .name = "Interval Time",
                        .priv = &keyframe_interval_time,
                        .min = 0,
                        .max = 28800,
                        .unit = UNIT_TIME,
                        .help = "Changes the interval between shots",
                    },
                    {
                        .name = "Bulb Duration",
                        .priv = &keyframe_bulb_duration,
                        .min = 0,
                        .max = 28800,
                        .unit = UNIT_TIME,
                        .depends_on = DEP_PHOTO_MODE,
                        .help = "Changes duration of the bulb timer",
                    },
                    {
                        .name = "White Balance",
                        .update = &kelvin_menu_update,
                        .select = &kelvin_menu_select,
                        .priv = &keyframe_kelvin,
                        .help = "Changes the Kelvin white balance",
                    },
                    MENU_EOL,
                }
            },
            MENU_EOL
        }
    }
};

static unsigned int adv_int_init()
{
    menu_add("Intervalometer", adv_int_menu, COUNT(adv_int_menu));
    return 0;
}

static unsigned int adv_int_deinit()
{
    return 0;
}

MODULE_CBRS_START()
    MODULE_CBR(CBR_INTERVALOMETER, adv_int_cbr, 0)
MODULE_CBRS_END()

MODULE_INFO_START()
    MODULE_INIT(adv_int_init)
    MODULE_DEINIT(adv_int_deinit)
MODULE_INFO_END()

MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_GUI_STATE)
MODULE_PROPHANDLERS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(adv_int)
    MODULE_CONFIG(adv_int_use_global_time)
    MODULE_CONFIG(adv_int_loop_after)
    MODULE_CONFIG(adv_int_external)
MODULE_CONFIGS_END()