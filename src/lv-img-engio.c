/** 
 * Experiments on LiveView, engio, registers that alter recorded image...
 * 
 **/

#include "dryos.h"
#include "bmp.h"
#include "property.h"
#include "lens.h"
#include "menu.h"
#include "config.h"
#include "math.h"

//~ #define EngDrvOutLV(reg, value) *(int*)(reg) = value

//~ #define LV_PAUSE_REGISTER 0xC0F08000 // writing to this pauses LiveView cleanly => good for silent pics

#define SHAD_GAIN      0xc0f08030       // controls clipping point (digital ISO)
#define SHAD_PRESETUP  0xc0f08034       // controls black point? as in "dcraw -k"
#define ISO_PUSH_REGISTER 0xc0f0e0f8    // like display gain, 0x100 = 1 stop, 0x700 = max of 7 stops

#define SHADOW_LIFT_REGISTER_1 0xc0f0e094 // raises shadows, but after they are crushed by Canon curves; default at 0x80?
#define SHADOW_LIFT_REGISTER_2 0xc0f0e0f0 // raises shadows, seems to bring back some shadow detail
#define SHADOW_LIFT_REGISTER_3 0xc0f0f1c4 // raises shadows; has discontinuity :(
#define SHADOW_LIFT_REGISTER_4 0xc0f0f43c // ugly...
#define SHADOW_LIFT_REGISTER_5 0xc0f0e054 // side effect: weird artifacts in highlight
#define SHADOW_LIFT_REGISTER_6 0xc0f0e084 // side effect: weird artifacts in highlight
#define SHADOW_LIFT_REGISTER_7 0xc0f0f178
#define SHADOW_LIFT_REGISTER_8 0xc0f0ecf8 // more like ISO control (clips whites)

static CONFIG_INT("digic.iso.gain.movie", digic_iso_gain_movie, 0); // units: like with the old display gain
static CONFIG_INT("digic.iso.gain.photo", digic_iso_gain_photo, 0);

#define DIGIC_ISO_GAIN_MOVIE (digic_iso_gain_movie ? digic_iso_gain_movie : 1024)
#define DIGIC_ISO_GAIN_PHOTO (digic_iso_gain_photo ? digic_iso_gain_photo : 1024)

int get_digic_iso_gain_movie() { return DIGIC_ISO_GAIN_MOVIE; }
int get_digic_iso_gain_photo() { return DIGIC_ISO_GAIN_PHOTO; }

static CONFIG_INT("digic.black.level", digic_black_level, 0);
int digic_iso_gain_movie_for_gradual_expo = 1024; // additional gain that won't appear in ML menus, but can be changed from code (to be "added" to digic_iso_gain_movie)
int digic_iso_gain_photo_for_bv = 1024;

int display_gain_menu_index = 0; // for menu

//~ CONFIG_INT("digic.shadow.lift", digic_shadow_lift, 0);
// that is: 1024 = 0 EV = disabled
// 2048 = 1 EV etc

// 1024 = 0 EV
void set_display_gain_equiv(int gain)
{
    if (gain == 1024) gain = 0;
    if (is_movie_mode()) digic_iso_gain_movie = gain;
    else digic_iso_gain_photo = gain;
}

void set_movie_digital_iso_gain(int gain)
{
    if (gain == 1024) gain = 0;
    digic_iso_gain_movie = gain;
}

void set_movie_digital_iso_gain_for_gradual_expo(int gain)
{
    digic_iso_gain_movie_for_gradual_expo = gain;
}

void set_photo_digital_iso_gain_for_bv(int gain)
{
    digic_iso_gain_photo_for_bv = gain;
}

int gain_to_ev_scaled(int gain, int scale)
{
    if (gain == 0) return 0;
    return (int) roundf(log2f(gain) * ((float)scale));
}


MENU_UPDATE_FUNC(digic_iso_print_movie)
{
    int G = 0;
    G = gain_to_ev_scaled(DIGIC_ISO_GAIN_MOVIE, 8) - 80;
    G = G * 10/8;
    int GA = abs(G);
    
    MENU_SET_VALUE(
        "%s%d.%d EV",
        G > 0 ? "+" : G < 0 ? "-" : "",
        GA/10, GA%10
    );
    MENU_SET_ENABLED(G);
    
    // ugly hack
    entry->priv = &digic_iso_gain_movie;
}

MENU_UPDATE_FUNC(display_gain_print)
{
    int G = gain_to_ev_scaled(DIGIC_ISO_GAIN_PHOTO, 8) - 80;
    G = G * 10/8;
    int GA = abs(G);
    display_gain_menu_index = GA/10;
}

/*
MENU_UPDATE_FUNC(digic_iso_print)
{
    if (is_movie_mode())
    {
        MENU_SET_NAME("ML digital ISO");
        digic_iso_print_movie(entry, info);
    }
    else
    {
        MENU_SET_NAME("LV Display Gain");
        display_gain_print(entry, info);
    }
}
*/

static MENU_UPDATE_FUNC(digic_black_print)
{
    MENU_SET_VALUE(
        "%s%d",
        digic_black_level > 0 ? "+" : "",
        digic_black_level
    );
    MENU_SET_ENABLED(digic_black_level);
}

static int digic_iso_presets[] = {256, 362, 512, 609, 664, 724, 790, 861, 939, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};

// for debugging
/*static int digic_iso_presets[] = {256, 362, 512, 609, 664, 724, 790, 861, 939, 
    1024, 1117, 1218, 1328, 1448, 1579, 1722, 1878,
    2048, 2233, 2435, 2656, 2896, 3158, 3444, 3756,
    4096, 4467, 4871, 5312, 5793, 6317, 6889, 7512,
    8192, 16384, 32768, 65536, 131072};
*/

void digic_iso_or_gain_toggle(int* priv, int delta)
{
    int mv = (priv == (int*)&digic_iso_gain_movie);
    
    if (*priv == 0) *priv = 1024;
    
    int i;
    for (i = 0; i < COUNT(digic_iso_presets); i++)
        if (digic_iso_presets[i] >= *priv) break;
    
    do {
        i = mod(i + delta, COUNT(digic_iso_presets));
    } while ((!mv && digic_iso_presets[i] < 1024)
    #ifdef CONFIG_DIGIC_V
    || (mv && digic_iso_presets[i] > 2048) // high display gains not working
    #endif
    || (!mv && digic_iso_presets[i] > 65536)
    );
    
    *priv = digic_iso_presets[i];
    if (*priv == 1024) *priv = 0;
}

void digic_iso_toggle(void* priv, int delta)
{
    if (is_movie_mode()) priv = &digic_iso_gain_movie;
    else priv = &digic_iso_gain_photo;
    digic_iso_or_gain_toggle(priv, delta);
}

void display_gain_toggle(void* priv, int delta)
{
    priv = &digic_iso_gain_photo;
    digic_iso_or_gain_toggle(priv, delta);
}

void digic_iso_toggle_movie(void* priv, int delta)
{
    priv = &digic_iso_gain_movie;
    digic_iso_or_gain_toggle(priv, delta);
}

//~ static CONFIG_INT("digic.effects", image_effects, 0);
static CONFIG_INT("digic.desaturate", desaturate, 0);
static CONFIG_INT("digic.negative", negative, 0);
static CONFIG_INT("digic.swap-uv", swap_uv, 0);
static CONFIG_INT("digic.cartoon", cartoon, 0);
static CONFIG_INT("digic.oilpaint", oilpaint, 0);
static CONFIG_INT("digic.sharp", sharp, 0);
static CONFIG_INT("digic.zerosharp", zerosharp, 0);
//~ static CONFIG_INT("digic.fringing", fringing, 0);


static int default_white_level = 4096;
static int shad_gain_last_written = 0;

static void autodetect_default_white_level()
{
    if (!lv) return;
    
    int current_shad_gain = (int) MEMX(SHAD_GAIN);
    if (current_shad_gain == shad_gain_last_written) return; // in the register there's the value we wrote => not good for computing what Canon uses as default setting

    default_white_level = current_shad_gain;
}

// get digic ISO level for movie mode
// use SHAD_GAIN as much as possible (range: 0-8191)
// if out of range, return a number of integer stops for boosting the ISO via ISO_PUSH_REGISTER and use SHAD_GAIN for the remainder
static int get_new_white_level(int movie_gain, int* boost_stops)
{
    int result = default_white_level;
    *boost_stops = 0;
    while (1)
    {
        result = default_white_level * COERCE(movie_gain, 0, 65536) / 1024;
        #ifdef CONFIG_DIGIC_V
        break;
        #endif
        if (result > 8192 && *boost_stops < 7) 
        { 
            movie_gain /= 2; 
            (*boost_stops)++;
        }
        else break;
    }
    return COERCE(result, 0, 8191);
}

#ifdef CONFIG_DIGIC_POKE

static CONFIG_INT("digic.poke", digic_poke, 0);
static CONFIG_INT("digic.reg.bas", digic_register_base, 0xC0F0);
static CONFIG_INT("digic.reg.mid", digic_register_mid, 0x80);
static CONFIG_INT("digic.reg.off", digic_register_off, 0x08);
static CONFIG_INT("digic.alter.mode", digic_alter_mode, 1);
int digic_register = 0;
int digic_value = 0;

int get_digic_register_addr()
{
    return ((digic_register_base << 16) & 0xFFFF0000) |
           ((digic_register_mid  <<  8) & 0x0000FF00) |
           ((digic_register_off  <<  0) & 0x000000FC) ;
}

void digic_show()
{
    NotifyBox(2000, "%x: %8x          \n"
                    "= %d             \n"
                    "= %d %d          \n"
                    "= %d %d %d %d      ",
                    digic_register, digic_value,
                    digic_value,
                    digic_value >> 16, (int16_t)digic_value,
                    (int8_t)(digic_value >> 24), (int8_t)(digic_value >> 16), (int8_t)(digic_value >> 8), (int8_t)(digic_value >> 0)
            );
}

void update_digic_register_addr(int dr, int delta, int skip_zero)
{
    while (1)
    {
        dr += delta;
        digic_register_base = (dr & 0xFFFF0000) >> 16;
        digic_register_mid  = (dr & 0x0000FF00) >> 8;
        digic_register_off  = (dr & 0x000000FC) >> 0;
        digic_register = get_digic_register_addr();

        if (!skip_zero) break;

        int value = MEMX(digic_register);
        if (value != 0) 
            break; // stop on first non-zero register
    }

    digic_value = MEMX(digic_register);
    if ((digic_value & 0xFFF) == 0x800) beep();
    digic_show();
}

void digic_find_lv_buffer(int dr, int delta)
{
    for (int i = 0; i < 0x1000; i+=4)
    {
        dr += delta;
        digic_register_base = (dr & 0xFFFF0000) >> 16;
        digic_register_mid  = (dr & 0x0000FF00) >> 8;
        digic_register_off  = (dr & 0x000000FC) >> 0;
        digic_register = get_digic_register_addr();

        if ((MEMX(digic_register) & 0xFFF) == 0x800) break;
    }

    digic_value = MEMX(digic_register);
}

int handle_digic_poke(struct event * event)
{
    if (digic_poke && lv && !gui_menu_shown())
    {
        if (event->param == BGMT_PRESS_LEFT)
        {
            update_digic_register_addr(digic_register, -4, 0);
            return 0;
        }
        if (event->param == BGMT_PRESS_RIGHT)
        {
            update_digic_register_addr(digic_register, 4, 0);
            return 0;
        }
        if (event->param == BGMT_PRESS_DOWN)
        {
            update_digic_register_addr(digic_register, -4, 1);
            return 0;
        }
        if (event->param == BGMT_PRESS_UP)
        {
            update_digic_register_addr(digic_register, 4, 1);
            return 0;
        }
    }
    return 1;
}

void digic_poke_step()
{
    if (digic_poke && DISPLAY_IS_ON && lv)
    {
        digic_register = get_digic_register_addr();

        if (HALFSHUTTER_PRESSED)
        {
            if (digic_alter_mode == 0) // rand
                digic_value = rand();
            else if (digic_alter_mode == 1) // increment 
                digic_value += is_manual_focus() ? 1 : -1;
            else if (digic_alter_mode == 2) // increment << 8
                digic_value += (is_manual_focus() ? 1 : -1) << 8;
            else if (digic_alter_mode == 3) // increment << 16
                digic_value += (is_manual_focus() ? 1 : -1) << 16;
            else if (digic_alter_mode == 4) // increment << 24
                digic_value += (is_manual_focus() ? 1 : -1) << 24;
            
            //~ digic_value--;
            digic_show();
            _EngDrvOut(digic_register, digic_value);
            _EngDrvOut(0xC0F06000, 1); // apply the change
            //~ fps_set_main_timer(digic_value);

            //~ EngDrvOutLV(0xc0f04a08, 0x6000080);
            
            //~ int lvw = MEMX(0xc0f04308);
            //~ int hdw = MEMX(0xc0f04208);
            //~ EngDrvOutLV(0xc0f04308, hdw);
            //~ EngDrvOutLV(0xc0f04208, lvw);
        }
        else
        {
            digic_value = MEMX(digic_register);
        }
    }
}

void hex_toggle_8bit(void* priv, int delta)
{
    MEM(priv) += 4 * delta;
    MEM(priv) &= 0xFF;
}

void digic_value_toggle(void* priv, int delta)
{
    digic_value += delta;
}

void digic_random_register(void* priv, int delta)
{
    digic_register_mid = rand() & 0xFF;
    digic_register_off = rand() & 0xFC;
}

static MENU_UPDATE_FUNC(digic_value_print)
{
    MENU_SET_NAME("Value[%08x]", digic_register);
    MENU_SET_VALUE("%x", digic_value);
}


void digic_dump()
{
    msleep(1000);


    static char log_filename[100];
    
    int log_number = 0;
    for (log_number = 0; log_number < 100; log_number++)
    {
        snprintf(log_filename, sizeof(log_filename), CARD_DRIVE "digic%02d.LOG", log_number);
        unsigned size;
        if( FIO_GetFileSize( log_filename, &size ) != 0 ) break;
        if (size == 0) break;
    }

    FILE* f = FIO_CreateFileEx(log_filename);
    
    for (uint32_t reg = 0xc0f00000; reg < 0xC0f40000; reg+=4)
    {
        int value = (int) shamem_read(reg);
        if (value && value != -1)
        {
            bmp_printf(FONT_LARGE, 50, 50, "%8x: %8x", reg, value);
            my_fprintf(f, "%8x: %8x\n", reg, value);
        }
    }
    FIO_CloseFile(f);
}

void digic_dump_h264()
{
    msleep(1000);
    FILE* f = FIO_CreateFileEx(CARD_DRIVE "ML/LOGS/h264.log");
    
    for (uint32_t reg = 0xc0e10000; reg < 0xC0f00000; reg+=4)
    {
        int value = MEM(reg);
        if (value && value != -1)
        {
            bmp_printf(FONT_LARGE, 50, 50, "%8x: %8x", reg, value);
            my_fprintf(f, "%8x: %8x\n", reg, value);
        }
    }
    FIO_CloseFile(f);
}

#endif // CONFIG_DIGIC_POKE

#ifdef CONFIG_FULLFRAME
#define VIGNETTING_MAX_INDEX 155 // 5D2
#else
#define VIGNETTING_MAX_INDEX 145 // 60D
#endif

static uint32_t vignetting_data[0x100];
static uint32_t vignetting_data_prep[0x80];
static int vignetting_correction_initialized = 0;

static CONFIG_INT("digic.vignetting.corr", vignetting_correction_enable, 0);
static CONFIG_INT("digic.vignetting.a", vignetting_correction_a, -10);
static CONFIG_INT("digic.vignetting.b", vignetting_correction_b, 0);
static CONFIG_INT("digic.vignetting.c", vignetting_correction_c, 0);



// index from 0 to 0x100, value from 0 to 1023
// indexes greater than VIGNETTING_MAX_INDEX fall outside the image area
static void vignetting_correction_set(int index, int value)
{
    vignetting_data[index & 0xFF] = value;
}

static int vignetting_correction_get(int index)
{
    return vignetting_data[index & 0xFF];
}

static void vignetting_correction_set_coeffs(int a, int b, int c)
{
    uint32_t index = 0;
    int min = 1023;
    int max = -1023;
    for(index = 0; index < 0x100; index++)
    {
        int t = COERCE(index * 1024 / VIGNETTING_MAX_INDEX, 0, 1024);
        int ts = (1024 - t);
        ts = ts * ts / 1024;
        ts = 1024 - ts;
        int t2 = t * t / 1024;
        int t4 = t2 * t2 / 1024;
        int v = ts * a / 10 + t * b / 10 + t4 * c / 10;
        min = MIN(min, v);
        max = MAX(max, v);
        vignetting_data[index] = v;
    }
    
    // normalize data so it fits into 0...1023
    int range = max - min;
    for(index = 0; index < 0x100; index++)
    {
        vignetting_data[index] -= min;
        
        if (range > 1023)
            vignetting_data[index] = vignetting_data[index] * 1023 / range;
        
        #ifdef CONFIG_DIGIC_V
        // prefer to use 512 (0EV) if no correction is needed
        vignetting_data[index] += MIN(512, 1023 - MIN(range, 1023));
        #endif
        
        // debug - for finding VIGNETTING_MAX_INDEX
        // vignetting_data[index] = (index > VIGNETTING_MAX_INDEX) ? 1023 : 0;
    }
    
    /* pre-calculate register values */
    for(index = 0; index < 0x80; index++)
    {
        #ifdef CONFIG_DIGIC_V
        // 0    -> pitch black
        // 256  -> -1EV (with pink highlights)
        // 512  -> 0 EV (no correction)
        // 1024 -> +1EV (blows hihglights)
        // let's map 0-1024 to -2...+2EV; stick to 0EV if no correction is needed
        int ev1 = vignetting_data[2*index] - 512;
        int val1 = 512 * powf(2, ev1 / 256.0);
        int ev2 = vignetting_data[2*index+1] - 512;
        int val2 = 512 * powf(2, ev2 / 256.0);
        uint32_t data = (val1 & 0xFFFF) | ((val2 & 0xFFFF) << 16);
        #else
        uint32_t data = (vignetting_data[2*index] & 0x03FF) | ((vignetting_data[2*index+1] & 0x03FF) << 10);
        #endif
        vignetting_data_prep[index] = data;
    }
    
#if defined(CONFIG_7D)
    /* send vignetting data to master */
    ml_rpc_send_vignetting(vignetting_data_prep, vignetting_correction_enable ? sizeof(vignetting_data_prep) : 0);
#endif
}


#if defined(CONFIG_7D)
/* 7D version doesnt rewrite digic registers, but updates canon's register value buffer which is held in LVMgr */
void vignetting_correction_apply_lvmgr(uint32_t *lvmgr)
{
    uint32_t index = 0;
    if(vignetting_correction_enable && lvmgr)
    {
        uint32_t *vign = &lvmgr[0x83];

        for(index = 0; index < 0x80; index++)
        {
            vign[index] = vignetting_data_prep[index];
        }
    }
}
#else
/* the other cameras rewrite digic registers */
void vignetting_correction_apply_regs()
{
    if (!is_movie_mode()) return;
    if (!DISPLAY_IS_ON && !recording) return;
    if (!lv) return;
    if (lv_paused) return;
    
    if (!vignetting_correction_initialized || !vignetting_correction_enable)
    {
        return;
    }
    
    #ifdef CONFIG_DIGIC_V
    for(uint32_t index = 0; index < COUNT(vignetting_data_prep); index++)
    {
        *(volatile uint32_t*)(0xC0F08D1C) = vignetting_data_prep[index];
        *(volatile uint32_t*)(0xC0F08D24) = vignetting_data_prep[index];
    }
    #else
    for(uint32_t index = 0; index < COUNT(vignetting_data_prep); index++)
    {
        *(volatile uint32_t*)(0xC0F08578) = index * 2;
        *(volatile uint32_t*)(0xC0F0857C) = vignetting_data_prep[index];
    }
    *(volatile uint32_t*)(0xC0F08578) = COUNT(vignetting_data_prep) * 2;
    #endif

}
#endif

extern void flip_zoom();

static void vignetting_correction_toggle(void* priv, int delta)
{
    uint32_t *state = (uint32_t *)priv;
    
    *state = !*state;
#if defined(CONFIG_7D)
    ml_rpc_send_vignetting(vignetting_data_prep, *state ? sizeof(vignetting_data_prep) : 0);
#endif
}

static void vignetting_coeff_toggle(void* priv, int delta)
{
    menu_numeric_toggle(priv, delta, -10, 10);

    vignetting_correction_set_coeffs(vignetting_correction_a, vignetting_correction_b, vignetting_correction_c);

#ifdef CONFIG_7D
    if (vignetting_correction_enable)
        ml_rpc_send_vignetting(vignetting_data_prep, sizeof(vignetting_data_prep));
#endif
}

static int vignetting_luma[0x100];
static uint16_t vignetting_luma_n[0x100];
static uint8_t vignetting_luma_max[0x100];
static uint8_t vignetting_luma_min[0x100];

static void vignetting_measure_luma(int samples)
{
    uint8_t* lv = vram_lv.vram;
    
    for (int r = 0; r < 0x100; r++)
    {
        vignetting_luma[r] = 0;
        vignetting_luma_n[r] = 0;
        vignetting_luma_max[r] = 0;
        vignetting_luma_min[r] = 255;
    }

    for (int i = 0; i < samples; i++)
    {
        int xc = os.x0 + os.x_ex/2;
        int yc = os.y0 + os.y_ex/2;
        int x = os.x0 + (rand() % os.x_ex);
        int y = os.y0 + (rand() % os.y_ex);
        
        int ya = y;
        #ifdef CONFIG_4_3_SCREEN
        ya = ((y - yc) * 9/8) + yc;
        #endif
        int r = sqrtf((x - xc) * (x - xc) + (ya - yc) * (ya - yc)) / 2;
        r = MIN(r, 216);
        int luma = lv[BM2LV(x,y)+1];
        vignetting_luma[r & 0xFF] += luma;
        vignetting_luma_n[r & 0xFF]++;
        vignetting_luma_min[r & 0xFF] = MIN(luma, vignetting_luma_min[r & 0xFF]);
        vignetting_luma_max[r & 0xFF] = MAX(luma, vignetting_luma_max[r & 0xFF]);
    }

    for (int r = 0; r < 0x100; r++)
        vignetting_luma[r] /= vignetting_luma_n[r];
}

static MENU_UPDATE_FUNC(vignetting_graphs_update)
{
    if (entry->selected && info->x)
    {
        int yb = 400;
        int xa = 720-218 - 70;
        //~ int yb = menu_active_but_hidden() ? 430 : 455;
        bmp_fill(COLOR_BLACK, xa, yb-128, 216, 128);
        bmp_draw_rect(COLOR_WHITE, xa-1, yb-129, 218, 130);

        vignetting_measure_luma(10000);

        for (int r = 0; r < 216; r++)
        {
            int luma = vignetting_luma[r];
            int luma_min = vignetting_luma_min[r];
            int luma_max = vignetting_luma_max[r];
            int x = xa + r;
            int y = yb - luma/2;
            draw_line(x, yb - luma_min/2, x, yb - luma_max/2, 45);
            bmp_putpixel(x, y, COLOR_WHITE);
        }

        int xb = 70;

        bmp_fill(COLOR_BLACK, xb, yb-128, 216, 128);
        bmp_draw_rect(COLOR_WHITE, xb-1, yb-129, 218, 130);
        int max = 0;
        for (int i = 0; i < VIGNETTING_MAX_INDEX; i++)
        {
            int x = xb + sqrtf(i) * 216 / sqrtf(VIGNETTING_MAX_INDEX);
            int y = yb - vignetting_data[i] / 8;
            bmp_putpixel(x, y, COLOR_WHITE);
            bmp_putpixel(x+1, y, COLOR_WHITE);
            max = MAX(max, vignetting_data[i]);
        }
        
        #ifdef CONFIG_DIGIC_V
        bmp_printf(FONT(FONT_MED, 60, COLOR_BLACK), xb + 225, yb-128 - font_med.height/2, "+2 EV");
        bmp_printf(FONT(FONT_MED, 60, COLOR_BLACK), xb + 225, yb - font_med.height/2, "-2 EV");
        #else
        bmp_printf(FONT(FONT_MED, 60, COLOR_BLACK), xb + 225, yb-128 - font_med.height/2, "+1 EV");
        bmp_printf(FONT(FONT_MED, 60, COLOR_BLACK), xb + 225, yb - font_med.height/2, "0");
        #endif
        
    }
    
    if (!vignetting_correction_enable)
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Vignetting correction is disabled.");
}

#ifdef FEATURE_EXTREME_SHUTTER_SPEEDS
    #ifndef CONFIG_FRAME_SHUTTER_OVERRIDE
    #error "This requires CONFIG_FRAME_SHUTTER_OVERRIDE"
    #endif

static CONFIG_INT("extreme.shutter", extreme_shutter, 0);

void extreme_shutter_step()
{
    if (is_movie_mode() && extreme_shutter)
    {
        FRAME_SHUTTER_TIMER = 7 - extreme_shutter;
    }
}
#endif


void image_effects_step()
{
    if (!DISPLAY_IS_ON && !recording) return;
    if (!lv) return;
    if (lv_paused) return;

    #ifdef CONFIG_DIGIC_POKE
    digic_poke_step();
    #endif
    
#ifdef FEATURE_IMAGE_EFFECTS
    
    // bulb ramping calibration works best on grayscale image
    extern int bulb_ramp_calibration_running;
    if (bulb_ramp_calibration_running)
    {
        EngDrvOutLV(0xc0f0f070, 0x01000100);
        return;
    }
    static int prev_swap_uv = 0;

    if (!is_movie_mode()) return;

    if (desaturate) EngDrvOutLV(0xc0f0f070, 0x01000100);
    if (negative)   EngDrvOutLV(0xc0f0f000, 0xb1);
    if (swap_uv)    EngDrvOutLV(0xc0f0de2c, 0x10); else if (prev_swap_uv) EngDrvOutLV(0xc0f0de2c, 0);
    if (cartoon)    
    {
        if (cartoon == 1) 
        {
            EngDrvOutLV(0xc0f23164, -1);
        }
        else if (cartoon == 2)
        { 
            EngDrvOutLV(0xc0f23164, -1);
            EngDrvOutLV(0xc0f0f29c, 0xffff); // also c0f2194c?
        }
        else
        {
            EngDrvOutLV(0xc0f23164, -1);
            for (uint32_t reg = 0xc0f0f100; reg <= 0xc0f0f160; reg += 4)
                EngDrvOutLV(reg, 0);
            EngDrvOutLV(0xc0f0f11c, 128);
            EngDrvOutLV(0xc0f0f128, 128);
            EngDrvOutLV(0xc0f0f134, 128);
            EngDrvOutLV(0xc0f0f150, 128);
            EngDrvOutLV(0xc0f0f160, 128);
        }
        EngDrvOutLV(0xc0f2116c, 0xffff0000); // boost picturestyle sharpness to max
    }
    if (oilpaint)   EngDrvOutLV(0xc0f2135c, -1);
    if (sharp)      EngDrvOutLV(0xc0f0f280, -1);
    if (zerosharp)  EngDrvOutLV(0xc0f2116c, 0x0); // sharpness trick: at -1, cancel it completely

    prev_swap_uv = swap_uv;
#endif
}

void digic_iso_step()
{
#if defined(FEATURE_EXPO_ISO_DIGIC) || defined(FEATURE_LV_DISPLAY_GAIN) || defined(FEATURE_EXTREME_SHUTTER_SPEEDS)
    if (!DISPLAY_IS_ON && !recording) return;
    if (!lv) return;
    if (lv_paused) return;
    int mv = is_movie_mode();
    if (mv && lens_info.iso == 0) return; // no auto ISO, please

    extern int bulb_ramp_calibration_running;
    if (bulb_ramp_calibration_running) return;
#endif
#ifdef FEATURE_EXPO_ISO_DIGIC
    if (mv)
    {
        if (digic_iso_gain_movie_for_gradual_expo == 0) digic_iso_gain_movie_for_gradual_expo = 1024;
        int total_movie_gain = DIGIC_ISO_GAIN_MOVIE * digic_iso_gain_movie_for_gradual_expo / 1024;
        if (total_movie_gain != 1024)
        {
            autodetect_default_white_level();
            int boost_stops = 0;
            int new_gain = get_new_white_level(total_movie_gain, &boost_stops);
            EngDrvOutLV(SHAD_GAIN, new_gain);
            shad_gain_last_written = new_gain;
            #ifndef CONFIG_DIGIC_V
            EngDrvOutLV(ISO_PUSH_REGISTER, boost_stops << 8);
            #endif
        }

        if (digic_black_level)
        {
            int presetup = MEMX(SHAD_PRESETUP);
            presetup = ((presetup + 100) & 0xFF00) + ((int)digic_black_level);
            EngDrvOutLV(SHAD_PRESETUP, presetup);
        }

        #ifdef CONFIG_DIGIC_V
        if (LVAE_DISP_GAIN) call("lvae_setdispgain", 0); // reset display gain
        #endif

    }
#endif
#ifdef FEATURE_LV_DISPLAY_GAIN
    if (!mv) // photo mode - display gain, for preview only
    {
        if (digic_iso_gain_photo_for_bv == 0) digic_iso_gain_photo_for_bv = 1024;
        int total_photo_gain = DIGIC_ISO_GAIN_PHOTO * digic_iso_gain_photo_for_bv / 1024;

        if (total_photo_gain == 0) total_photo_gain = 1024;
    #ifdef CONFIG_DIGIC_V
        int g = total_photo_gain == 1024 ? 0 : COERCE(total_photo_gain, 0, 65534);
        if (LVAE_DISP_GAIN != g) 
        {
            call("lvae_setdispgain", g);
        }
    #else
        if (total_photo_gain > 1024 && !LVAE_DISP_GAIN)
        {
            int boost_stops = COERCE((int)log2f(total_photo_gain / 1024), 0, 7);
            EngDrvOutLV(ISO_PUSH_REGISTER, boost_stops << 8);
        }
    #endif
    }
#endif
}

void menu_open_submenu();

static struct menu_entry lv_img_menu[] = {
    #ifdef FEATURE_VIGNETTING_CORRECTION
    {
        .name = "Vignetting",
        .max = 1,
        .priv = &vignetting_correction_enable,
        .select = vignetting_correction_toggle,
        .help = "Vignetting correction or effects.",
        .depends_on = DEP_MOVIE_MODE,
        .submenu_width = 710,
        .submenu_height = 250,
        .children =  (struct menu_entry[]) {
            {
                .name = "Mid-range correction     ",
                .priv = &vignetting_correction_a,
                .unit = UNIT_x10,
                .min = -10,
                .max = 10,
                .select = vignetting_coeff_toggle,
                .update = vignetting_graphs_update,
                .help = "Correction applied between central area and corners.",
                .help2 = "Tip: set this to -1 for a nice vignette effect.",
            },
            {
                .name = "Corner correction        ",
                .priv = &vignetting_correction_b,
                .min = -10,
                .max = 10,
                .unit = UNIT_x10,
                .select = vignetting_coeff_toggle,
                .update = vignetting_graphs_update,
                .help = "Correction with a stronger bias towards corners.",
            },
            {
                .name = "Extreme corner correction",
                .priv = &vignetting_correction_c,
                .min = -10,
                .max = 10,
                .unit = UNIT_x10,
                .select = vignetting_coeff_toggle,
                .update = vignetting_graphs_update,
            },
            MENU_EOL,
        },
    },
    #endif

    #if defined(FEATURE_IMAGE_EFFECTS) || defined(FEATURE_EXPO_ISO_DIGIC) || defined(FEATURE_EXTREME_SHUTTER_SPEEDS)
    {
        .name = "Image Finetuning",
        .select = menu_open_submenu,
        .help = "Subtle image enhancements via DIGIC register tweaks.",
        .depends_on = DEP_MOVIE_MODE,
        .submenu_width = 700,
        .children =  (struct menu_entry[]) {


            #ifdef FEATURE_EXPO_ISO_DIGIC
            {
                .name = "ML Digital ISO",
                .priv = &digic_iso_gain_movie,
                .update = digic_iso_print_movie,
                .select = digic_iso_toggle_movie,
                .help = "ISO tweaks. Negative gain has better highlight roll-off.",
                .edit_mode = EM_MANY_VALUES_LV,
                .depends_on = DEP_MOVIE_MODE | DEP_MANUAL_ISO,
                .icon_type = IT_DICE_OFF,
            },
            {
                .name = "Black Level", 
                .priv = &digic_black_level,
                .min = -100,
                .max = 100,
                .update = digic_black_print,
                .icon_type = IT_PERCENT_OFF,
                .edit_mode = EM_MANY_VALUES_LV,
                .depends_on = DEP_LIVEVIEW | DEP_MOVIE_MODE,
                .help = "Adjust dark level, as with 'dcraw -k'. Fixes green shadows.",
            },
            #endif
            
            #ifdef FEATURE_EXTREME_SHUTTER_SPEEDS
            {
                .name = "Extreme Shutter Speed", 
                .priv = &extreme_shutter, 
                #ifdef CONFIG_5D3
                .max = 5, // 1/50000 is pitch black
                #else
                .max = 6, // at least 60D works at 1/50000
                #endif
                .choices = CHOICES("OFF", "1/8000", "1/10000", "1/12500", "1/16000", "1/25000", "1/50000"),
                .icon_type = IT_DICE_OFF,
                .edit_mode = EM_MANY_VALUES_LV,
                .help = "Very fast shutter speeds (1/8000 - 1/50000).",
                .depends_on = DEP_LIVEVIEW | DEP_MOVIE_MODE,
            },
            #endif

            #ifdef FEATURE_IMAGE_EFFECTS
            {
                .name = "Absolute Zero Sharpness", 
                .priv = &zerosharp, 
                .max = 1,
                .help = "Disable sharpening completely (below Canon's zero level).",
                .depends_on = DEP_LIVEVIEW | DEP_MOVIE_MODE,
            },
            #if !(defined(CONFIG_600D) || defined(CONFIG_1100D))
            {
                .name = "Edge Emphasis", 
                .priv = &sharp, 
                .max = 1,
                .help = "Darken sharp edges in bright areas.",
                .depends_on = DEP_LIVEVIEW | DEP_MOVIE_MODE,
            },
            {
                .name = "Noise Reduction", 
                .priv = &oilpaint, 
                .max = 1,
                .help = "Some sort of movie noise reduction, or smearing.",
                .depends_on = DEP_LIVEVIEW | DEP_MOVIE_MODE,
            },
            #endif
            #endif
            
            MENU_EOL,
        }
    },
    #endif

    #ifdef FEATURE_IMAGE_EFFECTS
    {
        .name = "Creative Effects",
        .select = menu_open_submenu,
        .help = "Experimental image filters found by digging into DIGIC.",
        .depends_on = DEP_MOVIE_MODE,
        .children =  (struct menu_entry[]) {
            {
                .name = "Desaturate",
                .priv       = &desaturate,
                .min = 0,
                .max = 1,
                .help = "Grayscale recording. Use WB or pic styles for fine tuning.",
                .depends_on = DEP_LIVEVIEW | DEP_MOVIE_MODE,
            },
            {
                .name = "Negative",
                .priv       = &negative,
                .min = 0,
                .max = 1,
                .help = "Negative image. Inverts all colors :)",
                .depends_on = DEP_LIVEVIEW | DEP_MOVIE_MODE,
            },
            {
                .name = "Swap U-V",
                .priv       = &swap_uv,
                .min = 0,
                .max = 1,
                .help = "Swaps U and V channels (red <--> blue).",
                .depends_on = DEP_LIVEVIEW | DEP_MOVIE_MODE,
            },
            {
                .name = "Cartoon Look",
                .priv       = &cartoon,
                .min = 0,
                .max = 3,
                .choices = (const char *[]) {"OFF", "Mode 1", "Mode 2", "Mode 3"},
                .help = "Cartoonish look obtained by emphasizing the edges.",
                .icon_type = IT_DICE_OFF,
                .depends_on = DEP_LIVEVIEW | DEP_MOVIE_MODE,
            },
            MENU_EOL
        }
    }
    #endif
};

#ifdef CONFIG_DIGIC_POKE

static struct menu_entry dbg_menu[] = {
    {
        .name = "DIGIC poke",
        .priv       = &digic_poke,
        .min = 0,
        .max = 1,
        .help = "Changes a DIGIC register to find out what it does. DANGER!",
        .children =  (struct menu_entry[]) {
            {
                .name = "Register family",
                .priv = &digic_register_base,
                .unit = UNIT_HEX,
                .min = 0xC000,
                .max = 0xCFFF,
                .help = "DIGIC register address, mask=FFFF0000.",
            },
            {
                .name = "Register base  ",
                .priv = &digic_register_mid,
                .unit = UNIT_HEX,
                .min = 0x00,
                .max = 0xFF,
                .help = "DIGIC register address, mask=0000FF00.",
            },
            {
                .name = "Register offset",
                .priv = &digic_register_off,
                .unit = UNIT_HEX,
                .min = 0x00,
                .max = 0xFF,
                .select = hex_toggle_8bit,
                .help = "DIGIC register address, mask=000000FC.",
            },
            {
                .name = "Value          ",
                .priv = &digic_value,
                .update = digic_value_print,
                .select = digic_value_toggle,
                .help = "Current value of selected register. Change w. HalfShutter.",
            },
            {
                .name = "Altering mode  ",
                .priv = &digic_alter_mode,
                .max = 4,
                .choices = (const char *[]) {"rand()", "x++", "x += (1<<8)", "x += (1<<16)", "x += (1<<24)"},
                .help = "How to change current value [HalfShutter]. MF(+) / AF(-).",
            },
            {
                .name = "Random register",
                .select = digic_random_register,
                .help = "Click to select some random register.",
            },
            MENU_EOL
        }
    }, 
    {
        .name = "Dump DIGIC registers",
        .priv = digic_dump,
        .select = run_in_separate_task,
        .help = "Saves the contents of DIGIC shadow copy to DIGIC.LOG."
    }
};
#endif

static void lv_img_init()
{
    menu_add( "Movie", lv_img_menu, COUNT(lv_img_menu) );
    
#ifdef CONFIG_DIGIC_POKE
    menu_add( "Debug", dbg_menu, COUNT(dbg_menu) );
#endif

}

static void vignetting_init (void * parm)
{
    vignetting_correction_set_coeffs(vignetting_correction_a, vignetting_correction_b, vignetting_correction_c);
    vignetting_correction_initialized = 1;
}

INIT_FUNC("lv_img", lv_img_init);
TASK_CREATE( "vignetting_init", vignetting_init, 0, 0x1e, 0x2000 );
