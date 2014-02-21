

#include "dryos.h"
#include "bmp.h"
#include "tasks.h"
#include "debug.h"
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

static void draw_electronic_level(int angle, int prev_angle, int force_redraw)
{
    if (!force_redraw && angle == prev_angle) return;

    int x0 = os.x0 + os.x_ex/2;
    int y0 = os.y0 + os.y_ex/2;
    int r = 200;
    draw_angled_line(x0, y0, r, prev_angle, 0);
    draw_angled_line(x0+1, y0+1, r, prev_angle, 0);
    draw_angled_line(x0, y0, r, angle, (angle % 900) ? COLOR_BLACK : COLOR_GREEN1);
    draw_angled_line(x0+1, y0+1, r, angle, (angle % 900) ? COLOR_WHITE : COLOR_GREEN2);
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
    draw_electronic_level(angle10, prev_angle10, force_redraw);
    draw_electronic_level(angle10 + 1800, prev_angle10 + 1800, force_redraw);
    //~ draw_line(x0, y0, x0 + r * cos(angle), y0 + r * sin(angle), COLOR_BLUE);
    prev_angle10 = angle10;

    //if (angle10 > 1800) angle10 -= 3600;
    //bmp_printf(FONT_MED, 0, 35, "%s%3d", angle10 < 0 ? "-" : angle10 > 0 ? "+" : " ", ABS(angle10/10));
}

static LVINFO_UPDATE_FUNC(electronic_level_update)
{
    item->hidden = level_data.status == 2 ? 0 : 1;
    item->disabled = level_data.status == 2 ? 0 : 1;
    
    LVINFO_BUFFER(8);
    int angle10 = (level_data.roll_sensor1 * 256 + level_data.roll_sensor2) / 10;
    if (angle10 > 1800) angle10 -= 3600;
    
    snprintf(buffer, sizeof(buffer), "%s%3d"SYM_DEGREE, angle10 == 0 ? "=" : (ABS(angle10) > 0 && ABS(angle10) < 10) ? SYM_PLUSMINUS : angle10 >= 10 ? "+" : angle10 <= 10 ? "-" : " ", ABS(angle10/10));
    
    item->color_fg = angle10 == 0 || ABS(angle10) == 900 || ABS(angle10) == 1800 ? COLOR_GREEN1 : COLOR_WHITE;
    
}

static struct lvinfo_item info_items[] = {
    {
        .name = "Electronic level",
        .which_bar = LV_TOP_BAR_ONLY,
        .update = electronic_level_update,
        .preferred_position = -100,
        .priority = 1,
        .hidden = 1,
        .disabled = 1,
    },
};

static void electronic_level_init()
{
    lvinfo_add_items(info_items, COUNT(info_items));
}

INIT_FUNC("electronic_level_info", electronic_level_init);

#endif

