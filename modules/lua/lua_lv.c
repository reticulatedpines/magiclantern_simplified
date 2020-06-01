/***
 LiveView functions
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module lv
 */

#include <dryos.h>
#include <string.h>
#include <shoot.h>
#include <property.h>
#include <zebra.h>
#include <lvinfo.h>
#include <powersave.h>

#include "lua_common.h"


struct lvinfo_item_entry
{
    struct lvinfo_item item;
    lua_State * L;
    int custom_drawing;
    int function_ref;
    int self_ref;
};

static int luaCB_lv_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Whether or not LV is enabled (may be running or paused).
    // @tfield bool enabled
    if(!strcmp(key, "enabled")) lua_pushboolean(L, lv || LV_PAUSED);
    /// Whether or not LV is paused (shutter open, but sensor inactive; useful for powersaving).
    // @tfield bool paused
    else if(!strcmp(key, "paused")) lua_pushboolean(L, LV_PAUSED);
    /// Whether or not LV is running (that is, enabled and not paused).
    // @tfield bool running
    else if(!strcmp(key, "running")) lua_pushboolean(L, lv);
    /// Get/set LiveView zoom factor (1, 5, 10).
    // @tfield bool zoom
    else if(!strcmp(key, "zoom")) lua_pushinteger(L, lv_dispsize);
    /// Get the status of LiveView overlays (false = disabled, 1 = Canon, 2 = ML)
    // @tfield int overlays
    else if(!strcmp(key, "overlays"))
    {
        if (zebra_should_run()) lua_pushinteger(L, 2);
        else if (lv && lv_disp_mode) lua_pushinteger(L, 1);
        else lua_pushboolean(L, 0);
    }
    /// Get the name of current LiveView video mode.
    ///
    /// Examples: MV-1080, MV-720, MVC-1080, REC-1080, ZOOM-X5, PH-LV, PH-QR, PLAY-PH, PLAY-MV...
    //@tfield string vidmode
    else if(!strcmp(key, "vidmode"))
    {
        lua_pushstring(L, get_video_mode_name(0));
    }
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_lv_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "enabled"))
    {
        LUA_PARAM_BOOL(value, 3);
        if(value && !lv && !LV_PAUSED) force_liveview();
        else if(lv) close_liveview();
    }
    else if(!strcmp(key, "zoom"))
    {
        LUA_PARAM_INT(value, 3);

        if (!lv)
        {
            return luaL_error(L, "LiveView must be enabled");
        }

        set_lv_zoom(value);

        if (lv_dispsize != value)
        {
            return luaL_error(L, "Could not set LiveView zoom");
        }
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

/***
 Enter LiveView.
 @function start
 */
static int luaCB_lv_start(lua_State * L)
{
    force_liveview();
    return 0;
}

/***
 Pause LiveView (but leave shutter open).
 @function pause
 */
static int luaCB_lv_pause(lua_State * L)
{
    PauseLiveView();
    return 0;
}

/***
 Resume LiveView.
 @function resume
 */
static int luaCB_lv_resume(lua_State * L)
{
    ResumeLiveView();
    return 0;
}

/***
 Exit LiveView.
 @function stop
 */
static int luaCB_lv_stop(lua_State * L)
{
    close_liveview();
    return 0;
}

/***
 Wait for N frames in LiveView.
 @tparam int num_frames
 @function wait
 */
static int luaCB_lv_wait(lua_State * L)
{
    LUA_PARAM_INT(num_frames, 1);

    int ok = wait_lv_frames(num_frames);

    if (!ok)
    {
        return luaL_error(L,
            lv ? "lv.wait failed (timeout?)"
               : "lv.wait failed (LV stopped)"
        );
    }
    return 0;
}

static LVINFO_UPDATE_FUNC(lua_lvinfo_update)
{
    struct lvinfo_item_entry * entry = (struct lvinfo_item_entry *)item;
    if(!entry) return;
    if(entry->custom_drawing && !can_draw)
    {
        item->custom_drawing = 1;
        return;
    }
    lua_State * L = entry->L;
    struct semaphore * sem = NULL;
    if (lua_take_semaphore(L, 50, &sem) == 0)
    {
        ASSERT(sem);
        if(entry->function_ref != LUA_NOREF)
        {
            if(lua_rawgeti(L, LUA_REGISTRYINDEX, entry->function_ref) == LUA_TFUNCTION)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, entry->self_ref);
                if(docall(L, 1, 1))
                {
                    fprintf(stderr, "[%s] script error:\n %s\n", lua_get_script_filename(L), lua_tostring(L, -1));
                    lua_save_last_error(L);
                }
            }
            lua_pop(L,1);
        }
        give_semaphore(sem);
    }
    else
    {
        printf("[%s] semaphore timeout: lv.info.update (%dms)\n", lua_get_script_filename(L), 50);
    }
}

static int luaCB_lvinfo_index(lua_State * L);
static int luaCB_lvinfo_newindex(lua_State * L);

/***
 Create a new LV info item
 @tparam table definition A table with the inital values for properties of the item
 @treturn lvinfo
 @function info
 */
static int luaCB_lv_info(lua_State * L)
{
    struct lvinfo_item_entry * entry = lua_newuserdata(L, sizeof(struct lvinfo_item_entry));
    if(!entry) return luaL_error(L, "malloc error");
    
    //script created a lvinfo so it can't be unloaded
    lua_set_cant_unload(L, 1, LUA_LVINFO_UNLOAD_MASK);
    
    struct lvinfo_item * item = &(entry->item);
    memset(entry, 0, sizeof(struct lvinfo_item_entry));
    lua_pushvalue(L, -1);
    entry->self_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    entry->L = L;
    lua_newtable(L);
    lua_pushcfunction(L, luaCB_lvinfo_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaCB_lvinfo_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
    
    lua_pushvalue(L, 1);
    
    item->name = LUA_FIELD_STRING("name", "unknown");
    item->value = LUA_FIELD_STRING("value", "unknown");
    item->priority = LUA_FIELD_INT("priority", 0);
    item->preferred_position = LUA_FIELD_INT("preferred_position", 0);
    item->which_bar = LUA_FIELD_INT("which_bar", LV_WHEREVER_IT_FITS);
    entry->custom_drawing = LUA_FIELD_BOOL("custom_drawing", 0);
    
    if(lua_getfield(L, -1, "update") == LUA_TFUNCTION)
    {
        entry->function_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        item->update = lua_lvinfo_update;
    }
    else
    {
        entry->function_ref = LUA_NOREF;
        item->update = NULL;
        lua_pop(L,1);
    }
    
    lvinfo_add_item(item);
    lua_pop(L, 1);
    
    return 1;
}

/***
 Represents an info item that is displayed in the ML info bars in LiveView.
 @type lvinfo
 */

static int luaCB_lvinfo_index(lua_State * L)
{
    if(!lua_isuserdata(L, 1)) return luaL_argerror(L, 1, NULL);
    struct lvinfo_item_entry * entry = lua_touserdata(L, 1);
    if(!entry) return luaL_argerror(L, 1, "internal error: userdata was NULL");
    struct lvinfo_item * item = &(entry->item);
    
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    
    /// Function called before displaying; can override strings, dimensions and so on.
    // @function update
    if(!strcmp(key, "update")) lua_rawgeti(L, LUA_REGISTRYINDEX, entry->function_ref);
    /// Get/Set the item name (for menu).
    // @tfield string name
    else if(!strcmp(key, "name")) lua_pushstring(L, item->name);
    /// Get/Set the item value.
    // @tfield string value
    else if(!strcmp(key, "value")) lua_pushstring(L, item->value);
    /// Get/Set the item background color.
    // @tfield int background see @{constants.COLOR}
    else if(!strcmp(key, "background")) lua_pushinteger(L, item->color_bg);
    /// Get/Set the item foreground color.
    // @tfield int foreground see @{constants.COLOR}
    else if(!strcmp(key, "foreground")) lua_pushinteger(L, item->color_fg);
    /// Get/Set whether the item uses custom drawing.
    // @tfield bool custom_drawing
    else if(!strcmp(key, "custom_drawing")) lua_pushboolean(L, entry->custom_drawing);
    /// Get the item font assigned by the backend.
    // @tfield int font readonly see @{constants.FONT}
    else if(!strcmp(key, "font")) lua_pushinteger(L, item->fontspec);
    /// Get/Set the item height.
    // @tfield int height
    else if(!strcmp(key, "height")) lua_pushinteger(L, item->height);
    /// Get/Set the item's preferred horizontal position (signed integer, look up other `struct lvinfo_item` items in ML source code).
    // @tfield[opt=0] int preferred_position
    else if(!strcmp(key, "preferred_position")) lua_pushinteger(L, item->preferred_position);
    /// Get/Set the item priority: if there's not enough space, the items with low priority will disappear.
    // @tfield[opt=0] int priority
    else if(!strcmp(key, "priority")) lua_pushinteger(L, item->priority);
    /// Get/Set the which bar the item appears on (see enum lvinfo_bar in lvinfo.h).
    ///
    /// TODO: constants.
    // @tfield[opt=anywhere] int bar
    else if(!strcmp(key, "bar")) lua_pushinteger(L, item->which_bar);
    /// Get/Set the item width; default: measured from value and fontspec.
    ///
    /// 0 = do not display this item at all.
    // @tfield int width
    else if(!strcmp(key, "width")) lua_pushinteger(L, item->width);
    /// Get the item x position.
    // @tfield int x readonly
    else if(!strcmp(key, "x")) lua_pushinteger(L, item->x);
    /// Get the item y position.
    // @tfield int y readonly
    else if(!strcmp(key, "y")) lua_pushinteger(L, item->y);
    else lua_rawget(L, 1);
    return 1;
}

static void set_string(char ** entry, const char * value)
{
    if(*entry) free(*entry);
    *entry = copy_string(value);
}

static int luaCB_lvinfo_newindex(lua_State * L)
{
    if(!lua_isuserdata(L, 1)) return luaL_argerror(L, 1, NULL);
    struct lvinfo_item_entry * entry = lua_touserdata(L, 1);
    if(!entry) return luaL_argerror(L, 1, "internal error: userdata was NULL");
    struct lvinfo_item * item = &(entry->item);
    
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    
    if(!strcmp(key, "update"))
    {
        if(entry->function_ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, entry->function_ref);
        if(!lua_isfunction(L, 3))
        {
            entry->function_ref = LUA_NOREF;
            item->update = NULL;
        }
        else
        {
            entry->function_ref = luaL_ref(L, LUA_REGISTRYINDEX);
            item->update = lua_lvinfo_update;
        }
    }
    else if(!strcmp(key, "name"))
    {
        LUA_PARAM_STRING(value, 3);
        set_string(&(item->name),value);
    }
    else if(!strcmp(key, "value"))
    {
        LUA_PARAM_STRING(value, 3);
        set_string(&(item->value),value);
    }
    else if(!strcmp(key, "background"))
    {
        LUA_PARAM_INT(value, 3);
        item->color_bg = value;
    }
    else if(!strcmp(key, "foreground"))
    {
        LUA_PARAM_INT(value, 3);
        item->color_fg = value;
    }
    else if(!strcmp(key, "custom_drawing"))
    {
        LUA_PARAM_BOOL(value, 3);
        entry->custom_drawing = value;
    }
    else if(!strcmp(key, "height"))
    {
        LUA_PARAM_INT(value, 3);
        item->height = value;
    }
    else if(!strcmp(key, "preferred_position"))
    {
        LUA_PARAM_INT(value, 3);
        item->preferred_position = value;
    }
    else if(!strcmp(key, "priority"))
    {
        LUA_PARAM_INT(value, 3);
        item->priority = value;
    }
    else if(!strcmp(key, "bar"))
    {
        LUA_PARAM_INT(value, 3);
        item->which_bar = value;
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

static const char * lua_lv_fields[] =
{
    "enabled",
    "paused",
    "running",
    "zoom",
    "overlays",
    NULL
};

static const luaL_Reg lvlib[] =
{
    { "start", luaCB_lv_start },
    { "pause", luaCB_lv_pause },
    { "resume", luaCB_lv_resume },
    { "stop", luaCB_lv_stop },
    { "wait", luaCB_lv_wait },
    { "info", luaCB_lv_info },
    { NULL, NULL }
};

LUA_LIB(lv)
