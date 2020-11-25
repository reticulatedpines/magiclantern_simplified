/***
 Constants
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module constants
 */

#undef MODULE
#include <module.h>
#include <dryos.h>
#include <bmp.h>
#include <menu.h>
#include <property.h>

#include "lua_common.h"

/// Key Codes
// @field HALFSHUTTER
// @field UNPRESS_HALFSHUTTER
// @field FULLSHUTTER
// @field UNPRESS_FULLSHUTTER
// @field WHEEL_UP
// @field WHEEL_DOWN
// @field WHEEL_LEFT
// @field WHEEL_RIGHT
// @field SET
// @field UNPRESS_SET
// @field JOY_CENTER
// @field UP
// @field UP_RIGHT
// @field UP_LEFT
// @field RIGHT
// @field LEFT
// @field DOWN_RIGHT
// @field DOWN_LEFT
// @field DOWN
// @field UNPRESS_UDLR
// @field ZOOMIN
// @field MENU
// @field INFO
// @field PLAY
// @field TRASH
// @field RATE
// @field REC
// @field LV
// @field Q
// @field PICSTYLE
// @field FLASH_MOVIE
// @field UNPRESS_FLASH_MOVIE
// @field DP
// @field UNPRES_DP
// @field TOUCH_1_FINGER
// @field UNTOUCH_1_FINGER
// @field TOUCH_2_FINGER
// @field UNTOUCH_2_FINGER
// @table KEY
int luaopen_KEY(lua_State * L)
{
    lua_newtable(L);
    LUA_CONSTANT(HALFSHUTTER, MODULE_KEY_PRESS_HALFSHUTTER);
    LUA_CONSTANT(UNPRESS_HALFSHUTTER, MODULE_KEY_UNPRESS_HALFSHUTTER);
    LUA_CONSTANT(FULLSHUTTER, MODULE_KEY_PRESS_FULLSHUTTER);
    LUA_CONSTANT(UNPRESS_FULLSHUTTER, MODULE_KEY_UNPRESS_FULLSHUTTER);
    LUA_CONSTANT(WHEEL_UP, MODULE_KEY_WHEEL_UP);
    LUA_CONSTANT(WHEEL_DOWN, MODULE_KEY_WHEEL_DOWN);
    LUA_CONSTANT(WHEEL_LEFT, MODULE_KEY_WHEEL_LEFT);
    LUA_CONSTANT(WHEEL_RIGHT, MODULE_KEY_WHEEL_RIGHT);
    LUA_CONSTANT(SET, MODULE_KEY_PRESS_SET);
    LUA_CONSTANT(UNPRESS_SET, MODULE_KEY_UNPRESS_SET);
    LUA_CONSTANT(JOY_CENTER, MODULE_KEY_JOY_CENTER);
    LUA_CONSTANT(UP, MODULE_KEY_PRESS_UP);
    LUA_CONSTANT(UP_RIGHT, MODULE_KEY_PRESS_UP_RIGHT);
    LUA_CONSTANT(UP_LEFT, MODULE_KEY_PRESS_UP_LEFT);
    LUA_CONSTANT(RIGHT, MODULE_KEY_PRESS_RIGHT);
    LUA_CONSTANT(LEFT, MODULE_KEY_PRESS_LEFT);
    LUA_CONSTANT(DOWN_RIGHT, MODULE_KEY_PRESS_DOWN_RIGHT);
    LUA_CONSTANT(DOWN_LEFT, MODULE_KEY_PRESS_DOWN_LEFT);
    LUA_CONSTANT(DOWN, MODULE_KEY_PRESS_DOWN);
    LUA_CONSTANT(UNPRESS_UDLR, MODULE_KEY_UNPRESS_UDLR);
    LUA_CONSTANT(ZOOMIN, MODULE_KEY_PRESS_ZOOMIN);
    LUA_CONSTANT(MENU, MODULE_KEY_MENU);
    LUA_CONSTANT(INFO, MODULE_KEY_INFO);
    LUA_CONSTANT(PLAY, MODULE_KEY_PLAY);
    LUA_CONSTANT(TRASH, MODULE_KEY_TRASH);
    LUA_CONSTANT(RATE, MODULE_KEY_RATE);
    LUA_CONSTANT(REC, MODULE_KEY_REC);
    LUA_CONSTANT(LV, MODULE_KEY_LV);
    LUA_CONSTANT(Q, MODULE_KEY_Q);
    LUA_CONSTANT(PICSTYLE, MODULE_KEY_PICSTYLE);
    LUA_CONSTANT(FLASH_MOVIE, MODULE_KEY_PRESS_FLASH_MOVIE);
    LUA_CONSTANT(UNPRESS_FLASH_MOVIE, MODULE_KEY_UNPRESS_FLASH_MOVIE);
    LUA_CONSTANT(DP, MODULE_KEY_PRESS_DP);
    LUA_CONSTANT(UNPRESS_DP, MODULE_KEY_UNPRESS_DP);
    LUA_CONSTANT(TOUCH_1_FINGER, MODULE_KEY_TOUCH_1_FINGER);
    LUA_CONSTANT(UNTOUCH_1_FINGER, MODULE_KEY_UNTOUCH_1_FINGER);
    LUA_CONSTANT(TOUCH_2_FINGER, MODULE_KEY_TOUCH_2_FINGER);
    LUA_CONSTANT(UNTOUCH_2_FINGER, MODULE_KEY_UNTOUCH_2_FINGER);
    return 1;
}

static int luaCB_color_gray(lua_State * L)
{
    LUA_PARAM_INT(percent, 1);
    lua_pushinteger(L, COLOR_GRAY(percent));
    return 1;
}

/// Color palatte
// @field TRANSPARENT
// @field WHITE
// @field BLACK
// @field TRANSPARENT_BLACK
// @field LIGHT_GRAY
// @field GRAY
// @field DARK_GRAY
// @field CYAN
// @field GREEN1
// @field GREEN2
// @field RED
// @field LIGHT_BLUE
// @field BLUE
// @field DARK_RED
// @field MAGENTA
// @field YELLOW
// @field ORANGE
// @field ALMOST_BLACK
// @field ALMOST_WHITE
// @field DARK_GREEN1_MOD
// @field DARK_GREEN2_MOD
// @field DARK_ORANGE_MOD
// @field DARK_CYAN1_MOD
// @field DARK_CYAN2_MOD
// @tfield func gray converts integer percentage to shade of gray
// @table COLOR
int luaopen_COLOR(lua_State * L)
{
    lua_newtable(L);
    LUA_CONSTANT(TRANSPARENT, COLOR_EMPTY);
    LUA_CONSTANT(WHITE, COLOR_WHITE);
    LUA_CONSTANT(BLACK, COLOR_BLACK);
    LUA_CONSTANT(TRANSPARENT_BLACK, COLOR_TRANSPARENT_BLACK);
    LUA_CONSTANT(LIGHT_GRAY, COLOR_GRAY(80));
    LUA_CONSTANT(GRAY, COLOR_GRAY(50));
    LUA_CONSTANT(DARK_GRAY, COLOR_GRAY(20));
    LUA_CONSTANT(CYAN, COLOR_CYAN);
    LUA_CONSTANT(GREEN1, COLOR_GREEN1);
    LUA_CONSTANT(GREEN2, COLOR_GREEN2);
    LUA_CONSTANT(RED, COLOR_RED);
    LUA_CONSTANT(LIGHT_BLUE, COLOR_LIGHT_BLUE);
    LUA_CONSTANT(BLUE, COLOR_BLUE);
    LUA_CONSTANT(DARK_RED, COLOR_DARK_RED);
    LUA_CONSTANT(MAGENTA, COLOR_MAGENTA);
    LUA_CONSTANT(YELLOW, COLOR_YELLOW);
    LUA_CONSTANT(ORANGE, COLOR_ORANGE);
    LUA_CONSTANT(ALMOST_BLACK, COLOR_ALMOST_BLACK);
    LUA_CONSTANT(ALMOST_WHITE, COLOR_ALMOST_WHITE);
    LUA_CONSTANT(DARK_GREEN1_MOD, COLOR_DARK_GREEN1_MOD);
    LUA_CONSTANT(DARK_GREEN2_MOD, COLOR_DARK_GREEN2_MOD);
    LUA_CONSTANT(DARK_ORANGE_MOD, COLOR_DARK_ORANGE_MOD);
    LUA_CONSTANT(DARK_CYAN1_MOD, COLOR_DARK_CYAN1_MOD);
    LUA_CONSTANT(DARK_CYAN2_MOD, COLOR_DARK_CYAN2_MOD);
    lua_pushcfunction(L, luaCB_color_gray);
    lua_setfield(L, -2, "gray");
    return 1;
}

static int luaCB_font_index(lua_State * L);
static int luaCB_font_newindex(lua_State * L);

#define LUA_FONT(name, value) \
lua_newtable(L);\
lua_pushinteger(L, value);\
lua_setfield(L, -2, "_spec");\
lua_pushcfunction(L, luaCB_font_index);\
lua_setfield(L, -2, "__index");\
lua_pushcfunction(L, luaCB_font_newindex);\
lua_setfield(L, -2, "__newindex");\
lua_pushvalue(L, -1);\
lua_setmetatable(L, -2);\
lua_setfield(L, -2, #name);

/// Fonts
// @tfield font MONO_12
// @tfield font MONO_20
// @tfield font SANS_23
// @tfield font SANS_28
// @tfield font SANS_32
// @tfield font CANON
// @tfield font SMALL
// @tfield font MED
// @tfield font MED_LARGE
// @tfield font LARGE
// @table FONT
int luaopen_FONT(lua_State * L)
{
    lua_newtable(L);
    LUA_FONT(MONO_12, FONT_MONO_12);
    LUA_FONT(MONO_20, FONT_MONO_20);
    LUA_FONT(SANS_23, FONT_SANS_23);
    LUA_FONT(SANS_28, FONT_SANS_28);
    LUA_FONT(SANS_32, FONT_SANS_32);
    LUA_FONT(CANON, FONT_CANON);
    LUA_FONT(SMALL, FONT_SMALL);
    LUA_FONT(MED, FONT_MED);
    LUA_FONT(MED_LARGE, FONT_MED_LARGE);
    LUA_FONT(LARGE, FONT_LARGE);
    return 1;
}

/// Camera shooting mode
// @field P
// @field TV
// @field AV
// @field M
// @field BULB
// @field ADEP
// @field C
// @field C2
// @field C3
// @field CA
// @field AUTO
// @field NOFLASH
// @field PORTRAIT
// @field LANDSCAPE
// @field MACRO
// @field SPORTS
// @field NIGHT
// @field MOVIE
// @table MODE
int luaopen_MODE(lua_State * L)
{
    lua_newtable(L);
    LUA_CONSTANT(P, SHOOTMODE_P);
    LUA_CONSTANT(TV, SHOOTMODE_TV);
    LUA_CONSTANT(AV, SHOOTMODE_AV);
    LUA_CONSTANT(M, SHOOTMODE_M);
    LUA_CONSTANT(BULB, SHOOTMODE_BULB);
    LUA_CONSTANT(ADEP, SHOOTMODE_ADEP);
    LUA_CONSTANT(C, SHOOTMODE_C);
    LUA_CONSTANT(C2, SHOOTMODE_C2);
    LUA_CONSTANT(C3, SHOOTMODE_C3);
    LUA_CONSTANT(CA, SHOOTMODE_CA);
    LUA_CONSTANT(AUTO, SHOOTMODE_AUTO);
    LUA_CONSTANT(NOFLASH, SHOOTMODE_NOFLASH);
    LUA_CONSTANT(PORTRAIT, SHOOTMODE_PORTRAIT);
    LUA_CONSTANT(LANDSCAPE, SHOOTMODE_LANDSCAPE);
    LUA_CONSTANT(MACRO, SHOOTMODE_MACRO);
    LUA_CONSTANT(SPORTS, SHOOTMODE_SPORTS);
    LUA_CONSTANT(NIGHT, SHOOTMODE_NIGHT);
    LUA_CONSTANT(MOVIE, SHOOTMODE_MOVIE);
    return 1;
}

/// Menu icon type
// @field AUTO
// @field BOOL
// @field DICE
// @field PERCENT
// @field ALWAYS_ON
// @field ACTION
// @field BOOL_NEG
// @field DISABLE_SOME_FEATURE
// @field SUBMENU
// @field DICE_OFF
// @field PERCENT_OFF
// @field PERCENT_LOG
// @field PERCENT_LOG_OFF
// @table ICON_TYPE
int luaopen_ICON_TYPE(lua_State * L)
{
    lua_newtable(L);
    LUA_CONSTANT(AUTO, IT_AUTO);
    LUA_CONSTANT(BOOL, IT_BOOL);
    LUA_CONSTANT(DICE, IT_DICE);
    LUA_CONSTANT(PERCENT, IT_PERCENT);
    LUA_CONSTANT(ALWAYS_ON, IT_ALWAYS_ON);
    LUA_CONSTANT(ACTION, IT_ACTION);
    LUA_CONSTANT(BOOL_NEG, IT_BOOL_NEG);
    LUA_CONSTANT(DISABLE_SOME_FEATURE, IT_DISABLE_SOME_FEATURE);
    LUA_CONSTANT(SUBMENU, IT_SUBMENU);
    LUA_CONSTANT(DICE_OFF, IT_DICE_OFF);
    LUA_CONSTANT(PERCENT_OFF, IT_PERCENT_OFF);
    LUA_CONSTANT(PERCENT_LOG, IT_PERCENT_LOG);
    LUA_CONSTANT(PERCENT_LOG_OFF, IT_PERCENT_LOG_OFF);
    return 1;
}

/// Menu value unit
// @field EV
// @field x10
// @field PERCENT
// @field PERCENT_x10
// @field ISO
// @field HEX
// @field DEC
// @field TIME
// @table UNIT
int luaopen_UNIT(lua_State * L)
{
    lua_newtable(L);
    LUA_CONSTANT(EV, UNIT_1_8_EV);
    LUA_CONSTANT(x10, UNIT_x10);
    LUA_CONSTANT(PERCENT, UNIT_PERCENT);
    LUA_CONSTANT(PERCENT_x10, UNIT_PERCENT_x10);
    LUA_CONSTANT(ISO, UNIT_ISO);
    LUA_CONSTANT(HEX, UNIT_HEX);
    LUA_CONSTANT(DEC, UNIT_DEC);
    LUA_CONSTANT(TIME, UNIT_TIME);
    return 1;
}

/// Dependency for a menu item
// @field GLOBAL_DRAW
// @field LIVEVIEW
// @field NOT_LIVEVIEW
// @field MOVIE_MODE
// @field PHOTO_MODE
// @field AUTOFOCUS
// @field MANUAL_FOCUS
// @field CFN_AF_HALFSHUTTER
// @field CFN_AF_BACK_BUTTON
// @field EXPSIM
// @field NOT_EXPSIM
// @field CHIPPED_LENS
// @field M_MODE
// @field MANUAL_ISO
// @field SOUND_RECORDING
// @field NOT_SOUND_RECORDING
// @table DEPENDS_ON
int luaopen_DEPENDS_ON(lua_State * L)
{
    lua_newtable(L);
    LUA_CONSTANT(GLOBAL_DRAW, DEP_GLOBAL_DRAW);
    LUA_CONSTANT(LIVEVIEW, DEP_LIVEVIEW);
    LUA_CONSTANT(NOT_LIVEVIEW, DEP_NOT_LIVEVIEW);
    LUA_CONSTANT(MOVIE_MODE, DEP_MOVIE_MODE);
    LUA_CONSTANT(PHOTO_MODE, DEP_PHOTO_MODE);
    LUA_CONSTANT(AUTOFOCUS, DEP_AUTOFOCUS);
    LUA_CONSTANT(MANUAL_FOCUS, DEP_MANUAL_FOCUS);
    LUA_CONSTANT(CFN_AF_HALFSHUTTER, DEP_CFN_AF_HALFSHUTTER);
    LUA_CONSTANT(CFN_AF_BACK_BUTTON, DEP_CFN_AF_BACK_BUTTON);
    LUA_CONSTANT(EXPSIM, DEP_EXPSIM);
    LUA_CONSTANT(NOT_EXPSIM, DEP_NOT_EXPSIM);
    LUA_CONSTANT(CHIPPED_LENS, DEP_CHIPPED_LENS);
    LUA_CONSTANT(M_MODE, DEP_M_MODE);
    LUA_CONSTANT(MANUAL_ISO, DEP_MANUAL_ISO);
    LUA_CONSTANT(SOUND_RECORDING, DEP_SOUND_RECORDING);
    LUA_CONSTANT(NOT_SOUND_RECORDING, DEP_NOT_SOUND_RECORDING);
    return 1;
}

/***
 Font.

 TODO: move to a module.

 @type font
 */

/***
 Gets the width of some text in this font.
 @tparam string text
 @function width
 */
static int luaCB_font_width(lua_State * L)
{
    lua_getfield(L, 1, "_spec");
    uint32_t spec = (uint32_t)lua_tointeger(L, -1);
    lua_pop(L, 1);
    LUA_PARAM_STRING(text, 2);
    lua_pushinteger(L, bmp_string_width(spec, text));
    return 1;
}

static int luaCB_font_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "_spec")) return lua_rawget(L, 1);
    
    lua_getfield(L, 1, "_spec");
    uint32_t spec = (uint32_t)lua_tointeger(L, -1);
    lua_pop(L, 1);
    /// The height of this font in pixels
    // @tparam int height
    if(!strcmp(key,"height")) lua_pushinteger(L, fontspec_height(spec));
    else if(!strcmp(key,"width")) lua_pushcfunction(L, luaCB_font_width);
    else return 0;
    return 1;
}

static int luaCB_font_newindex(lua_State * L)
{
    return luaL_error(L, "font type is readonly");
}

int luaopen_constants(lua_State *L)
{
    luaL_requiref(L, "KEY", luaopen_KEY, 1);
    luaL_requiref(L, "COLOR", luaopen_COLOR, 1);
    luaL_requiref(L, "FONT", luaopen_FONT, 1);
    luaL_requiref(L, "MODE", luaopen_MODE, 1);
    luaL_requiref(L, "ICON_TYPE", luaopen_ICON_TYPE, 1);
    luaL_requiref(L, "UNIT", luaopen_UNIT, 1);
    luaL_requiref(L, "DEPENDS_ON", luaopen_DEPENDS_ON, 1);
    
    //just return true
    lua_pushboolean(L, 1);
    return 1;
}
