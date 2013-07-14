#include "dryos.h"

#include "menu.h"
#include "property.h"

#ifdef FEATURE_CROP_MODE_HACK

int video_mode[6];
PROP_HANDLER(PROP_VIDEO_MODE)
{
    memcpy(video_mode, buf, 20);
}

unsigned int is_crop_hack_supported() {
    if(recording || video_mode_resolution != 0) {
         return 0;
    }
    return 1;
}

void movie_crop_hack_enable() {
    int video_mode_patched[6];

    if(recording || video_mode_resolution != 0 || video_mode_crop) {
        return;
    }
    video_mode[0] = 0xc;
    video_mode[4] = 2;
    prop_request_change(PROP_VIDEO_MODE, video_mode, 0);
    NotifyBox(1000,"[ON ] DONE");
}


void movie_crop_hack_disable() {
    if(recording || video_mode_resolution != 0 || !video_mode_crop) {
        return;
    }
    video_mode[0] = 0;
    video_mode[4] = 0;
    prop_request_change(PROP_VIDEO_MODE, video_mode, 0);
}


static void movie_crop_hack_toggle(void* priv, int sign)
{
    if(!video_mode_crop) {
        movie_crop_hack_enable();
    } else {
        movie_crop_hack_disable();
    }
}

static MENU_UPDATE_FUNC(movie_crop_hack_display)
{
    if(recording) {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "You can't change crop mode while recording");
    } else if(video_mode_resolution != 0) {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Crop video mode works in 1080p only");
    }
    MENU_SET_VALUE(video_mode_crop?"ON":"OFF");
}


static struct menu_entry crop_hack_menus[] = {
    {
        .name = "Movie crop mode",
        .update = movie_crop_hack_display,
        .select = movie_crop_hack_toggle,
        .max = 1,
        .choices = (const char *[]) {"OFF", "ON"},
        .help   = "Enables 600D movie crop-mode",
        .depends_on = DEP_MOVIE_MODE,
    },
};

void crop_mode_hack_init()
{
    menu_add( "Movie", crop_hack_menus, COUNT(crop_hack_menus) );
}

#endif
