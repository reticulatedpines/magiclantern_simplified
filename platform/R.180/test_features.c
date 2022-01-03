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

extern void DISP_SetHighLight(int);
int highlight_flag = 0;

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
        .children =  (struct menu_entry[]) {
            {
                .name   = "Overexposure warning in LiveView",
                .priv   = overexpo_toggle,
                .select = run_in_separate_task,
                .help   = "You may need to toggle twice each time you enter LV."
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
