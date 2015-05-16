/***
 Canon Properties
 
 @usage
 function property.SHUTTER:handler(value)
     print("shutter: "..value)
 end
 @author Magic Lantern Team
 @copyright 2014
 @license GPL
 @module property
 */

#include <dryos.h>
#include <string.h>
#include <property.h>
#include <propvalues.h>

#include "lua_common.h"

// !!!DANGER WILL ROBINSON!!!
//#define LUA_PROP_REQUEST_CHANGE

struct msg_queue * lua_prop_queue = NULL;

struct lua_prop
{
    struct lua_prop * next;
    lua_State * L;
    int self_ref;
    unsigned prop_id;
    int prop_handler_ref;
    unsigned prop_len;
    void * prop_value;
};

int lua_prop_task_running = 0;
static struct lua_prop * prop_handlers = NULL;

//TODO: create a new task per script so that scripts don't block each other's prop handlers, necessary?
static void lua_prop_task(int unused)
{
    lua_prop_queue = msg_queue_create("lua_prop_queue", 1);
    TASK_LOOP
    {
        struct lua_prop * lua_prop = NULL;
        int err = msg_queue_receive(lua_prop_queue, &lua_prop, 0);
        
        if(err || !lua_prop) continue;
        
        lua_State * L = lua_prop->L;
        struct semaphore * sem = NULL;
        if(!lua_take_semaphore(L, 1000, &sem) && sem)
        {
            if(lua_rawgeti(L, LUA_REGISTRYINDEX, lua_prop->prop_handler_ref) == LUA_TFUNCTION)
            {
                lua_rawgeti(L, LUA_REGISTRYINDEX, lua_prop->self_ref);
                if(lua_prop->prop_len > 4)
                {
                    //long, probably a string
                    size_t len = MIN(lua_prop->prop_len, 255);
                    char * str = malloc(len + 1);
                    memcpy(str, lua_prop->prop_value, len);
                    str[len] = 0x0;
                    lua_pushstring(L, str);
                    free(str);
                }
                else if(lua_prop->prop_len == 4) lua_pushinteger(L, *((uint32_t*)(lua_prop->prop_value)));
                else if(lua_prop->prop_len >= 2) lua_pushinteger(L, *((uint16_t*)(lua_prop->prop_value)));
                else lua_pushinteger(L, *((uint8_t*)(lua_prop->prop_value)));
                if(docall(L, 2, 0))
                {
                    console_printf("script prop handler failed:\n %s\n", lua_tostring(L, -1));
                }
            }
            give_semaphore(sem);
        }
        else
        {
            console_printf("lua semaphore timeout (another task is running this script)\n");
        }
    }
}

static void lua_prophandler(unsigned property, void * priv, void * addr, unsigned len)
{
    struct lua_prop * current;
    for(current = prop_handlers; current; current = current->next)
    {
        if(current->prop_id == property)
        {
            current->prop_len = len;
            if(!current->prop_value) current->prop_value = malloc(len);
            memcpy(current->prop_value, addr, len);
            msg_queue_post(lua_prop_queue, (uint32_t)current);
        }
    }
}

static void lua_register_prop_handler(unsigned prop_id)
{
    if(!lua_prop_task_running)
    {
        lua_prop_task_running = 1;
        task_create("lua_prop_task", 0x1c, 0x8000, lua_prop_task, 0);
    }
    //check for existing prop handler
    struct lua_prop * current;
    for(current = prop_handlers; current; current = current->next)
    {
        if(current->prop_id == prop_id) return;
    }
    prop_add_handler(prop_id, &lua_prophandler);
    prop_update_registration();
}

static int luaCB_property_index(lua_State * L);
static int luaCB_property_newindex(lua_State * L);

static void create_lua_property(lua_State * L, unsigned prop_id)
{
    lua_newtable(L);
    lua_pushinteger(L, prop_id);
    lua_setfield(L, -2, "_id");
    lua_pushcfunction(L, luaCB_property_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaCB_property_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);
}

static int handle_prop_string(lua_State * L, const char * name)
{
    unsigned prop_id = 0;
    
    /// @tfield property BURST_COUNT
    if(!strcmp(name, "BURST_COUNT")) prop_id = PROP_BURST_COUNT;
    /// @tfield property BAT_INFO
    else if(!strcmp(name, "BAT_INFO")) prop_id = PROP_BAT_INFO;
    /// @tfield property TFT_STATUS
    else if(!strcmp(name, "TFT_STATUS")) prop_id = PROP_TFT_STATUS;
    /// @tfield property LENS_NAME
    else if(!strcmp(name, "LENS_NAME")) prop_id = PROP_LENS_NAME;
    /// @tfield property LENS_SOMETHING
    else if(!strcmp(name, "LENS_SOMETHING")) prop_id = PROP_LENS_SOMETHING;
    /// @tfield property LENS
    else if(!strcmp(name, "LENS")) prop_id = PROP_LENS;
    /// @tfield property HDMI_CHANGE
    else if(!strcmp(name, "HDMI_CHANGE")) prop_id = PROP_HDMI_CHANGE;
    /// @tfield property HDMI_CHANGE_CODE
    else if(!strcmp(name, "HDMI_CHANGE_CODE")) prop_id = PROP_HDMI_CHANGE_CODE;
    /// @tfield property USBRCA_MONITOR
    else if(!strcmp(name, "USBRCA_MONITOR")) prop_id = PROP_USBRCA_MONITOR;
    /// @tfield property MVR_REC_START
    else if(!strcmp(name, "MVR_REC_START")) prop_id = PROP_MVR_REC_START;
    /// @tfield property REC_TIME
    else if(!strcmp(name, "REC_TIME")) prop_id = PROP_REC_TIME;
    /// @tfield property GUI_STATE
    else if(!strcmp(name, "GUI_STATE")) prop_id = PROP_GUI_STATE;
    /// @tfield property LIVE_VIEW_FACE_AF
    else if(!strcmp(name, "LIVE_VIEW_FACE_AF")) prop_id = PROP_LIVE_VIEW_FACE_AF;
    /// @tfield property LV_LOCK
    else if(!strcmp(name, "LV_LOCK")) prop_id = PROP_LV_LOCK;
    /// @tfield property LV_ACTION
    else if(!strcmp(name, "LV_ACTION")) prop_id = PROP_LV_ACTION;
    /// @tfield property LCD_POSITION
    else if(!strcmp(name, "LCD_POSITION")) prop_id = PROP_LCD_POSITION;
    /// @tfield property USBDEVICE_CONNECT
    else if(!strcmp(name, "USBDEVICE_CONNECT")) prop_id = PROP_USBDEVICE_CONNECT;
    /// @tfield property MVR_MOVW_START0
    else if(!strcmp(name, "MVR_MOVW_START0")) prop_id = PROP_MVR_MOVW_START0;
    /// @tfield property MVR_MOVW_START1
    else if(!strcmp(name, "MVR_MOVW_START1")) prop_id = PROP_MVR_MOVW_START1;
    /// @tfield property AF_MODE
    else if(!strcmp(name, "AF_MODE")) prop_id = PROP_AF_MODE;
    /// @tfield property MVR_REC
    else if(!strcmp(name, "MVR_REC")) prop_id = PROP_MVR_REC;
    /// @tfield property LV_LENS
    else if(!strcmp(name, "LV_LENS")) prop_id = PROP_LV_LENS;
    /// @tfield property LV_LENS_STABILIZE
    else if(!strcmp(name, "LV_LENS_STABILIZE")) prop_id = PROP_LV_LENS_STABILIZE;
    /// @tfield property LV_MANIPULATION
    else if(!strcmp(name, "LV_MANIPULATION")) prop_id = PROP_LV_MANIPULATION;
    /// @tfield property LV_AFFRAME
    else if(!strcmp(name, "LV_AFFRAME")) prop_id = PROP_LV_AFFRAME;
    /// @tfield property LV_FOCUS
    else if(!strcmp(name, "LV_FOCUS")) prop_id = PROP_LV_FOCUS;
    /// @tfield property LV_FOCUS_DONE
    else if(!strcmp(name, "LV_FOCUS_DONE")) prop_id = PROP_LV_FOCUS_DONE;
    /// @tfield property LV_FOCUS_STOP
    else if(!strcmp(name, "LV_FOCUS_STOP")) prop_id = PROP_LV_FOCUS_STOP;
    /// @tfield property LV_FOCUS_BAD
    else if(!strcmp(name, "LV_FOCUS_BAD")) prop_id = PROP_LV_FOCUS_BAD;
    /// @tfield property LV_FOCUS_STATE
    else if(!strcmp(name, "LV_FOCUS_STATE")) prop_id = PROP_LV_FOCUS_STATE;
    /// @tfield property LV_FOCUS_STATUS
    else if(!strcmp(name, "LV_FOCUS_STATUS")) prop_id = PROP_LV_FOCUS_STATUS;
    /// @tfield property LV_FOCUS_CMD
    else if(!strcmp(name, "LV_FOCUS_CMD")) prop_id = PROP_LV_FOCUS_CMD;
    /// @tfield property LV_FOCUS_DATA
    else if(!strcmp(name, "LV_FOCUS_DATA")) prop_id = PROP_LV_FOCUS_DATA;
    /// @tfield property LV_STATE
    else if(!strcmp(name, "LV_STATE")) prop_id = PROP_LV_STATE;
    /// @tfield property LV_DISPSIZE
    else if(!strcmp(name, "LV_DISPSIZE")) prop_id = PROP_LV_DISPSIZE;
    /// @tfield property LVCAF_STATE
    else if(!strcmp(name, "LVCAF_STATE")) prop_id = PROP_LVCAF_STATE;
    /// @tfield property HALF_SHUTTER
    else if(!strcmp(name, "HALF_SHUTTER")) prop_id = PROP_HALF_SHUTTER;
    /// @tfield property ORIENTATION
    else if(!strcmp(name, "ORIENTATION")) prop_id = PROP_ORIENTATION;
    /// @tfield property LV_LENS_DRIVE_REMOTE
    else if(!strcmp(name, "LV_LENS_DRIVE_REMOTE")) prop_id = PROP_LV_LENS_DRIVE_REMOTE;
    /// @tfield property LIVE_VIEW_VIEWTYPE
    else if(!strcmp(name, "LIVE_VIEW_VIEWTYPE")) prop_id = PROP_LIVE_VIEW_VIEWTYPE;
    /// @tfield property MODE
    else if(!strcmp(name, "MODE")) prop_id = PROP_MODE;
    /// @tfield property DRIVE
    else if(!strcmp(name, "DRIVE")) prop_id = PROP_DRIVE;
    /// @tfield property SHUTTER
    else if(!strcmp(name, "SHUTTER")) prop_id = PROP_SHUTTER;
    /// @tfield property SHUTTER_RANGE
    else if(!strcmp(name, "SHUTTER_RANGE")) prop_id = PROP_SHUTTER_RANGE;
    /// @tfield property APERTURE
    else if(!strcmp(name, "APERTURE")) prop_id = PROP_APERTURE;
    /// @tfield property ISO
    else if(!strcmp(name, "ISO")) prop_id = PROP_ISO;
    /// @tfield property AE
    else if(!strcmp(name, "AE")) prop_id = PROP_AE;
    /// @tfield property UILOCK
    else if(!strcmp(name, "UILOCK")) prop_id = PROP_UILOCK;
    /// @tfield property ISO_AUTO
    else if(!strcmp(name, "ISO_AUTO")) prop_id = PROP_ISO_AUTO;
    /// @tfield property SHUTTER_AUTO
    else if(!strcmp(name, "SHUTTER_AUTO")) prop_id = PROP_SHUTTER_AUTO;
    /// @tfield property APERTURE_AUTO
    else if(!strcmp(name, "APERTURE_AUTO")) prop_id = PROP_APERTURE_AUTO;
    /// @tfield property APERTURE3
    else if(!strcmp(name, "APERTURE3")) prop_id = PROP_APERTURE3;
    /// @tfield property SHUTTER_RELEASE
    else if(!strcmp(name, "SHUTTER_RELEASE")) prop_id = PROP_SHUTTER_RELEASE;
    /// @tfield property AVAIL_SHOT
    else if(!strcmp(name, "AVAIL_SHOT")) prop_id = PROP_AVAIL_SHOT;
    /// @tfield property MIC_INSERTED
    else if(!strcmp(name, "MIC_INSERTED")) prop_id = PROP_MIC_INSERTED;
    /// @tfield property SENSOR
    else if(!strcmp(name, "SENSOR")) prop_id = PROP_SENSOR;
    /// @tfield property DISPSENSOR_CTRL
    else if(!strcmp(name, "DISPSENSOR_CTRL")) prop_id = PROP_DISPSENSOR_CTRL;
    /// @tfield property LV_OUTPUT_DEVICE
    else if(!strcmp(name, "LV_OUTPUT_DEVICE")) prop_id = PROP_LV_OUTPUT_DEVICE;
    /// @tfield property HOUTPUT_TYPE
    else if(!strcmp(name, "HOUTPUT_TYPE")) prop_id = PROP_HOUTPUT_TYPE;
    /// @tfield property MIRROR_DOWN
    else if(!strcmp(name, "MIRROR_DOWN")) prop_id = PROP_MIRROR_DOWN;
    /// @tfield property MYMENU_LISTING
    else if(!strcmp(name, "MYMENU_LISTING")) prop_id = PROP_MYMENU_LISTING;
    /// @tfield property LV_MOVIE_SELECT
    else if(!strcmp(name, "LV_MOVIE_SELECT")) prop_id = PROP_LV_MOVIE_SELECT;
    /// @tfield property SHOOTING_TYPE
    else if(!strcmp(name, "SHOOTING_TYPE")) prop_id = PROP_SHOOTING_TYPE;
    /// @tfield property ERR_BATTERY
    else if(!strcmp(name, "ERR_BATTERY")) prop_id = PROP_ERR_BATTERY;
    /// @tfield property CUSTOM_SETTING
    else if(!strcmp(name, "CUSTOM_SETTING")) prop_id = PROP_CUSTOM_SETTING;
    /// @tfield property DEFAULT_CUSTOM
    else if(!strcmp(name, "DEFAULT_CUSTOM")) prop_id = PROP_DEFAULT_CUSTOM;
    /// @tfield property DEFAULT_BRACKET
    else if(!strcmp(name, "DEFAULT_BRACKET")) prop_id = PROP_DEFAULT_BRACKET;
    /// @tfield property PARTIAL_SETTING
    else if(!strcmp(name, "PARTIAL_SETTING")) prop_id = PROP_PARTIAL_SETTING;
    /// @tfield property EMPOWER_OFF
    else if(!strcmp(name, "EMPOWER_OFF")) prop_id = PROP_EMPOWER_OFF;
    /// @tfield property LVAF_MODE
    else if(!strcmp(name, "LVAF_MODE")) prop_id = PROP_LVAF_MODE;
    /// @tfield property ACTIVE_SWEEP_STATUS
    else if(!strcmp(name, "ACTIVE_SWEEP_STATUS")) prop_id = PROP_ACTIVE_SWEEP_STATUS;
    /// @tfield property EFIC_TEMP
    else if(!strcmp(name, "EFIC_TEMP")) prop_id = PROP_EFIC_TEMP;
    /// @tfield property LANGUAGE
    else if(!strcmp(name, "LANGUAGE")) prop_id = PROP_LANGUAGE;
    /// @tfield property VIDEO_SYSTEM
    else if(!strcmp(name, "VIDEO_SYSTEM")) prop_id = PROP_VIDEO_SYSTEM;
    /// @tfield property DATE_FORMAT
    else if(!strcmp(name, "DATE_FORMAT")) prop_id = PROP_DATE_FORMAT;
    /// @tfield property ICU_UILOCK
    else if(!strcmp(name, "ICU_UILOCK")) prop_id = PROP_ICU_UILOCK;
    /// @tfield property SHOOTING_MODE
    else if(!strcmp(name, "SHOOTING_MODE")) prop_id = PROP_SHOOTING_MODE;
    /// @tfield property SHOOTING_MODE_2
    else if(!strcmp(name, "SHOOTING_MODE_2")) prop_id = PROP_SHOOTING_MODE_2;
    /// @tfield property WB_MODE_LV
    else if(!strcmp(name, "WB_MODE_LV")) prop_id = PROP_WB_MODE_LV;
    /// @tfield property WB_KELVIN_LV
    else if(!strcmp(name, "WB_KELVIN_LV")) prop_id = PROP_WB_KELVIN_LV;
    /// @tfield property WB_MODE_PH
    else if(!strcmp(name, "WB_MODE_PH")) prop_id = PROP_WB_MODE_PH;
    /// @tfield property WB_KELVIN_PH
    else if(!strcmp(name, "WB_KELVIN_PH")) prop_id = PROP_WB_KELVIN_PH;
    /// @tfield property WBS_GM
    else if(!strcmp(name, "WBS_GM")) prop_id = PROP_WBS_GM;
    /// @tfield property WBS_BA
    else if(!strcmp(name, "WBS_BA")) prop_id = PROP_WBS_BA;
    /// @tfield property CUSTOM_WB
    else if(!strcmp(name, "CUSTOM_WB")) prop_id = PROP_CUSTOM_WB;
    /// @tfield property METERING_MODE
    else if(!strcmp(name, "METERING_MODE")) prop_id = PROP_METERING_MODE;
    /// @tfield property LAST_JOB_ID
    else if(!strcmp(name, "LAST_JOB_ID")) prop_id = PROP_LAST_JOB_ID;
    /// @tfield property PROP_PICTURE_STYLE
    else if(!strcmp(name, "PICTURE_STYLE")) prop_id = PROP_PICTURE_STYLE;
    
    if(prop_id)
    {
        create_lua_property(L, prop_id);
        return 1;
    }
    else
    {
        return 0;
    }
}

/// Represents a Canon property
//@type property

/*** 
 Set the value of the property
 
 -----------------------------------------------------------------------------------------------
 __WARNING__ This is very dangerous, setting invalid values for properties can brick your camera.
 This functionality is disabled by default, and will throw an error if you try to call it. 
 To enable, compile with `LUA_PROP_REQUEST_CHANGE`
 
 @tparam int value
 @tparam int len
 @tparam[opt=true] bool wait
 @tparam[opt] int timeout
 @function request_change
 */
static int luaCB_property_request_change(lua_State * L)
{
#ifndef LUA_PROP_REQUEST_CHANGE
    return luaL_error(L, "property request change is disabled");
#else
    if(!lua_istable(L, 1)) return luaL_argerror(L, 1, "expected table");
    if(lua_getfield(L, 1, "_id") != LUA_TNUMBER) return luaL_error(L, "invalid property");
    int prop_id = lua_tointeger(L, -1);
    LUA_PARAM_INT(value, 2);
    LUA_PARAM_INT(len, 3);
    LUA_PARAM_BOOL_OPTIONAL(wait, 4, 1);
    LUA_PARAM_INT_OPTIONAL(timeout, 5, 0);
    if(wait)
    {
        prop_request_change_wait(prop_id, &value, len, timeout);
    }
    else
    {
        prop_request_change(prop_id, &value, len);
    }
    return 0;
#endif
}

static int luaCB_property_index(lua_State * L)
{
    if(lua_isstring(L, 2))
    {
        LUA_PARAM_STRING(key, 2);
        if(!strcmp(key,"request_change"))
        {
            lua_pushcfunction(L, luaCB_property_request_change);
        }
        /// Called when the property value changes
        //@usage
        //function property.SHUTTER:handler(value)
        //    print("shutter: "..value)
        //end
        //@tparam int value the new value of the property
        //@function handler
        else if(!strcmp(key, "handler"))
        {
            if(lua_getfield(L, 1, "_id") != LUA_TNUMBER) return luaL_error(L, "invalid property");
            unsigned prop_id = (unsigned)lua_tointeger(L, -1);
            struct lua_prop * current;
            for(current = prop_handlers; current; current = current->next)
            {
                if(current->L == L && current->prop_id == prop_id)
                {
                    if (current->prop_handler_ref != LUA_NOREF) lua_rawgeti(L, LUA_REGISTRYINDEX, current->prop_handler_ref);
                    break;
                }
            }
        }
        else if(!handle_prop_string(L, key))
        {
            lua_rawget(L, 1);
        }
        
    }
    else if(lua_isinteger(L, 2))
    {
        LUA_PARAM_INT(key, 2);
        create_lua_property(L, key);
    }
    else lua_rawget(L, 1);
    return 1;
}

static int luaCB_property_newindex(lua_State * L)
{
    if(lua_isstring(L, 2))
    {
        if(lua_getfield(L, 1, "_id") != LUA_TNUMBER) return luaL_error(L, "invalid property");
        unsigned prop_id = (unsigned)lua_tointeger(L, -1);
        LUA_PARAM_STRING(key, 2);
        if(!strcmp(key,"handler"))
        {
            if (!(lua_isfunction(L, 3) || lua_isnil(L, 3))) return luaL_error(L, "property handler must be a function");
            //check for existing prop handler
            struct lua_prop * current;
            for(current = prop_handlers; current; current = current->next)
            {
                if(current->L == L && current->prop_id == prop_id)
                {
                    if (current->prop_handler_ref != LUA_NOREF)
                    {
                        luaL_unref(L, LUA_REGISTRYINDEX, current->prop_handler_ref);
                    }
                    lua_pushvalue(L, 3);
                    current->prop_handler_ref = lua_isfunction(L, -1) ? luaL_ref(L, LUA_REGISTRYINDEX) : LUA_NOREF;
                    return 0;
                }
            }
            if(!lua_isnil(L, 3))
            {
                lua_register_prop_handler(prop_id);
                //create new prop handler
                current = malloc(sizeof(struct lua_prop));
                if (!current) return luaL_error(L, "malloc error");
                current->L = L;
                current->prop_id = prop_id;
                current->next = prop_handlers;
                current->prop_value = NULL;
                current->prop_len = 0;
                prop_handlers = current;
                lua_pushvalue(L, 3);
                current->prop_handler_ref = luaL_ref(L, LUA_REGISTRYINDEX);
                lua_pushvalue(L, 1);
                current->self_ref = luaL_ref(L, LUA_REGISTRYINDEX);
            }
        }
        else
        {
            return luaL_error(L,"'%s' is readonly", key);
        }
    }
    else if(lua_isinteger(L, 2))
    {
#ifndef LUA_PROP_REQUEST_CHANGE
        return luaL_error(L, "property request change is disabled");
#else
        LUA_PARAM_INT(key, 2);
        LUA_PARAM_INT(value, 3);
        prop_request_change_wait(key, &value, sizeof(int), 0);
#endif
    }
    else lua_rawset(L, 1);
    return 0;
}

const luaL_Reg propertylib[] =
{
    {NULL, NULL}
};

LUA_LIB(property)