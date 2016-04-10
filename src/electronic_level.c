#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
#include "math.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "lens.h"
#include "version.h"
#include "edmac.h"
#include "asm.h"
#include "lvinfo.h"

#ifdef CONFIG_ELECTRONIC_LEVEL

/* Canon stub */
extern void GUI_SetRollingPitchingLevelStatus(int request);

struct rolling_pitching
{
    uint8_t status;
    uint8_t cameraposture;
    uint8_t roll_sensor1;
    uint8_t roll_sensor2;
    uint8_t pitch_sensor1;
    uint8_t pitch_sensor2;
};

static struct rolling_pitching level_data;

PROP_HANDLER(PROP_ROLLING_PITCHING_LEVEL)
{
    memcpy(&level_data, buf, 6);
}

static void draw_level_lines(int angle, int pitch, int show)
{
    int x0 = os.x0 + os.x_ex/2;
    int y0 = os.y0 + os.y_ex/2;
    int r = 200;

    #define MUL 16384
    #define PI_1800 0.00174532925

    // Compute roll line parameters
    int s = sinf(angle * PI_1800) * MUL;
    int c = cosf(angle * PI_1800) * MUL;
    int x_offset = r * c / MUL;
    int y_offset = r * s / MUL;
    int dx = (angle % 1800 >= 450 && angle % 1800 < 1350);
    int dy = 1 - dx;

    int color1 = show ? ((angle % 900) ? COLOR_BLACK : COLOR_GREEN1) : 0;
    int color2 = show ? ((angle % 900) ? COLOR_WHITE : COLOR_GREEN2) : 0;

    // Draw or erase the roll line
    draw_line(x0 - x_offset, y0 - y_offset, x0 + x_offset, y0 + y_offset, color1);
    draw_line(x0 - x_offset + dx, y0 - y_offset + dy, x0 + x_offset + dx, y0 + y_offset + dy, color2);

    // Don't draw the pitch line until we see the pitch value change. This prevents it
    // ever being drawn on cameras that don't report pitch, such as the 60D.
    static int pitch_changed = 0;
    if (pitch != 0) pitch_changed = 1;
    if (!pitch_changed) return;

    // Compute pitch line parameters
    int pitch_s = sinf(pitch * PI_1800) * MUL;

    r = 120;
    x0 -= (r * pitch_s / MUL) * s / MUL;
    y0 += (r * pitch_s / MUL) * c / MUL;
    x_offset /= 2;
    y_offset /= 2;

    color1 = show ? ((pitch % 900) ? COLOR_BLACK : COLOR_GREEN1) : 0;
    color2 = show ? ((pitch % 900) ? COLOR_WHITE : COLOR_GREEN2) : 0;

    // Draw or erase the pitch line
    draw_line(x0 - x_offset, y0 - y_offset, x0 + x_offset, y0 + y_offset, color1);
    draw_line(x0 - x_offset + dx, y0 - y_offset + dy, x0 + x_offset + dx, y0 + y_offset + dy, color2);
}

static void draw_electronic_level(int angle, int prev_angle, int pitch, int prev_pitch, int force_redraw)
{
    if (!force_redraw && angle == prev_angle && pitch == prev_pitch) return;

    draw_level_lines(prev_angle, prev_pitch, 0);
    draw_level_lines(angle, pitch, 1);
}

void disable_electronic_level()
{
    if (level_data.status == 2)
    {
        GUI_SetRollingPitchingLevelStatus(1);
        msleep(100);
    }
}

void show_electronic_level()
{
    static int prev_angle10 = 0;
    static int prev_pitch10 = 0;
    int force_redraw = 0;
    if (level_data.status != 2)
    {
        GUI_SetRollingPitchingLevelStatus(0);
        msleep(100);
        force_redraw = 1;
    }

    static int k = 0;
    k++;
    if (k % 10 == 0) force_redraw = 1;

    int angle100 = level_data.roll_sensor1 * 256 + level_data.roll_sensor2;
    int angle10 = angle100/10;
    int pitch100 = level_data.pitch_sensor1 * 256 + level_data.pitch_sensor2;
    int pitch10 = pitch100/10;
    draw_electronic_level(angle10, prev_angle10, pitch10, prev_pitch10, force_redraw);
    prev_angle10 = angle10;
    prev_pitch10 = pitch10;
}

#define FMT_FIXEDPOINT1E(x) (x) < 0 ? "-" : (x) > 0 ? "+" : "=", ABS(x)/10, ABS(x)%10

static LVINFO_UPDATE_FUNC(electronic_level_update)
{    
    LVINFO_BUFFER(8);
    if(level_data.status == 2)
    {
        int angle10 = (level_data.roll_sensor1 * 256 + level_data.roll_sensor2) / 10;
        if (angle10 > 1800) angle10 -= 3600;
    
        snprintf(buffer, sizeof(buffer), "%s%d.%d" SYM_DEGREE, FMT_FIXEDPOINT1E(angle10));
    
        item->color_fg = angle10 == 0 || ABS(angle10) == 900 || ABS(angle10) == 1800 ? COLOR_GREEN1 : COLOR_WHITE;
    }
}

static struct lvinfo_item info_items[] = {
    {
        .name = "Electronic level",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = electronic_level_update,
        .preferred_position = -100,
        .priority = 1,
    },
};

static void electronic_level_init()
{
    lvinfo_add_items(info_items, COUNT(info_items));
}

INIT_FUNC("electronic_level_info", electronic_level_init);

#endif
