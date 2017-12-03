/*
 * Copyright (C) 2014 David Milligan
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

#include <module.h>
#include <dryos.h>
#include <string.h>
#include <math.h>
#include <raw.h>
#include <lens.h>
#include <property.h>
#include <console.h>
#include <beep.h>
#include <fio-ml.h>
#include <shoot.h>
#include <zebra.h>
#include <focus.h>
#include <config.h>
#include <bmp.h>
#include <powersave.h>
#include "lua_common.h"
#include "umm_malloc/umm_malloc.h"

struct lua_script
{
    int argc;
    union
    {
        char * filename;
        char * argv[5];
    };

    char * name;
    char * description;
    char * last_error;
    const char * last_menu_parent;
    const char * last_menu_entry;
    int autorun;
    int state;
    int load_time;
    int cant_unload;
    int cant_yield;
    int tasks_started;
    lua_State * L;
    struct semaphore * sem;
    struct msg_queue * key_mq;
    struct menu_entry * menu_entry;
    struct lua_script * next;
};

static struct lua_script * lua_scripts = NULL;

/* result always valid (1:1 mapping between lua_State's and lua_script's) */
static struct lua_script * lua_script(lua_State * L)
{
    for (struct lua_script * script = lua_scripts; script; script = script->next)
    {
        if (script->L == L)
        {
            return script;
        }
    }

    /* should be unreachable */
    ASSERT(0);
    while(1);
}

struct script_event_entry
{
    struct script_event_entry * next;
    int function_ref;
    lua_State * L;
    int mask;
};

static int lua_loaded = 0;
int last_keypress = 0;
int waiting_for_keypress = -1;  /* 0 = all, -1 = none, other = wait for a specific key */

int lua_take_semaphore(lua_State * L, int timeout, struct semaphore ** assoc_semaphore)
{
    struct lua_script * script = lua_script(L);

    if (assoc_semaphore) *assoc_semaphore = script->sem;
    return take_semaphore(script->sem, timeout);
}

int lua_give_semaphore(lua_State * L, struct semaphore ** assoc_semaphore)
{
    struct lua_script * script = lua_script(L);

    if (assoc_semaphore) *assoc_semaphore = script->sem;
    return give_semaphore(script->sem);
}

int lua_msg_queue_receive(lua_State * L, uint32_t * msg, int timeout)
{
    struct lua_script * script = lua_script(L);

    return msg_queue_receive(script->key_mq, msg, timeout);
}

/*
 Determines if a string ends in some string
 */
static int string_ends_with(const char *source, const char *ending)
{
    if(source == NULL || ending == NULL) return 0;
    if(strlen(source) <= 0) return 0;
    if(strlen(source) < strlen(ending)) return 0;
    return !strcmp(source + strlen(source) - strlen(ending), ending);
}

char * copy_string(const char * str)
{
    size_t len = strlen(str) + 1;
    char * copy = malloc(sizeof(char) * len);
    if(copy) strncpy(copy,str,len);
    return copy;
}

/***
 Event Handlers.
 
 Scripts can respond to events by setting the functions in the 'event' table.
 Event handler functions can take one integer parameter, and must return a boolean
 that specifies whether or not the backend should continue executing event handlers
 for this particular event.
 
 Event handlers will not run if there's already a script or another event handler
 actively executing at the same time.
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module event
 @usage
 event.keypress = function(key)
 print("You pressed a key: "..key)
 return false
 end
 */


static unsigned int lua_do_cbr(unsigned int ctx, struct script_event_entry * event_entries, const char * event_name, int timeout, int sucess, int failure)
{
    //no events registered by lua scripts
    if(!event_entries || !lua_loaded) return sucess;
    
    unsigned int result = sucess;
    struct script_event_entry * current;
    for(current = event_entries; current; current = current->next)
    {
        lua_State * L = current->L;
        if(current->function_ref != LUA_NOREF)
        {
            struct semaphore * sem = NULL;
            if (lua_take_semaphore(L, timeout, &sem) == 0)
            {
                ASSERT(sem);
                if(lua_rawgeti(L, LUA_REGISTRYINDEX, current->function_ref) == LUA_TFUNCTION)
                {
                    lua_pushinteger(L, ctx);
                    if(docall(L, 1, 1))
                    {
                        fprintf(stderr, "[%s] cbr error:\n %s\n", lua_get_script_filename(L), lua_tostring(L, -1));
                        lua_save_last_error(L);
                        result = CBR_RET_ERROR;
                        give_semaphore(sem);
                        break;
                    }
                    else
                    {
                        if(lua_isboolean(L, -1) && !lua_toboolean(L, -1))
                        {
                            lua_pop(L, 1);
                            result = failure;
                            give_semaphore(sem);
                            break;
                        }
                        
                    }
                }
                lua_pop(L,1);
                give_semaphore(sem);
            }
            else
            {
                printf("[%s] semaphore timeout: %s (%dms)\n", lua_get_script_filename(L), event_name, timeout);
            }
        }
    }
    return result;
}

#define LUA_CBR_FUNC(name, arg, timeout)\
static struct script_event_entry * name##_cbr_scripts = NULL;\
static unsigned int lua_##name##_cbr(unsigned int ctx) {\
return lua_do_cbr(arg, name##_cbr_scripts, #name, timeout, CBR_RET_CONTINUE, CBR_RET_STOP);\
}\

LUA_CBR_FUNC(pre_shoot, ctx, 500)
LUA_CBR_FUNC(post_shoot, ctx, 500)
LUA_CBR_FUNC(shoot_task, ctx, 500)
LUA_CBR_FUNC(seconds_clock, ctx, 100)
LUA_CBR_FUNC(custom_picture_taking, ctx, 1000)
LUA_CBR_FUNC(intervalometer, get_interval_count(), 1000)
LUA_CBR_FUNC(config_save, ctx, 1000)

#ifdef CONFIG_VSYNC_EVENTS
LUA_CBR_FUNC(vsync)
LUA_CBR_FUNC(display_filter)
LUA_CBR_FUNC(vsync_setparam)
#endif

static struct script_event_entry * keypress_cbr_scripts = NULL;
static unsigned int lua_keypress_cbr(unsigned int ctx)
{
    /* ignore unknown button codes */
    if (!ctx) return 1;

    last_keypress = ctx;
    //keypress cbr interprets things backwards from other CBRs
    int result = lua_do_cbr(ctx, keypress_cbr_scripts, "keypress", 500, CBR_RET_KEYPRESS_NOTHANDLED, CBR_RET_KEYPRESS_HANDLED);

    if (result == CBR_RET_KEYPRESS_NOTHANDLED)
    {
        /* waiting for this/all keypress/es?
         * send it to the script(s) waiting for a key
         * then block this event */
        if (waiting_for_keypress == last_keypress || waiting_for_keypress == 0)
        {
            for (struct lua_script * script = lua_scripts; script; script = script->next)
            {
                /* note: key.wait() will clear the buffer before starting to wait
                 * also msg_queue_post is not blocking, therefore, sending this
                 * to scripts not waiting for a key press should be harmless. */
                msg_queue_post(script->key_mq, last_keypress);
            }

            /* done waiting */
            waiting_for_keypress = -1;
            return CBR_RET_KEYPRESS_HANDLED;
        }
    }

    return result;
}

#define SCRIPT_CBR_SET(event) \
if(!strcmp(key, #event))\
{\
lua_pushvalue(L, 3);\
set_event_script_entry(&event##_cbr_scripts, L, lua_isfunction(L, -1) ? luaL_ref(L, LUA_REGISTRYINDEX) : LUA_NOREF); \
return 0;\
}\



static void update_event_cant_unload(struct script_event_entry ** root, lua_State * L)
{
    //are there any event handlers for this script left?
    int any_active_handlers = 0;
    struct script_event_entry * current;
    for(current = *root; current; current = current->next)
    {
        if(current->L == L && current->function_ref != LUA_NOREF)
        {
            any_active_handlers = 1;
            break;
        }
    }
    if(*root) lua_set_cant_unload(L, any_active_handlers, (*root)->mask);
}

static void set_event_script_entry(struct script_event_entry ** root, lua_State * L, int function_ref)
{
    struct script_event_entry * current;
    for(current = *root; current; current = current->next)
    {
        if(current->L == L)
        {
            if(current->function_ref != LUA_NOREF)
            {
                luaL_unref(L, LUA_REGISTRYINDEX, current->function_ref);
            }
            current->function_ref = function_ref;
            update_event_cant_unload(root, L);
            return;
        }
    }
    if(function_ref != LUA_NOREF)
    {
        struct script_event_entry * new_entry = calloc(1, sizeof(struct script_event_entry));
        if(new_entry)
        {
            static int event_masks = LUA_EVENT_UNLOAD_MASK;
            new_entry->mask = *root == NULL ? event_masks++ : (*root)->mask;
            new_entry->next = *root;
            new_entry->L = L;
            new_entry->function_ref = function_ref;
            *root = new_entry;
        }
        else
        {
            luaL_error(L, "malloc error creating script event");
        }
    }
    update_event_cant_unload(root, L);
}

static int luaCB_event_index(lua_State * L)
{
    //LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    lua_rawget(L, 1);
    return 1;
}

static int luaCB_event_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    
    // Called before a picture is taken
    // @param arg unused
    // @treturn bool whether or not to continue executing CBRs for this event.
    //
    // Recommended: true (no real reason to block other CBRs here).
    // @function pre_shoot
    SCRIPT_CBR_SET(pre_shoot);
    // Called after a picture is taken
    // @param arg unused
    // @treturn bool whether or not to continue executing CBRs for this event.
    //
    // Recommended: true (no real reason to block other CBRs here).
    // @function post_shoot
    SCRIPT_CBR_SET(post_shoot);
    /// Called periodicaly from shoot_task.
    // @param arg unused
    // @treturn bool whether or not to continue executing CBRs for this event.
    //
    // Recommended: true (no real reason to block other CBRs here).
    // @function shoot_task
    SCRIPT_CBR_SET(shoot_task);
    /// Called once per second.
    /// Should return quickly, because other tasks may rely on timing accuracy.
    // @param arg unused
    // @treturn bool whether or not to continue executing CBRs for this event.
    //
    // Recommended: true (no real reason to block other CBRs here).
    // @function seconds_clock
    SCRIPT_CBR_SET(seconds_clock);
    /// Called when a key is pressed.
    // @tparam int key the key that was pressed, see @{constants.KEY}
    // @treturn bool whether or not to continue executing CBRs for this event.
    //
    // Returning false will prevent other modules and/or Canon firmware from processing this event.
    //
    // For all unhandled events, return true.
    // @function keypress
    SCRIPT_CBR_SET(keypress);
    /// Special types of picture taking (e.g.&nbsp;silent pics)
    /// so intervalometer and other photo taking routines should use that instead of regular pics.
    // @param arg unused
    // @treturn bool whether or not to continue executing CBRs for this event.
    //
    // Recommended: false if your code took a custom picture, true otherwise.
    // @function custom_picture_taking
    SCRIPT_CBR_SET(custom_picture_taking);
    /// Called after a picture is taken with the intervalometer.
    // @tparam int interval_count the current interval count
    // @treturn bool whether or not to continue executing CBRs for this event.
    //
    // Recommended: true (no real reason to block other CBRs here).
    // @function intervalometer
    SCRIPT_CBR_SET(intervalometer);
    /// Called when configs are being saved; save any config data for your script here.
    /// 
    /// This event can be used in simple scripts; in this case, the CBR will be called
    /// after the main script body exits, right before unloading the script.
    /// 
    // @param arg unused
    // @treturn bool whether or not to continue executing CBRs for this event.
    //
    // Recommended: true (no real reason to block other CBRs here).
    // @function config_save
    SCRIPT_CBR_SET(config_save);
#ifdef CONFIG_VSYNC_EVENTS
    SCRIPT_CBR_SET(display_filter);
    SCRIPT_CBR_SET(vsync);
    SCRIPT_CBR_SET(vsync_setparam);
#endif
    
    lua_rawset(L, 1);
    return 0;
}

static const char * lua_event_fields[] =
{
    "pre_shoot",
    "post_shoot",
    "shoot_task",
    "seconds_clock",
    "keypress",
    "custom_picture_taking",
    "intervalometer",
    "config_save",
    NULL
};

static const luaL_Reg eventlib[] =
{
    { NULL, NULL }
};

LUA_LIB(event)

static const luaL_Reg alllibs[] =
{
    {LUA_COLIBNAME, luaopen_coroutine},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_IOLIBNAME, luaopen_io},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_UTF8LIBNAME, luaopen_utf8},
    {LUA_DBLIBNAME, luaopen_debug},
    {"console", luaopen_console},
    {"camera", luaopen_camera},
    {"lv", luaopen_lv},
    {"lens", luaopen_lens},
    {"movie", luaopen_movie},
    {"display", luaopen_display},
    {"key", luaopen_key},
    {"menu", luaopen_menu},
    {"event", luaopen_event},
    {"dryos", luaopen_dryos},
    {"interval", luaopen_interval},
    {"battery", luaopen_battery},
    {"task", luaopen_task},
    {"property", luaopen_property},
    {"constants", luaopen_constants},
    {"MODE", luaopen_MODE},
    {"ICON_TYPE", luaopen_ICON_TYPE},
    {"UNIT", luaopen_UNIT},
    {"DEPENDS_ON", luaopen_DEPENDS_ON},
    {"FONT", luaopen_FONT},
    {"COLOR", luaopen_COLOR},
    {"KEY", luaopen_KEY},
    {NULL,NULL}
};

static int luaCB_global_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    const luaL_Reg *lib;
    for (lib = alllibs; lib->func; lib++)
    {
        if(!strcmp(key, lib->name))
        {
            luaL_requiref(L, lib->name, lib->func, 1);
            return 1;
        }
    }
    return 0;
}

int do_lua_next(lua_State * L)
{
    int r = lua_type(L, 1) == LUA_TTABLE && lua_next(L, 1);
    if (!r)
    {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 2;
}

int luaCB_next(lua_State * L)
{
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, "fields");
    const char ** fields = (const char **)lua_touserdata(L, -1);
    lua_pop(L, 2);
    
    if(lua_isnil(L, 2))
    {
        if(fields && fields[0])
        {
            lua_pushstring(L, fields[0]);
        }
        else
        {
            lua_pushvalue(L, 2);
            return do_lua_next(L);
        }
    }
    else if(lua_type(L, 2) == LUA_TSTRING)
    {
        LUA_PARAM_STRING(key, 2);
        int found = 0;
        for(int i = 0; fields[i]; i++)
        {
            if(!strcmp(key, fields[i]))
            {
                if(fields && fields[i+1])
                {
                    lua_pushstring(L, fields[i+1]);
                    found = true;
                    break;
                }
                else
                {
                    lua_pushnil(L);
                    return do_lua_next(L);
                }
            }
        }
        if(!found)
        {
            lua_pushvalue(L, 2);
            return do_lua_next(L);
        }
    }
    else
    {
        lua_pushvalue(L, 2);
        return do_lua_next(L);
    }
    lua_pushvalue(L, -1);
    lua_gettable(L, 1);
    return 2;
}

int luaCB_pairs(lua_State * L)
{
    lua_pushcfunction(L, luaCB_next);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    return 3;
}

/* from lua/lua.c */
static void createargtable (lua_State *L, char **argv, int argc, int script) {
  int i, narg;
  if (script == argc) script = 0;  /* no script name? */
  narg = argc - (script + 1);  /* number of positive indices */
  lua_createtable(L, narg, script + 1);
  for (i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - script);
  }
  lua_setglobal(L, "arg");
}

/* from lua/lua.c */
static int pushargs (lua_State *L) {
  int i, n;
  if (lua_getglobal(L, "arg") != LUA_TTABLE)
    luaL_error(L, "'arg' is not a table");
  n = (int)luaL_len(L, -1);
  luaL_checkstack(L, n + 3, "too many arguments to script");
  for (i = 1; i <= n; i++)
    lua_rawgeti(L, -i, i);
  lua_remove(L, -i);  /* remove table from the stack */
  return n;
}

static lua_State * load_lua_state(int argc, char** argv)
{
    lua_State* L = luaL_newstate();
    luaL_requiref(L, "_G", luaopen_base, 1);
    luaL_requiref(L, LUA_LOADLIBNAME, luaopen_package, 1);
    luaL_requiref(L, "globals", luaopen_globals, 0);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 0);
    
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
    const luaL_Reg *lib;
    for (lib = alllibs; lib->func; lib++)
    {
        lua_pushcfunction(L, lib->func);
        lua_setfield(L, -2, lib->name);
    }
    lua_pop(L, 1);
    
    lua_getglobal(L, "_G");
    lua_newtable(L);
    lua_pushcfunction(L, luaCB_global_index);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);
    lua_pop(L, 1);
    
    /* preload strict.lua once, since it will be used in all scripts */
    int bufsize;
    static char * strict_lua = (void*) 0xFFFFFFFF;
    if (strict_lua == (void*) 0xFFFFFFFF)
    {
        strict_lua = (char*) read_entire_file(SCRIPTS_DIR "/lib/strict.lua", &bufsize);
        
        if (!strict_lua)
        {
            /* allow scripts to run without strict.lua, if not present */
            printf("[Lua] warning: strict.lua not found.\n");
        }
        
        /* note: strict_lua is never freed */
    }
    
    if (strict_lua)
    {
        if (luaL_loadstring(L, strict_lua) || docall(L, 0, LUA_MULTRET))
        {
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
        }
    }

    createargtable(L, argv, argc, 0);

    return L;
}

#define SCRIPT_FLAG_AUTORUN_ENABLED "LEN"

#define SCRIPT_STATE_NOT_RUNNING            0
#define SCRIPT_STATE_LOADING_OR_RUNNING     1
#define SCRIPT_STATE_RUNNING_IN_BACKGROUND  2

/* note: when specifying LUA_TASK_UNLOAD_MASK,
 * the caller must have started or stopped one task (not more, not less)
 * because this routine also keeps a counter of how many tasks were started
 */
void lua_set_cant_unload(lua_State * L, int cant_unload, int mask)
{
    struct lua_script * script = lua_script(L);

    if (mask & LUA_TASK_UNLOAD_MASK)
    {
        /* the script started or stopped one task */
        script->tasks_started += (cant_unload ? 1 : -1);
        
        if (!cant_unload && script->tasks_started)
        {
            /* if there are still tasks running,
             * we cannot allow unloading yet */
            mask &= ~LUA_TASK_UNLOAD_MASK;
        }
    }

    if(cant_unload)
    {
        script->cant_unload |= (1 << mask);
    }
    else
    {
        script->cant_unload &= ~(1 << mask);
    }
}

static void lua_clear_last_error(struct lua_script * script)
{
    if (script->last_error)
    {
        free(script->last_error);
        script->last_error = 0;
    }
}

void lua_save_last_error(lua_State * L)
{
    struct lua_script * script = lua_script(L);

    lua_clear_last_error(script);
    script->last_error = copy_string(lua_tostring(L, -1));
}

/* called when a script registers a menu,
 * so we know to tell the user where the new menu is
 */
void lua_set_last_menu(lua_State * L, const char * parent_menu, const char * menu_entry)
{
    struct lua_script * script = lua_script(L);

    printf("[%s] menu: %s - %s\n", lua_get_script_filename(L), parent_menu, menu_entry);
    script->last_menu_parent = parent_menu;
    script->last_menu_entry = menu_entry;
}

/* hack to prevent some unsafe yield calls */
/* fixme: proper thread safety */
void lua_set_cant_yield(lua_State * L, int cant_yield)
{
    struct lua_script * script = lua_script(L);

    script->cant_yield = cant_yield;
}

int lua_get_cant_yield(lua_State * L)
{
    struct lua_script * script = lua_script(L);

    return script->cant_yield;
}

const char * lua_get_script_filename(lua_State * L)
{
    struct lua_script * script = lua_script(L);

    return script->filename;
}

static int lua_get_config_flag_path(struct lua_script * script, char * full_path, const char * flag)
{
    snprintf(full_path, MAX_PATH_LEN, "%s%s", get_config_dir(), script->filename);
    int len = strlen(full_path);
    if(len > 3 && strlen(flag) >= 3)
    {
        memcpy(&(full_path[len - 3]), flag, 3);
        return 1;
    }
    return 0;
}

static void lua_set_flag(struct lua_script * script, const char * flag, int value)
{
    char full_path[MAX_PATH_LEN];
    if(lua_get_config_flag_path(script, full_path, flag))
    {
        config_flag_file_setting_save(full_path, value);
    }
}

static int lua_get_flag(struct lua_script * script, const char * flag)
{
    char full_path[MAX_PATH_LEN];
    if(lua_get_config_flag_path(script, full_path, flag))
    {
        return config_flag_file_setting_load(full_path);
    }
    return 0;
}

static void load_script(struct lua_script * script)
{
    if(script->L)
    {
        fprintf(stderr, "[%s] script is already running.\n", script->filename);
        return;
    }

    /* disable powersave during the initial load, or while a simple script is running */
    powersave_prohibit();

    script->load_time = get_seconds_clock();
    script->state = SCRIPT_STATE_LOADING_OR_RUNNING;
    lua_State* L = script->L = load_lua_state(script->argc, script->argv);
    script->cant_unload = 0;
    script->cant_yield = 0;
    script->tasks_started = 0;
    lua_clear_last_error(script);
    
    if (!script->sem)
    {
        /* create semaphore on first run */
        script->sem = create_named_semaphore(script->filename, 0);
        ASSERT(script->sem);
    }

    if (!script->key_mq)
    {
        /* create key message queue on first run */
        script->key_mq = (struct msg_queue *) msg_queue_create(script->filename, 1);
        ASSERT(script->key_mq);
    }
    
    char full_path[MAX_PATH_LEN];
    snprintf(full_path, MAX_PATH_LEN, SCRIPTS_DIR "/%s", script->filename);
    printf("[%s] script starting.\n", script->filename);

    int status = luaL_loadfile(L, full_path);
    if (status == LUA_OK) {
        int n = pushargs(L);  /* push arguments to script */
        status = docall(L, n, LUA_MULTRET);
    }

    if (status != LUA_OK)
    {
        /* error loading script */
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_save_last_error(L);
    }

    give_semaphore(script->sem);

    /* allow simple scripts to use config_save events */
    if (config_save_cbr_scripts && (script->cant_unload & (1 << config_save_cbr_scripts->mask)))
    {
        printf("[%s] saving config...\n", script->filename);

        /* any config_save hook? call it now */
        /* fixme: this will call the config_save event for all other running scripts; important? */
        unsigned int save_result = lua_do_cbr(0, config_save_cbr_scripts, "config_save", 5000, 0, 0);

        if (save_result == CBR_RET_ERROR)
        {
            /* error calling the config_save event */
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
            lua_save_last_error(L);
        }

        /* allow unloading the script even if it has a config_save event - we have already handled it */
        lua_set_cant_unload(script->L, 0, config_save_cbr_scripts->mask);
    }

    if (script->cant_unload)
    {
        /* "complex" script that keeps running after load
         * set autorun and hide the "run script" menu
         */
        script->state = SCRIPT_STATE_RUNNING_IN_BACKGROUND;
        script->menu_entry->icon_type = IT_BOOL;

        printf("[%s] running in background.\n", script->filename);
    }
    else
    {
        /* "simple" script didn't create a menu, start a task,
         * or register any event handlers, so we can safely unload it
         */

        /* unregister the config_save event, if any */
        set_event_script_entry(&config_save_cbr_scripts, L, LUA_NOREF);

        lua_close(L);
        script->L = NULL;
        script->menu_entry->icon_type = IT_ACTION;
        script->state = SCRIPT_STATE_NOT_RUNNING;
        script->load_time = 0;
        printf("[%s] script finished.\n\n", script->filename);
    }

    /* script finished or loaded in background; allow auto power off */
    powersave_permit();
}

static MENU_UPDATE_FUNC(script_print_state)
{
    struct lua_script * script = (struct lua_script *)(entry->priv);
    if (!script) return;

    if (script->last_error)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "%s", script->last_error);
        return;
    }

    switch (script->state)
    {
        case SCRIPT_STATE_NOT_RUNNING:
            MENU_SET_WARNING(MENU_WARN_INFO, "Press SET to load/run this script.");
            break;

        case SCRIPT_STATE_LOADING_OR_RUNNING:
            MENU_SET_WARNING(MENU_WARN_INFO, "Script is running.");
            break;

        case SCRIPT_STATE_RUNNING_IN_BACKGROUND:
            if (script->last_menu_parent && script->last_menu_entry)
            {
                MENU_SET_WARNING(MENU_WARN_INFO,
                    "Running in background. Menu: %s -> %s.",
                    script->last_menu_parent, script->last_menu_entry
                );
            }
            else
            {
                MENU_SET_WARNING(MENU_WARN_INFO,
                    "Running in background. Complex script%s%s%s%s%s.",
                    script->cant_unload & (1<<LUA_TASK_UNLOAD_MASK)   ? "; task running" : "",
                    script->cant_unload & (1<<LUA_LVINFO_UNLOAD_MASK) ? "; LVInfo item" : "",
                    script->cant_unload & (1<<LUA_PROP_UNLOAD_MASK)   ? "; property handler" : "",
                    script->cant_unload & 0xFFFFFFF0                  ? "; event handler" : "",
                    script->cant_unload & (1<<LUA_MENU_UNLOAD_MASK)   ? "; menu item" : ""
                );
            }
            break;
    }
}

static MENU_UPDATE_FUNC(lua_script_menu_update)
{
    if(!lua_loaded)
    {
        MENU_SET_WARNING(MENU_WARN_NOT_WORKING, "Please wait for Lua to finish loading...");
        return;
    }

    struct lua_script * script = (struct lua_script *)(entry->priv);
    if(script)
    {
        MENU_SET_VALUE("");
        MENU_SET_HELP(script->description);
        MENU_SET_ENABLED(1);

        if (script->autorun)
        {
            MENU_SET_WARNING(MENU_WARN_INFO, "This script will autorun on next boot.");
        }
        else
        {
            script_print_state(entry, info);
        }
        
        const char * script_status = 
            script->autorun
                ? "AUTORUN" :
            script->last_error
                ? "ERROR" :
            script->state == SCRIPT_STATE_NOT_RUNNING
                ? script->filename :
            script->state == SCRIPT_STATE_LOADING_OR_RUNNING
                ? "Running" :
            script->state == SCRIPT_STATE_RUNNING_IN_BACKGROUND
                ? "Running (BG)" : "?!";
        
        if (info->can_custom_draw)
        {
            int fg = (script->state || script->autorun || script->last_error)
                ? COLOR_WHITE : entry->selected ? COLOR_GRAY(50) : COLOR_GRAY(10);
            int fnt = SHADOW_FONT(FONT(FONT_MED_LARGE, fg, COLOR_BLACK));
            bmp_printf(fnt | FONT_ALIGN_RIGHT, 680, info->y+2, script_status);
        }
        else
        {
            MENU_SET_RINFO(script_status);
        }

        switch (script->state)
        {
            case SCRIPT_STATE_LOADING_OR_RUNNING:
                MENU_SET_ICON(MNI_RECORD, 0);
                break;

            case SCRIPT_STATE_RUNNING_IN_BACKGROUND:
                MENU_SET_ICON(MNI_ON, 1);
                break;
        }
    }
}

static void lua_user_load_task(struct lua_script * script)
{
    ASSERT(script->filename == script->argv[0]);
    load_script(script);
}

static MENU_SELECT_FUNC(lua_script_menu_select)
{
    struct lua_script * script = (struct lua_script *)priv;
    ASSERT(script); if (!script) return;
    
    /* attempt to start the script */
    if ( script->state == SCRIPT_STATE_NOT_RUNNING )
    {
        if (lua_loaded)
        {
            console_clear();
            script->state = SCRIPT_STATE_LOADING_OR_RUNNING;
            task_create("lua_user_load_task", 0x1c, 0x10000, lua_user_load_task, script);
            return;
        }
    }
}

static MENU_UPDATE_FUNC(lua_script_run_update)
{
    struct lua_script * script = (struct lua_script *)(entry->priv);
    if (!script) return;

    MENU_SET_VALUE("");

    script_print_state(entry, info);
    
    if (script->state != SCRIPT_STATE_NOT_RUNNING)
    {
        info->warning_level = MENU_WARN_NOT_WORKING;
    }
}


static MENU_SELECT_FUNC(lua_script_toggle_autorun)
{
    // toggle auto_run (priv = &script->autorun)
    // note: any script can be set to autorun
    struct lua_script * script = (struct lua_script *)(priv - offsetof(struct lua_script, autorun));
    script->autorun = !script->autorun;
    lua_set_flag(script, SCRIPT_FLAG_AUTORUN_ENABLED, script->autorun);
}

static MENU_SELECT_FUNC(lua_script_edit)
{
    struct lua_script * script = (struct lua_script *)priv;
    ASSERT(script); if (!script) return;

    struct lua_script * editor = 0;
    for (struct lua_script * current = lua_scripts; current; current = current->next)
    {
        if (strcasecmp(current->filename, "EDITOR.LUA") == 0)
        {
            editor = current;
            break;
        }
    }

    if (!editor)
    {
        console_show();
        printf("[Lua] could not find EDITOR.LUA.");
        return;
    }
    
    /* attempt to start the script */
    if ( editor->state == SCRIPT_STATE_NOT_RUNNING && lua_loaded)
    {
        static char full_path[MAX_PATH_LEN];
        snprintf(full_path, MAX_PATH_LEN, SCRIPTS_DIR "/%s", script->filename);

        editor->argc = 2;
        editor->argv[1] = full_path;

        editor->state = SCRIPT_STATE_LOADING_OR_RUNNING;
        task_create("lua_user_load_task", 0x1c, 0x10000, lua_user_load_task, editor);
    }
    else
    {
        console_show();
        printf("[Lua] could not start EDITOR.LUA.");
    }
}

static MENU_UPDATE_FUNC(menu_no_value)
{
    MENU_SET_VALUE("");
}

static struct menu_entry script_menu_template = {
    .icon_type  = IT_ACTION,
    .select = menu_open_submenu,
    .update = lua_script_menu_update,
};

static struct menu_entry script_submenu_template[] = {
    {
        .name       = "Run Script",
        .select     = lua_script_menu_select,
        .update     = lua_script_run_update,
        .icon_type  = IT_ACTION,
        .help       = "Press SET to load/run this script."
    },
    {
        .name       = "Edit Script",
        .select     = lua_script_edit,
        .update     = menu_no_value,
        .icon_type  = IT_ACTION,
        .help       = "Load this script in the text editor (EDITOR.LUA)."
    },
    {
        .name       = "Autorun",
        .select     = lua_script_toggle_autorun,
        .max        = 1,
        .help       = "Select whether this script will be loaded at camera startup."
    },
    MENU_EOL,
};

/* from console.c */
extern int console_visible;

static MENU_SELECT_FUNC(console_toggle)
{
    if (console_visible)
    {
        console_hide();
    }
    else
    {
        console_show();
    }
}

static struct menu_entry script_console_menu[] = {
    {
        .name       = "Show console",
        .select     = console_toggle,
        .priv       = &console_visible,
        .max        = 1,
        .help       = "Show/hide script console."
    },
};

/* extract script name/description from comments */
/* (it allocates the output buffer and can optionally use a prefix) */
static char* script_extract_string_from_comments(char* buf, char** output, const char* prefix)
{
    char* c = buf;
    
    /* skip Lua comment markers and spaces */
    while (isspace(*c) || *c == '-' || *c == '[')
    {
        c++;
    }

    /* extract script title/description (the current line) */
    char* p = strchr(c, '\n');
    if (!p) return c;
    
    *p = 0;
    *output = copy_string(c);
    *p = '\n';
    
    /* strip spaces and Lua comment markers from the end of the string, if any */
    c = *output + strlen(*output) - 1;
    while (isspace(*c) || *c == '-' || *c == '[')
    {
        *c = 0;
        c--;
        if (c == *output) break; 
    }
    
    /* use a prefix, if any */
    if (prefix)
    {
        char* old = *output;
        uint32_t maxlen = strlen(old) + strlen(*output) + 2 + 1;
        *output = malloc(maxlen);
        snprintf(*output, maxlen, "%s: %s", prefix, old);
        free(old);
    }
    
    return p;
}

static void script_get_name_from_comments(const char * filename, char ** p_name, char ** p_description)
{
    ASSERT(p_name);
    ASSERT(p_description);

    *p_name = 0;
    *p_description = 0;

    char full_path[MAX_PATH_LEN];
    snprintf(full_path, MAX_PATH_LEN, SCRIPTS_DIR "/%s", filename);
    
    char buf[256];
    FILE* f = fopen(full_path, "r");
    ASSERT(f); if (!f) return;
    fread(buf, 1, sizeof(buf), f);
    buf[sizeof(buf)-1] = 0;
    fclose(f);
    
    /* extract name and description */
    char* c = script_extract_string_from_comments(buf, p_name, 0);

    /* name too long? shorten it and use the full string as description */
    char * name = *p_name;
    if (name && bmp_string_width(FONT_LARGE, name) > 450)
    {
        script_extract_string_from_comments(buf, p_description, filename);

        /* attempt to break the string at spaces, commas or whatever */
        int len = strlen(name) - 3;
        while (bmp_string_width(FONT_LARGE, name) > 450)
        {
            for (len--; len > 0; len--)
            {
                if (!isalnum(name[len]))
                {
                    name[len] = name[len+1] = name[len+2] = '.';
                    name[len+3] = 0;
                    break;
                }
            }
        }
    }
    else
    {
        /* first line short => OK, use it as script name and next line as description */
        script_extract_string_from_comments(c, p_description, filename);
    }
}

static void add_script(const char * filename)
{
    struct lua_script * new_script = calloc(1, sizeof(struct lua_script));
    if (!new_script) goto err;

    new_script->argc = 1;
    new_script->filename = copy_string(filename);
    if (!new_script->filename) goto err;

    new_script->L = NULL;
    new_script->next = lua_scripts;
    lua_scripts = new_script;
    
    new_script->menu_entry = calloc(1, sizeof(struct menu_entry));
    if (!new_script->menu_entry) goto err;
    
    memcpy(new_script->menu_entry, &script_menu_template, sizeof(script_menu_template));
    script_get_name_from_comments(new_script->filename, &new_script->name, &new_script->description);
    new_script->menu_entry->name = new_script->name ? new_script->name : new_script->filename;
    new_script->menu_entry->priv = new_script;
    new_script->menu_entry->children = calloc(COUNT(script_submenu_template), sizeof(script_submenu_template[0]));
    if (!new_script->menu_entry->children) goto err;

    memcpy(new_script->menu_entry->children, script_submenu_template, COUNT(script_submenu_template) * sizeof(script_submenu_template[0]));
    new_script->menu_entry->children[0].priv = new_script;
    new_script->menu_entry->children[1].priv = new_script;
    new_script->menu_entry->children[2].priv = &new_script->autorun;
    menu_add("Scripts", new_script->menu_entry, 1);
    return;

err:
    if (new_script)
    {
        if (new_script->menu_entry)
        {
            free(new_script->menu_entry->children);
            free(new_script->menu_entry);
        }
        free(new_script->filename);
        free(new_script);
    }
    fprintf(stderr, "[Lua] add_script: malloc error\n");
}

static void lua_do_autoload()
{
    for (struct lua_script * script = lua_scripts; script; script = script->next)
    {
        if(lua_get_flag(script, SCRIPT_FLAG_AUTORUN_ENABLED))
        {
            if (!console_visible)
            {
                /* only show Lua script loading messages at startup (on demand) */
                console_clear();
                console_show();
            }
            script->autorun = 1;
            load_script(script);
            msleep(100);
        }
    }
}

static void lua_load_task(int unused)
{
    int console_was_visible = console_visible;

    /* wait until other modules (hopefully) finish loading */
    msleep(500);

    char script_names[64][16];
    int num_scripts = 0;

    struct fio_file file;
    struct fio_dirent * dirent = 0;

    dirent = FIO_FindFirstEx(SCRIPTS_DIR, &file);
    if(!IS_ERROR(dirent))
    {
        do
        {
            if (!(file.mode & ATTR_DIRECTORY) &&
                 (string_ends_with(file.name, ".LUA") ||
                  string_ends_with(file.name, ".lua")) &&
                 file.name[0] != '.' && file.name[0] != '_'
            )
            {
                if (num_scripts < COUNT(script_names))
                {
                    if (strlen(file.name) < sizeof(script_names[0]))
                    {
                        strcpy(script_names[num_scripts++], file.name);
                    }
                    else
                    {
                        fprintf(stderr, "[Lua] skipping %s (file name too long).\n", file.name);
                    }
                }
                else
                {
                    fprintf(stderr, "[Lua] skipping %s (too many scripts).\n", file.name);
                }
            }
        }
        while(FIO_FindNextEx(dirent, &file) == 0);
        FIO_FindClose(dirent);
    }

    char aux[sizeof(script_names[0])];
    
    for (int i = 0; i < num_scripts; i++)
    {
        for (int j = i + 1; j < num_scripts; j++)
        {
            if (strcmp(script_names[i], script_names[j]) > 0)
            {
                strcpy(aux, script_names[i]);
                strcpy(script_names[i], script_names[j]);
                strcpy(script_names[j], aux);
            }
        }
    }

    for (int i = 0; i < num_scripts; i++)
    {
        add_script(script_names[i]);
    }

    menu_add("Scripts", script_console_menu, COUNT(script_console_menu));
    lua_loaded = 1;
    
    lua_do_autoload();
    
    extern int core_reallocs;    /* ml-lua-shim.c */
    extern int core_reallocs_size;
    printf("[Lua] free umm_heap : %s\n", format_memory_size(umm_free_heap_size()));
    if (core_reallocs)
    {
        printf("[Lua] core reallocs : %d (%s)\n", core_reallocs, format_memory_size(core_reallocs_size));
    }
    printf("[Lua] all scripts loaded.\n");

    if (console_visible && !console_was_visible)
    {
        /* did we pop the console?
         * wait for key pressed or for 5-second timeout,
         * whichever comes first, then hide it. */
        last_keypress = 0;
        waiting_for_keypress = 0;
        for (int i = 0; i < 50 && !last_keypress; i++)
        {
            msleep(100);
        }
        waiting_for_keypress = -1;
        console_hide();
    }
}

static unsigned int lua_init()
{
    task_create("lua_load_task", 0x1c, 0x10000, lua_load_task, (void*) 0);
    return 0;
}

static unsigned int lua_deinit()
{
    return 0;
}

MODULE_INFO_START()
MODULE_INIT(lua_init)
MODULE_DEINIT(lua_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
MODULE_CBR(CBR_PRE_SHOOT, lua_pre_shoot_cbr, 0)
MODULE_CBR(CBR_POST_SHOOT, lua_post_shoot_cbr, 0)
MODULE_CBR(CBR_SHOOT_TASK, lua_shoot_task_cbr, 0)
MODULE_CBR(CBR_SECONDS_CLOCK, lua_seconds_clock_cbr, 0)
MODULE_CBR(CBR_KEYPRESS, lua_keypress_cbr, 0)
MODULE_CBR(CBR_CUSTOM_PICTURE_TAKING, lua_custom_picture_taking_cbr, 0)
MODULE_CBR(CBR_INTERVALOMETER, lua_intervalometer_cbr, 0)
MODULE_CBR(CBR_CONFIG_SAVE, lua_config_save_cbr, 0)

#ifdef CONFIG_VSYNC_EVENTS
MODULE_CBR(CBR_VSYNC, lua_vsync_cbr, 0)
MODULE_CBR(CBR_DISPLAY_FILTER, lua_display_filter_cbr, 0)
MODULE_CBR(CBR_VSYNC_SETPARAM, lua_vsync_setparam_cbr, 0)
#endif

MODULE_CBRS_END()


