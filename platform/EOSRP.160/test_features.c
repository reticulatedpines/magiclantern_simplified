/** \file
 * Temporary tests for features, specific for RP 1.6.0
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

#include "config.h"

void platform_pre_shutdown() {
#ifdef FEATURE_CLOSE_SHUTTER_ON_SHUTDOWN
    extern int close_shutter_on_shutdown;

    if (close_shutter_on_shutdown)
        call("FA_MechaShutterClose");
#endif
}

CONFIG_INT("experimental.close_shutter_on_shutdown", close_shutter_on_shutdown, 0);

static struct menu_entry test_features_debug_menu[] = {
    {
        .name     = "RP test features",
        .select   = menu_open_submenu,
        .help     = "Temporary features, not integrated into main codebase yet.",
        .children = (struct menu_entry[]) {
#ifdef FEATURE_CLOSE_SHUTTER_ON_SHUTDOWN
            {
                .name    = "Close shutter on camera shutdown",
                .priv    = &close_shutter_on_shutdown,
                .max     = 1,
                .choices = (const char *[]) {"OFF", "ON"},
                .help    = "Close shutter when turning off camera for sensor dust protection."
            },
#endif // FEATURE_CLOSE_SHUTTER_ON_SHUTDOWN
            MENU_EOL,
	},
   },
};

static void test_features_init()
{
    menu_add("Debug", test_features_debug_menu, COUNT(test_features_debug_menu));
}

INIT_FUNC(__FILE__, test_features_init);

#endif // !CONFIG_HELLO_WORLD

