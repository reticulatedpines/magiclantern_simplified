// included in zebra.c
// always use cropmarks in plural

static void bvram_mirror_clear();
static void clrscr_mirror();

static void cropmark_cache_update_signature();
static int cropmark_cache_is_valid();
static void default_movie_cropmarks();
static void black_bars_16x9();
static void black_bars();

void cropmark_clear_cache();

static struct bmp_file_t * cropmarks = 0;

static CONFIG_INT( "crop.enable",   crop_enabled,   0 ); // index of crop file
static CONFIG_INT( "crop.index",    crop_index, 0 ); // index of crop file
static CONFIG_INT( "crop.movieonly", cropmark_movieonly, 0);
static CONFIG_INT("crop.playback", cropmarks_play, 0);

static int cropmark_cache_dirty = 1;
static int crop_dirty = 0;       // redraw cropmarks after some time (unit: 0.1s)

void crop_set_dirty(int value)
{
    crop_dirty = MAX(crop_dirty, value);
}

#ifdef FEATURE_CROPMARKS

#define MAX_CROP_NAME_LEN 15
#define MAX_CROPMARKS 9
static int num_cropmarks = 0;
static int cropmarks_initialized = 0;
static char cropmark_names[MAX_CROPMARKS][MAX_CROP_NAME_LEN];

// Cropmark sorting code contributed by Nathan Rosenquist
static void sort_cropmarks()
{
    int i = 0;
    int j = 0;
    
    char aux[MAX_CROP_NAME_LEN];
    
    for (i=0; i<num_cropmarks; i++)
    {
        for (j=i+1; j<num_cropmarks; j++)
        {
            if (strcmp(cropmark_names[i], cropmark_names[j]) > 0)
            {
                strcpy(aux, cropmark_names[i]);
                strcpy(cropmark_names[i], cropmark_names[j]);
                strcpy(cropmark_names[j], aux);
            }
        }
    }
}
#endif

int is_valid_cropmark_filename(char* filename)
{
    int n = strlen(filename);
    if ((n > 4) && (streq(filename + n - 4, ".BMP") || streq(filename + n - 4, ".bmp")) && (filename[0] != '.') && (filename[0] != '_'))
        return 1;
    return 0;
}

#ifdef FEATURE_CROPMARKS
static void find_cropmarks()
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( CARD_DRIVE "ML/CROPMKS/", &file );
    if( IS_ERROR(dirent) )
    {
        NotifyBox(2000, "ML/CROPMKS dir missing\n"
                        "Please copy all ML files!" );
        return;
    }
    int k = 0;
    do {
        if (file.mode & ATTR_DIRECTORY) continue; // is a directory
        if (is_valid_cropmark_filename(file.name))
        {
            if (k >= MAX_CROPMARKS)
            {
                NotifyBox(2000, "TOO MANY CROPMARKS (max=%d)", MAX_CROPMARKS);
                break;
            }
            snprintf(cropmark_names[k], MAX_CROP_NAME_LEN, "%s", file.name);
            k++;
        }
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_FindClose(dirent);
    num_cropmarks = k;
    sort_cropmarks();
    cropmarks_initialized = 1;
}

static void reload_cropmark()
{
    int i = crop_index;
    static int old_i = -1;
    if (i == old_i) return; 
    old_i = i;
    //~ bmp_printf(FONT_LARGE, 0, 100, "reload crop: %d", i);

    if (cropmarks)
    {
        void* old_crop = cropmarks;
        cropmarks = 0;
        bmp_free(old_crop);
    }

    cropmark_clear_cache();
    
    if (!num_cropmarks) return;
    i = COERCE(i, 0, num_cropmarks-1);
    char bmpname[100];
    snprintf(bmpname, sizeof(bmpname), CARD_DRIVE "ML/CROPMKS/%s", cropmark_names[i]);
    cropmarks = bmp_load(bmpname,1);
    if (!cropmarks) bmp_printf(FONT_LARGE, 0, 50, "LOAD ERROR %d:%s   ", i, bmpname);
}

static void
crop_toggle( void* priv, int sign )
{
    crop_index = mod(crop_index + sign, num_cropmarks);
    //~ reload_cropmark(crop_index);
    crop_set_dirty(10);
}
#endif


#ifdef FEATURE_CROPMARKS
static MENU_UPDATE_FUNC(crop_display)
{
    int index = crop_index;
    index = COERCE(index, 0, num_cropmarks-1);
    if (crop_enabled) MENU_SET_VALUE(
         num_cropmarks ? cropmark_names[index] : "N/A"
    );
    if (cropmark_movieonly && !is_movie_mode())
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Cropmarks are configured only for movie mode");
}

static MENU_UPDATE_FUNC(crop_display_submenu)
{
    int index = crop_index;
    index = COERCE(index, 0, num_cropmarks-1);
    MENU_SET_NAME(
        "Bitmap (%d/%d)",
         index+1, num_cropmarks
    );
    MENU_SET_VALUE(
        "%s",
         num_cropmarks ? cropmark_names[index] : "N/A"
    );

    if (info->can_custom_draw)
    {
        int h = 170;
        int w = h * 720 / 480;
        //~ int xc = 360 - w/2;
        int xc = 400;
        int yc = info->y + font_large.height * 3 + 10;
        BMP_LOCK( reload_cropmark(); )
        bmp_fill(0, xc, yc, w, h);
        BMP_LOCK( bmp_draw_scaled_ex(cropmarks, xc, yc, w, h, 0); )
        bmp_draw_rect(COLOR_WHITE, xc, yc, w, h);
    }
    
    MENU_SET_ICON(MNI_DICE, (num_cropmarks<<16) + index);
}

static struct menu_entry cropmarks_menu[] = {
    {
        .name = "Cropmarks",
        .priv = &crop_enabled,
        .update    = crop_display,
        .max = 1,
        .help = "Cropmarks or custom grids for framing.",
        .depends_on = DEP_GLOBAL_DRAW,
        .submenu_width = 710,
        .submenu_height = 250,
        .children =  (struct menu_entry[]) {
            {
                .name = "Bitmap",
                .priv = &crop_index, 
                .select = crop_toggle,
                .update    = crop_display_submenu,
                .icon_type = IT_DICE,
                .help = "You can draw your own cropmarks in Paint.",
            },
            {
                .name = "Show in photo mode",
                .priv = &cropmark_movieonly, 
                .max = 1,
                .choices = (const char *[]) {"ON", "OFF"},
                .help = "Cropmarks are mostly used in movie mode.",
            },
            {
                .name = "Show in PLAY mode",
                .priv = &cropmarks_play, 
                .max = 1,
                .help = "You may also have cropmarks in Playback mode.",
            },
            MENU_EOL
        },
    },
};

static void cropmark_draw_from_cache()
{
    uint8_t* B = bmp_vram();
    uint8_t* M = get_bvram_mirror();
    get_yuv422_vram();
    ASSERT(B);
    ASSERT(M);
    
    for (int i = os.y0; i < os.y_max; i++)
    {
        for (int j = os.x0; j < os.x_max; j++)
        {
            uint8_t p = B[BM(j,i)];
            uint8_t m = M[BM(j,i)];
            if (!(m & 0x80)) continue;
            if (p != 0 && p != 0x14 && p != 0x3 && p != m) continue;
            B[BM(j,i)] = m & ~0x80;
            #ifdef CONFIG_500D
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
            #endif
        }
    }
}
#endif

void cropmark_clear_cache()
{
#ifdef FEATURE_CROPMARKS
    BMP_LOCK(
        clrscr_mirror();
        bvram_mirror_clear();
        default_movie_cropmarks();
    )
#endif
}

#ifdef FEATURE_CROPMARKS
static void 
cropmark_draw()
{
    if (!get_global_draw()) return;

    get_yuv422_vram(); // just to refresh VRAM params
    clear_lv_afframe_if_dirty();

    #ifdef FEATURE_GHOST_IMAGE
    if (transparent_overlay && !transparent_overlay_hidden && !PLAY_MODE)
    {
        show_overlay();
        zoom_overlay_dirty = 1;
        cropmark_cache_dirty = 1;
    }
    #endif
    crop_dirty = 0;

    reload_cropmark(); // reloads only when changed

    // this is very fast
    if (cropmark_cache_is_valid())
    {
        clrscr_mirror();
        cropmark_draw_from_cache();
        //~ bmp_printf(FONT_MED, 50, 50, "crop cached");
        //~ info_led_blink(5, 10, 10);
        goto end;
    }

    if (
            (!crop_enabled) ||
            (cropmark_movieonly && !is_movie_mode() && !PLAY_OR_QR_MODE)
       )
    {
        // Cropmarks disabled (or not shown in this mode)
        // Generate and draw default cropmarks
        cropmark_cache_update_signature();
        cropmark_clear_cache();
        cropmark_draw_from_cache();
        //~ info_led_blink(5,50,50);
        goto end;
    }
    
    if (cropmarks) 
    {
        // Cropmarks enabled, but cache is not valid
        if (!lv) msleep(500); // let the bitmap buffer settle, otherwise ML may see black image and not draw anything (or draw half of cropmark)
        clrscr_mirror(); // clean any remaining zebras / peaking
        cropmark_cache_update_signature();
        
        if (hdmi_code == 5 && is_pure_play_movie_mode())
        {   // exception: cropmarks will have some parts of them outside the screen
            bmp_draw_scaled_ex(cropmarks, BMP_W_MINUS+1, BMP_H_MINUS - 50, 960, 640, bvram_mirror);
        }
        else
            bmp_draw_scaled_ex(cropmarks, os.x0, os.y0, os.x_ex, os.y_ex, bvram_mirror);
        //~ info_led_blink(5,50,50);
        //~ bmp_printf(FONT_MED, 50, 50, "crop regen");
        goto end;
    }

end:
    cropmark_cache_dirty = 0;
    zoom_overlay_dirty = 1;
    crop_dirty = 0;
}

static int cropmark_cache_sig = 0;
static int cropmark_cache_get_signature()
{
    get_yuv422_vram(); // update VRAM params if needed
    int sig = 
        crop_index * 13579 + crop_enabled * 14567 +
        os.x0*811 + os.y0*467 + os.x_ex*571 + os.y_ex*487 + (is_movie_mode() ? 113 : 0) + video_mode_resolution * 8765;
    return sig;
}

static void cropmark_cache_update_signature()
{
    cropmark_cache_sig = cropmark_cache_get_signature();
}

static int cropmark_cache_is_valid()
{
    if (cropmark_cache_dirty) return 0; // some other ML task asked for redraw
    if (hdmi_code == 5 && PLAY_MODE) return 0; // unusual geometry - better force full redraw every time
    
    int sig = cropmark_cache_get_signature(); // video mode changed => needs redraw
    if (cropmark_cache_sig != sig) return 0;
    
    return 1; // everything looks alright
}

static void
cropmark_redraw()
{
    if (!cropmarks_initialized) return;
    if (gui_menu_shown()) return; 
    if (!zebra_should_run() && !PLAY_OR_QR_MODE) return;
    if (digic_zoom_overlay_enabled()) return;
    BMP_LOCK(
        if (!cropmark_cache_is_valid())
        {
            cropmark_clear_cache();
        }
        cropmark_draw(); 
    )
}
#endif

#if 0
void draw_cropmark_area()
{
    get_yuv422_vram();
    bmp_draw_rect(COLOR_BLUE, os.x0, os.y0, os.x_ex, os.y_ex);
    draw_line(os.x0, os.y0, os.x_max, os.y_max, COLOR_BLUE);
    draw_line(os.x0, os.y_max, os.x_max, os.y0, COLOR_BLUE);
    
    bmp_draw_rect(COLOR_RED, HD2BM_X(0), HD2BM_Y(0), HD2BM_DX(vram_hd.width), HD2BM_DY(vram_hd.height));
    draw_line(HD2BM_X(0), HD2BM_Y(0), HD2BM_X(vram_hd.width), HD2BM_Y(vram_hd.height), COLOR_RED);
    draw_line(HD2BM_X(0), HD2BM_Y(vram_hd.height), HD2BM_X(vram_hd.width), HD2BM_Y(0), COLOR_RED);
}
#endif

/*
void show_apsc_crop_factor()
{
    int x_ex_crop = os.x_ex * 10/16;
    int y_ex_crop = os.y_ex * 10/16;
    int x_off = (os.x_ex - x_ex_crop)/2;
    int y_off = (os.y_ex - y_ex_crop)/2;
    bmp_draw_rect(COLOR_WHITE, os.x0 + x_off, os.y0 + y_off, x_ex_crop, y_ex_crop);
    bmp_draw_rect(COLOR_BLACK, os.x0 + x_off + 1, os.y0 + y_off + 1, x_ex_crop - 2, y_ex_crop - 2);
}
*/

static void black_bars()
{
    if (!get_global_draw()) return;
    if (!is_movie_mode()) return;
    if (video_mode_resolution > 1) return; // these are only for 16:9
    int i,j;
    uint8_t * const bvram = bmp_vram();
    get_yuv422_vram();
    ASSERT(bvram);
    for (i = os.y0; i < MIN(os.y_max+1, BMP_H_PLUS); i++)
    {
        if (i < os.y0 + os.off_169 || i > os.y_max - os.off_169)
        {
            int newcolor = (i < os.y0 + os.off_169 - 2 || i > os.y_max - os.off_169 + 2) ? COLOR_BLACK : COLOR_BG;
            for (j = os.x0; j < os.x_max; j++)
            {
                if (bvram[BM(j,i)] == COLOR_BG)
                    bvram[BM(j,i)] = newcolor;
            }
        }
    }
}

static void default_movie_cropmarks()
{
    if (!get_global_draw()) return;
    if (!lv) return;
    if (!is_movie_mode()) return;
    if (video_mode_resolution > 1) return; // these are only for 16:9
    int i,j;
    uint8_t * const bvram_mirror = get_bvram_mirror();
    get_yuv422_vram();
    ASSERT(bvram_mirror);
    for (i = os.y0; i < MIN(os.y_max+1, BMP_H_PLUS); i++)
    {
        if (i < os.y0 + os.off_169 || i > os.y_max - os.off_169)
        {
            int newcolor = (i < os.y0 + os.off_169 - 2 || i > os.y_max - os.off_169 + 2) ? COLOR_BLACK : COLOR_BG;
            for (j = os.x0; j < os.x_max; j++)
            {
                bvram_mirror[BM(j,i)] = newcolor | 0x80;
            }
        }
    }
}

static void black_bars_16x9()
{
#ifdef CONFIG_KILL_FLICKER
    if (!get_global_draw()) return;
    if (!is_movie_mode()) return;
    get_yuv422_vram();
    if (video_mode_resolution > 1)
    {
        bmp_fill(COLOR_BLACK, os.x0, os.y0, os.off_43, os.y_ex);
        bmp_fill(COLOR_BLACK, os.x_max - os.off_43, os.y0, os.off_43, os.y_ex);
    }
    else
    {
        bmp_fill(COLOR_BLACK, os.x0, os.y0, os.x_ex, os.off_169);
        bmp_fill(COLOR_BLACK, os.x0, os.y_max - os.off_169, os.x_ex, os.off_169);
    }
#endif
}
