/** \file
 * Temporary tests for features, specific for R 1.8.0
 *
 * When porting to a new model, using this camera as a base, remove this file
 * and remove it from ML_SRC_EXTRA_OBJS in Makefile.platform.default!
 */
/*
 * Copyright (C) 2022 Magic Lantern Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef CONFIG_HELLO_WORLD
#include <dryos.h>
#include <property.h>
#include <bmp.h>

// random props I found
#define PROP_BATTERY_CHECK 0x8030013

/*
 * Temperature monitoring.
 * See props registerd in TempSvc task (e01ae332). Names come from function
 * at e01ae6f8.
 * PROP_EFIC_TEMP seems not to exists anymore. ID is referenced just once,
 * for prop slave (no masters registered)
 */

// handled by PropTemperatureMaster. Not sure what those do.
#define PROP_R_TEMP_X1   0x8004000f
#define PROP_R_TEMP_X2   0x80030080

// handled by PropTempStatusMaster
#define PROP_R_TEMP_SH   0x80030062
#define PROP_R_TEMP_MAIN 0x80030035
#define PROP_R_TEMP_A    0x8003009E
#define PROP_R_TEMP_WM   0x80030073
#define PROP_R_TEMP_BACK 0x80030081


uint32_t eosr_temperatures[5];

uint32_t rawTempToDegrees(uint raw)
{
    // Data comes in form of fixed point value: 4 LSB for decimal, rest for deg
    // In fn e01ae228 it is converted into a different value for prop delivery
    // Unfortunately this conversion eats up decimal points, so I just skip them
    return ((((raw - 0x80) << 0x14) / 0x10000) - 8) >> 4;
}

void printTemp(char * msg, uint32_t raw)
{
    uint32_t tmp = rawTempToDegrees(raw);
}

PROP_HANDLER(PROP_R_TEMP_SH)
{
    eosr_temperatures[0] = rawTempToDegrees(buf[0]);
}

PROP_HANDLER(PROP_R_TEMP_MAIN)
{
    eosr_temperatures[1] = rawTempToDegrees(buf[0]);
}

PROP_HANDLER(PROP_R_TEMP_A)
{
    eosr_temperatures[2] = rawTempToDegrees(buf[0]);
}

PROP_HANDLER(PROP_R_TEMP_WM)
{
    eosr_temperatures[3] = rawTempToDegrees(buf[0]);
}

PROP_HANDLER(PROP_R_TEMP_BACK)
{
    eosr_temperatures[4] = rawTempToDegrees(buf[0]);
}

static MENU_UPDATE_FUNC(temp_display)
{
    int sensor_id = (int) entry->priv;
    if (sensor_id >= sizeof(eosr_temperatures))
      return;

    MENU_SET_VALUE("%d"SYM_DEGREE"C", eosr_temperatures[sensor_id]);
}

/*
 * Display overexpo overlay in LV. Temporary solution using Canon functions.
 * See https://wiki.magiclantern.fm/digic8:registers for registers description
 * that should contribute to existing ML over/underexpo feature.
 */
extern void DISP_SetHighLight(int);
int highlight_flag = 0;

/*
 * This could be controlled directly via code below:
 *  #define D8_ZEBRA_EN_REG 0xD0304220
 *  #define D8_ZEBRA_EN_R_OFF 0x2000
 *  #define D8_ZEBRA_EN_R_MASK 0x100
 *  #define D8_ZEBRA_THRESHOLD_REG 0xD0304488
 *  #define D8_ZEBRA_COLOUR_REG 0xD030448C
 *
 *  // turns on highlights
 *  *(uint*)D8_ZEBRA_COLOUR_REG = 0x00008080;
 *  *(uint*)D8_ZEBRA_THRESHOLD_REG = 0x800700FF;
 *  *(uint*)D8_ZEBRA_EN_REG = 0x100;
 */
static void overexpo_toggle()
{
    highlight_flag = !highlight_flag;
    msleep(1000); //let the code execute when user is already back in LV
    DryosDebugMsg(0, 15, "Toggle overexposure warning in LV: %d", highlight_flag);
    DISP_SetHighLight(highlight_flag);
}

static struct menu_entry test_features_debug_menu[] = {
    {
        .name   = "EOS R Test features",
        .select = menu_open_submenu,
        .help   = "Temporary features, not integrated into main codebase yet.",
        .submenu_width = 710,
        .children =  (struct menu_entry[]) {
            {
                .name   = "Overexposure warning in LiveView",
                .priv   = overexpo_toggle,
                .select = run_in_separate_task,
                .help   = "You may need to toggle twice each time you enter LV."
            },
            {
                .name   = "Temp SH",
                .priv   = (int*)0,
                .update = temp_display,
                .help   = "Used by PhotoOperator",
                .icon_type = IT_ALWAYS_ON
            },
            {
                .name   = "Temp MAIN",
                .priv   = (int*)1,
                .update = temp_display,
                .help   = "Maybe sensor? Used by lv_set_atemp_new",
                .icon_type = IT_ALWAYS_ON
            },
            {
                .name   = "Temp A",
                .priv   = (int*)2,
                .update = temp_display,
                .help   = "Used by PhotoOperator and GmtState",
                .icon_type = IT_ALWAYS_ON
            },
            {
                .name   = "Temp WM",
                .priv   = (int*)3,
                .update = temp_display,
                .help   = "Unknown location, not used in code",
                .icon_type = IT_ALWAYS_ON
            },
            {
                .name   = "Temp BACK",
                .priv   = (int*)4,
                .update = temp_display,
                .help   = "Probably located near LCD",
                .icon_type = IT_ALWAYS_ON
            },
            MENU_EOL,
        },
    },
};

static void test_features_init()
{
    menu_add("Debug", test_features_debug_menu, COUNT(test_features_debug_menu));
}

INIT_FUNC(__FILE__, test_features_init);

#endif
