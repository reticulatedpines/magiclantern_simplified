/***
 DryOS operating system functions.
 
 TODO: what about Penlight-like API for file I/O (see e.g.
 [pl.path](http://stevedonovan.github.io/Penlight/api/libraries/pl.path.html),
 [pl.dir](http://stevedonovan.github.io/Penlight/api/libraries/pl.dir.html),
 [pl.file](http://stevedonovan.github.io/Penlight/api/libraries/pl.file.html),
 [pl.utils](http://stevedonovan.github.io/Penlight/api/libraries/pl.utils.html))?
 
 Or maybe even porting a subset of [Penlight](http://stevedonovan.github.io/Penlight/api/manual/01-introduction.md.html)
 and/or [LFS](https://keplerproject.github.io/luafilesystem/manual.html)?
 
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module dryos
 */

#include <dryos.h>
#include <fio-ml.h>
#include <fileprefix.h>
#include <string.h>
#include <config.h>
#include <propvalues.h>

#include "lua_common.h"

static int luaCB_card_index(lua_State * L);
static int luaCB_card_newindex(lua_State * L);
static int luaCB_directory_index(lua_State * L);
static int luaCB_directory_newindex(lua_State * L);
static int luaCB_directory_tostring(lua_State * L);
static int luaCB_card_image_path(lua_State * L);

const char * lua_dryos_directory_fields[] =
{
    "exists",
    "create",
    "children",
    "files",
    "parent",
    NULL
};

const char * lua_dryos_card_fields[] =
{
    "drive_letter",
    "dcim_dir",
    "file_number",
    "folder_number",
    "free_space",
    "image_path",
  //"path",
    "type",
    NULL
};

/***
 Creates a @{directory} object that is used to get information about a directory
 
 This function does not actually create a directory on the file system, it just creates
 an object that represents a directory. To actually create the directory in the file system
 call `directory:create()`.
 @usage
 local mydir = dryos.directory("mydir")
 if mydir.exists == false then
    mydir:create()
 end
 for i,v in ipairs(mydir:files()) do 
    print("filename: "..v)
 end
 @tparam string path
 @treturn directory
 @function directory
 */
static int luaCB_dryos_directory(lua_State * L)
{
    LUA_PARAM_STRING(path, 1);
    lua_newtable(L);
    if (strlen(path) == 0 || !strcmp(path, "/"))
    {
        struct card_info * ml_card = get_ml_card();
        if (ml_card == NULL) return luaL_error(L, "Could not get ML card");
        lua_pushfstring(L, "%s:/", ml_card->drive_letter);
    }
    else if (path[strlen(path) - 1] != '/')
    {
        lua_pushfstring(L, "%s/", path);
    }
    else
    {
        lua_pushvalue(L, 1);
    }
    lua_setfield(L, -2, "path");
    lua_newtable(L);
    lua_pushcfunction(L, luaCB_directory_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pushcfunction(L, luaCB_directory_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaCB_directory_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, luaCB_pairs);
    lua_setfield(L, -2, "__pairs");
    lua_pushlightuserdata(L, lua_dryos_directory_fields);
    lua_setfield(L, -2, "fields");
    lua_setmetatable(L, -2);
    return 1;
}

/***
 Deletes a file from the card.
 @tparam string filename
 @treturn bool success
 @function remove
 */
static int luaCB_dryos_remove(lua_State * L)
{
    LUA_PARAM_STRING(filename, 1);
    lua_pushboolean(L, FIO_RemoveFile(filename) == 0);
    return 1;
}

/***
 Renames/moves a file on the card (or between cards).
 @tparam string filename
 @treturn bool success
 @function rename
 */
static int luaCB_dryos_rename(lua_State * L)
{
    LUA_PARAM_STRING(src, 1);
    LUA_PARAM_STRING(dst, 2);
    int err = FIO_RenameFile(src, dst);
    if (err)
    {
        err = FIO_MoveFile(src, dst);
    }
    lua_pushboolean(L, err == 0);
    return 1;
}

/***
 Calls an eventproc (a function from the camera firmware which can be called by name).
 See [Eventprocs](http://magiclantern.wikia.com/wiki/Call_by_name).

 Dangerous - you need to compile Lua yourself in order to enable it.
 @tparam string function the name of the function to call
 @param[opt] arg argument to pass to the call
 @function call
 */
static int luaCB_dryos_call(lua_State * L)
{
#if 1
    return luaL_error(L,
        "dryos.call() is disabled for safety reasons.\n"
        "If you know what you are doing, just remove this message and recompile.\n"
    );
#endif

    LUA_PARAM_STRING(function_name, 1);
    int result = 0;
    int argc = lua_gettop(L);
    
    if(argc <= 1)
    {
        result = call(function_name);
    }
    else if(lua_isinteger(L, 2))
    {
        int arg = lua_tointeger(L, 2);
        result = call(function_name, arg);
    }
    else if(lua_isnumber(L, 2))
    {
        float arg = lua_tonumber(L, 2);
        result = call(function_name, arg);
    }
    else if(lua_isstring(L, 2))
    {
        const char * arg = lua_tostring(L, 2);
        result = call(function_name, arg);
    }
    
    lua_pushinteger(L, result);
    return 1;
}

static void setfield (lua_State *L, const char *key, int value) {
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
}

static void setboolfield (lua_State *L, const char *key, int value) {
    if (value < 0)  /* undefined? */
        return;  /* does not set field */
    lua_pushboolean(L, value);
    lua_setfield(L, -2, key);
}

static int lua_card_obj(lua_State * L, struct card_info * card)
{
    if(!card) return luaL_error(L, "Card info error");

    char root_path[] = "X:/";
    root_path[0] = card->drive_letter[0];
    if (!is_dir(root_path))
    {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    lua_pushlightuserdata(L, card);
    lua_setfield(L, -2, "_card_ptr");
    lua_pushfstring(L, "%s:/", card->drive_letter);
    lua_setfield(L, -2, "path");
    lua_newtable(L);
    lua_pushcfunction(L, luaCB_card_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaCB_card_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, luaCB_pairs);
    lua_setfield(L, -2, "__pairs");
    lua_pushlightuserdata(L, lua_dryos_card_fields);
    lua_setfield(L, -2, "fields");
    lua_setmetatable(L, -2);
    return 1;
}

static int luaCB_dryos_index(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get the number of the seconds since camera startup.
    // @tfield int clock
    if(!strcmp(key, "clock")) lua_pushinteger(L, get_seconds_clock());
    /// Get the number of milliseconds since camera startup.
    // @tfield int ms_clock
    else if(!strcmp(key, "ms_clock")) lua_pushinteger(L, get_ms_clock());
    /// Get/Set the image filename prefix (e.g.&nbsp;"IMG_").
    ///
    /// Set to empty string to restore default value.
    // @tfield string image_prefix
    else if(!strcmp(key, "image_prefix")) lua_pushstring(L, get_file_prefix());
    /// Get the ML config directory.
    // @tfield directory config_dir
    else if(!strcmp(key, "config_dir"))
    {
        lua_pushcfunction(L, luaCB_dryos_directory);
        lua_pushstring(L, get_config_dir());
        lua_call(L, 1, 1);
    }
    /// Get the card ML was started from.
    // @tfield card ml_card
    else if(!strcmp(key, "ml_card")) return lua_card_obj(L, get_ml_card());
    /// Get the shooting card (the one selected in Canon menu for taking pictures / recording videos).
    // @tfield card shooting_card
    else if(!strcmp(key, "shooting_card")) return lua_card_obj(L, get_shooting_card());
    /// Get the CF card if your camera has one, otherwise nil
    // @tfield ?card|nil cf_card
    else if(!strcmp(key, "cf_card")) return lua_card_obj(L, get_card(CARD_A));
    /// Get the SD card if your camera has one, otherwise nil
    // @tfield ?card|nil sd_card
    else if(!strcmp(key, "sd_card")) return lua_card_obj(L, get_card(CARD_B));
    /// Gets a table representing the current date/time.
    // @tfield date date
    else if(!strcmp(key, "date"))
    {
        /// Represents a date/time
        // @type date
        struct tm tm;
        LoadCalendarFromRTC(&tm);
        lua_newtable(L);
        /// Second
        // @tfield int sec
        setfield(L, "sec", tm.tm_sec);
        /// Minute
        // @tfield int min
        setfield(L, "min", tm.tm_min);
        /// Hour
        // @tfield int hour
        setfield(L, "hour", tm.tm_hour);
        /// Day
        // @tfield int day
        setfield(L, "day", tm.tm_mday);
        /// Month
        // @tfield int month
        setfield(L, "month", tm.tm_mon+1);
        /// Year
        // @tfield int year
        setfield(L, "year", tm.tm_year+1900);
        /// Day of week
        // @tfield int wday
        setfield(L, "wday", tm.tm_wday+1);
        /// Day of year
        // @tfield int yday
        setfield(L, "yday", tm.tm_yday+1);
        /// Daylight Savings
        // @tfield bool isdst
        setboolfield(L, "isdst", tm.tm_isdst);
    }
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_dryos_newindex(lua_State * L)
{
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(!strcmp(key, "clock") || !strcmp(key, "ms_clock") || !strcmp(key, "date") || !strcmp(key, "ml_card") || !strcmp(key, "dcim_dir"))
    {
        return luaL_error(L, "'%s' is readonly!", key);
    }
    else if(!strcmp(key, "image_prefix"))
    {
        static char prefix[8];
        static int prefix_key = 0;
        LUA_PARAM_STRING(new_prefix, 3);

        int len = strlen(new_prefix);
        if (len != 0 && len != 4)
        {
            return luaL_error(L, "invalid prefix length (4 chars, 0 to reset)");
        }

        if (prefix_key)
        {
            file_prefix_reset(prefix_key);
            prefix_key = 0;
        }

        if (len == 4)
        {
            strncpy(prefix, new_prefix, 7);
            prefix_key = file_prefix_set(prefix);
        }
    }
    else
    {
        lua_rawset(L, 1);
    }
    return 0;
}

/// Represents a directory
// @type directory

/***
 Creates a directory.
 @treturn bool whether or not the directory was sucessfully created
 @function create
 */
static int luaCB_directory_create(lua_State * L)
{
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    lua_getfield(L, 1, "path");
    const char * path = lua_tostring(L, -1);
    lua_pushinteger(L, FIO_CreateDirectory(path));
    return 1;
}

/***
 Get a list of subdirectories, as table of @{directory} objects.
 @treturn {directory,...}
 @function children
 */
static int luaCB_directory_children(lua_State * L)
{
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    if(lua_getfield(L, 1, "path") != LUA_TSTRING) return luaL_error(L, "invalid directory path");
    const char * path = lua_tostring(L, -1);
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx(path, &file);
    int index = 1;
    if(!IS_ERROR(dirent))
    {
        lua_newtable(L);
        do
        {
            if (file.mode & ATTR_DIRECTORY)
            {
                if (!streq(file.name, ".") && !streq(file.name, ".."))
                {
                    //call the directory constructor
                    lua_pushcfunction(L, luaCB_dryos_directory);
                    lua_pushfstring(L, "%s%s/", path, file.name);
                    lua_call(L, 1, 1);
                    lua_seti(L, -2, index++);
                }
            }
        }
        while(FIO_FindNextEx(dirent, &file) == 0);
        FIO_FindClose(dirent);
    }
    else
    {
        return luaL_error(L, "error reading directory '%s'", path);
    }
    
    return 1;
}

/***
 Get a list of file names (with full path) in this directory, as table of @{string}s.
 @treturn {string,...}
 @function files
 */
static int luaCB_directory_files(lua_State * L)
{
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    if(lua_getfield(L, 1, "path") != LUA_TSTRING) return luaL_error(L, "invalid directory path");
    const char * path = lua_tostring(L, -1);
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx(path, &file);
    int index = 1;
    if(!IS_ERROR(dirent))
    {
        lua_newtable(L);
        do
        {
            if (!(file.mode & ATTR_DIRECTORY))
            {
                lua_pushfstring(L, "%s%s", path, file.name);
                lua_seti(L, -2, index++);
            }
        }
        while(FIO_FindNextEx(dirent, &file) == 0);
        FIO_FindClose(dirent);
    }
    else
    {
        return luaL_error(L, "error reading directory: '%s'", path);
    }
    
    return 1;
}

static int luaCB_directory_index(lua_State * L)
{
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    /// Get the full path of the directory.
    // @tfield string path
    if(!strcmp(key, "path")) return lua_rawget(L, 1);
    
    if(lua_getfield(L, 1, "path") != LUA_TSTRING) return luaL_error(L, "invalid directory path");
    const char * path = lua_tostring(L, -1);
    lua_pop(L, 1);
    /// Get whether or not the directory exists.
    // @tfield bool exists
    if(!strcmp(key, "exists")) lua_pushboolean(L, is_dir(path));
    else if(!strcmp(key, "create")) lua_pushcfunction(L, luaCB_directory_create);
    else if(!strcmp(key, "children")) lua_pushcfunction(L, luaCB_directory_children);
    else if(!strcmp(key, "files")) lua_pushcfunction(L, luaCB_directory_files);
    /// Get a @{directory} object that represents the current directory's parent
    // @tfield directory parent
    else if(!strcmp(key, "parent"))
    {
        size_t len = strlen(path);
        if ((len > 3 || ((len == 2 || len == 3) && path[1] != ':')) && path[len - 1] == '/')
        {
            char * parent_path = copy_string(path);
            parent_path[len - 1] = 0x0;
            char * last = strrchr(parent_path, '/');
            if (last) *(last + 1) = 0x0;
            else parent_path[0] = 0x0;
            
            //call the directory constructor
            lua_pushcfunction(L, luaCB_dryos_directory);
            lua_pushstring(L, parent_path);
            lua_call(L, 1, 1);
            free(parent_path);
            return 1;
        }
        else
        {
            return 0;
        }
        
    }
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_directory_newindex(lua_State * L)
{
    return luaL_error(L, "'directory' type is readonly");
}

static int luaCB_directory_tostring(lua_State * L)
{
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    lua_getfield(L, 1, "path");
    return 1;
}

/// Represents a card (storage media).
// Inherits from @{directory}
// @type card

static int luaCB_card_image_path(lua_State * L)
{
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    if(lua_getfield(L, 1, "_card_ptr") == LUA_TLIGHTUSERDATA)
    {
        struct card_info * card = lua_touserdata(L, -1);
        LUA_PARAM_INT_OPTIONAL(file_offset, 2, 0);
        LUA_PARAM_STRING_OPTIONAL(extension, 3, 0);

        char image_path[32];    /* B:/DCIM/100CANON/IMG_1234.CR2 */

        int folder_num = card->folder_number;
        int file_num = card->file_number;
        for (int i = 0; i != file_offset; i += SGN(file_offset))
        {
            file_num += SGN(file_offset);
            if (file_num > 9999)
            {
                file_num = 1;
                folder_num++;
                if (folder_num > 999)
                {
                    folder_num = 100;
                }
            }
            else if (file_num < 1)
            {
                file_num = 9999;
                folder_num--;
                if (folder_num < 100)
                {
                    folder_num = 999;
                }
            }
        }

        int raw = pic_quality & 0x60000;

        snprintf(image_path, sizeof(image_path),
            "%s:/DCIM/%03d%s/%s%04d%s",
            card->drive_letter,     /* B */
            folder_num,             /* 100 */
            get_dcim_dir_suffix(),  /* CANON */
            get_file_prefix(),      /* IMG_ */
            file_num,               /* 1234 */
            extension ? extension : raw ? ".CR2" : ".JPG"
        );

        lua_pushstring(L, image_path);
        return 1;
    }
    else
    {
        return luaL_error(L, "could not get lightuserdata for card");
    }
}

static const char * get_card_dcim_dir(struct card_info * card)
{
    static char dcim_dir[FIO_MAX_PATH_LENGTH];
    snprintf(dcim_dir, sizeof(dcim_dir), "%s:/DCIM/%03d%s", card->drive_letter, card->folder_number, get_dcim_dir_suffix());
    return dcim_dir;
}

static int luaCB_card_index(lua_State * L)
{
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    LUA_PARAM_STRING_OPTIONAL(key, 2, "");
    if(lua_getfield(L, 1, "_card_ptr") == LUA_TLIGHTUSERDATA)
    {
        struct card_info * card = lua_touserdata(L, -1);
        /// Get the drive letter (A or B).
        // @tfield string drive_letter
        if(!strcmp(key, "drive_letter")) lua_pushstring(L, card->drive_letter);
        /// Get the DCIM directory for this card.
        // @tfield directory dcim_dir
        else if(!strcmp(key, "dcim_dir"))
        {
            lua_pushcfunction(L, luaCB_dryos_directory);
            lua_pushstring(L, get_card_dcim_dir(card));
            lua_call(L, 1, 1);
        }
        /// Get the current Canon file number (e.g. IMG_1234.CR2 -> 1234).
        // @tfield int file_number
        else if(!strcmp(key, "file_number")) lua_pushinteger(L, card->file_number);
        /// Get the current Canon folder number (e.g. DCIM/101CANON => 101).
        // @tfield int folder_number
        else if(!strcmp(key, "folder_number")) lua_pushinteger(L, card->folder_number);
        /// Get the current free space (in MiB).
        ///
        /// FIXME: does not update after writing files from ML code.
        // @tfield int free_space
        else if(!strcmp(key, "free_space")) lua_pushinteger(L, get_free_space_32k(card) * 32 / 1024);
        /// Get current/previous/future still image path. Examples:
        /// 
        ///  - `B:/DCIM/100CANON/IMG1234.CR2` (with extension)
        ///  - `B:/DCIM/100CANON/IMG1234` (without extension)
        ///  
        // @tparam[opt=0] int file_offset 0 = last saved image, positive = future images, negative = previous images.
        // @tparam[opt=nil] ?string|nil extension Suffix to append to the file name: `".CR2"` or `".JPG"`
        //
        // or `nil` = autodetect from picture quality setting (CR2 for RAW+JPEG).
        // @function image_path
        // @treturn string
        else if(!strcmp(key, "image_path")) lua_pushcfunction(L, luaCB_card_image_path);
        /// Get the type of card (`"SD"` or `"CF"`).
        // @tfield string type
        else if(!strcmp(key, "type")) lua_pushstring(L, card->type);
        else return luaCB_directory_index(L);
    }
    else
    {
        return luaL_error(L, "could not get lightuserdata for card");
    }
    return 1;
}
static int luaCB_card_newindex(lua_State * L)
{
    return luaL_error(L, "'card' type is readonly");
}

static const char * lua_dryos_fields[] =
{
    "clock",
    "ms_clock",
    "image_prefix",
    "config_dir",
    "ml_card",
    "shooting_card",
    "cf_card",
    "sd_card",
    "date",
    NULL
};

const luaL_Reg dryoslib[] =
{
    {"call", luaCB_dryos_call},
    {"directory", luaCB_dryos_directory},
    {"remove", luaCB_dryos_remove},
    {"rename", luaCB_dryos_rename},
    {NULL, NULL}
};

LUA_LIB(dryos)
