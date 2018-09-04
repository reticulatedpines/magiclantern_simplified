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
};

struct lua_prop_msg
{
    unsigned property;
    void * value;
    unsigned len;
};

int lua_prop_task_running = 0;
static struct lua_prop * prop_handlers = NULL;

//TODO: create a new task per script so that scripts don't block each other's prop handlers, necessary?
static void lua_prop_task(int unused)
{
    lua_prop_queue = msg_queue_create("lua_prop_queue", 1);
    TASK_LOOP
    {
        struct lua_prop_msg * msg = NULL;
        int err = msg_queue_receive(lua_prop_queue, &msg, 0);
        
        if(err || !msg) continue;
        
        struct lua_prop * lua_prop = NULL;
        int found = 0;
        for(lua_prop = prop_handlers; lua_prop; lua_prop = lua_prop->next)
        {
            if(lua_prop->prop_id == msg->property)
            {
                found = 1;
                break;
            }
        }
        
        if(found && msg->value)
        {
            lua_State * L = lua_prop->L;
            struct semaphore * sem = NULL;
            if (lua_take_semaphore(L, 1000, &sem) == 0)
            {
                ASSERT(sem);
                if(lua_rawgeti(L, LUA_REGISTRYINDEX, lua_prop->prop_handler_ref) == LUA_TFUNCTION)
                {
                    lua_rawgeti(L, LUA_REGISTRYINDEX, lua_prop->self_ref);
                    if(msg->len > 4)
                    {
                        //long, probably a string
                        ((char*)(msg->value))[msg->len] = 0x0;
                        lua_pushstring(L, (char*)(msg->value));
                    }
                    else if(msg->len == 4) lua_pushinteger(L, *((uint32_t*)(msg->value)));
                    else if(msg->len >= 2) lua_pushinteger(L, *((uint16_t*)(msg->value)));
                    else lua_pushinteger(L, *((uint8_t*)(msg->value)));
                    if(docall(L, 2, 0))
                    {
                        fprintf(stderr, "[Lua] prop handler failed:\n %s\n", lua_tostring(L, -1));
                        lua_save_last_error(L);
                    }
                }
                give_semaphore(sem);
            }
            else
            {
                printf("[Lua] semaphore timeout: prop handler %d (%dms)\n", lua_prop->prop_id, 1000);
            }
        }
        free(msg->value);
        free(msg);
    }
}

static void lua_prophandler(unsigned property, void * priv, void * addr, unsigned len)
{
    if (!lua_prop_queue) return;
    struct lua_prop_msg * msg = malloc(sizeof(struct lua_prop_msg));
    msg->property = property;
    msg->len = MIN(len,255);
    msg->value = malloc(msg->len + 1);
    if(msg->value)
    {
        memcpy(msg->value, addr, MIN(len,255));
        msg_queue_post(lua_prop_queue, (uint32_t)msg);
    }
    else
    {
        fprintf(stderr, "[Lua] lua_prophandler: malloc error");
    }
}

static void lua_register_prop_handler(unsigned prop_id)
{
    if(!lua_prop_task_running)
    {
        lua_prop_task_running = 1;
        task_create("lua_prop_task", 0x1c, 0x10000, lua_prop_task, 0);
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
    lua_newtable(L);
    lua_pushcfunction(L, luaCB_property_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaCB_property_newindex);
    lua_setfield(L, -2, "__newindex");
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
    /// @tfield property LV_AF_RESULT
    else if(!strcmp(name, "LV_AF_RESULT")) prop_id = PROP_LV_AF_RESULT;
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
    /// @tfield property PICTURE_STYLE
    else if(!strcmp(name, "PICTURE_STYLE")) prop_id = PROP_PICTURE_STYLE;
    /// @tfield property PC_FLAVOR1_PARAM
    else if(!strcmp(name, "PC_FLAVOR1_PARAM")) prop_id = PROP_PC_FLAVOR1_PARAM;
    /// @tfield property PC_FLAVOR2_PARAM
    else if(!strcmp(name, "PC_FLAVOR2_PARAM")) prop_id = PROP_PC_FLAVOR2_PARAM;
    /// @tfield property PC_FLAVOR3_PARAM
    else if(!strcmp(name, "PC_FLAVOR3_PARAM")) prop_id = PROP_PC_FLAVOR3_PARAM;
    /// @tfield property STROBO_FIRING
    else if(!strcmp(name, "STROBO_FIRING")) prop_id = PROP_STROBO_FIRING;
    /// @tfield property STROBO_ETTLMETER
    else if(!strcmp(name, "STROBO_ETTLMETER")) prop_id = PROP_STROBO_ETTLMETER;
    /// @tfield property STROBO_CURTAIN
    else if(!strcmp(name, "STROBO_CURTAIN")) prop_id = PROP_STROBO_CURTAIN;
    /// @tfield property STROBO_AECOMP
    else if(!strcmp(name, "STROBO_AECOMP")) prop_id = PROP_STROBO_AECOMP;
    /// @tfield property STROBO_SETTING
    else if(!strcmp(name, "STROBO_SETTING")) prop_id = PROP_STROBO_SETTING;
    /// @tfield property STROBO_REDEYE
    else if(!strcmp(name, "STROBO_REDEYE")) prop_id = PROP_STROBO_REDEYE;
    /// @tfield property POPUP_BUILTIN_FLASH
    else if(!strcmp(name, "POPUP_BUILTIN_FLASH")) prop_id = PROP_POPUP_BUILTIN_FLASH;
    /// @tfield property LCD_BRIGHTNESS
    else if(!strcmp(name, "LCD_BRIGHTNESS")) prop_id = PROP_LCD_BRIGHTNESS;
    /// @tfield property LCD_BRIGHTNESS_MODE
    else if(!strcmp(name, "LCD_BRIGHTNESS_MODE")) prop_id = PROP_LCD_BRIGHTNESS_MODE;
    /// @tfield property LCD_BRIGHTNESS_AUTO_LEVEL
    else if(!strcmp(name, "LCD_BRIGHTNESS_AUTO_LEVEL")) prop_id = PROP_LCD_BRIGHTNESS_AUTO_LEVEL;
    /// @tfield property STROBO_FIRING
    else if(!strcmp(name, "STROBO_FIRING")) prop_id = PROP_STROBO_FIRING;
    /// @tfield property DOF_PREVIEW_MAYBE
    else if(!strcmp(name, "DOF_PREVIEW_MAYBE")) prop_id = PROP_DOF_PREVIEW_MAYBE;
    /// @tfield property REMOTE_SW1
    else if(!strcmp(name, "REMOTE_SW1")) prop_id = PROP_REMOTE_SW1;
    /// @tfield property REMOTE_SW2
    else if(!strcmp(name, "REMOTE_SW2")) prop_id = PROP_REMOTE_SW2;
    /// @tfield property PROGRAM_SHIFT
    else if(!strcmp(name, "PROGRAM_SHIFT")) prop_id = PROP_PROGRAM_SHIFT;
    /// @tfield property QUICKREVIEW
    else if(!strcmp(name, "QUICKREVIEW")) prop_id = PROP_QUICKREVIEW;
    /// @tfield property QUICKREVIEW_MODE
    else if(!strcmp(name, "QUICKREVIEW_MODE")) prop_id = PROP_QUICKREVIEW_MODE;
    /// @tfield property REMOTE_AFSTART_BUTTON
    else if(!strcmp(name, "REMOTE_AFSTART_BUTTON")) prop_id = PROP_REMOTE_AFSTART_BUTTON;
    /// @tfield property REMOTE_BULB_RELEASE_END
    else if(!strcmp(name, "REMOTE_BULB_RELEASE_END")) prop_id = PROP_REMOTE_BULB_RELEASE_END;
    /// @tfield property REMOTE_BULB_RELEASE_START
    else if(!strcmp(name, "REMOTE_BULB_RELEASE_START")) prop_id = PROP_REMOTE_BULB_RELEASE_START;
    /// @tfield property REMOTE_RELEASE
    else if(!strcmp(name, "REMOTE_RELEASE")) prop_id = PROP_REMOTE_RELEASE;
    /// @tfield property REMOTE_SET_BUTTON
    else if(!strcmp(name, "REMOTE_SET_BUTTON")) prop_id = PROP_REMOTE_SET_BUTTON;
    /// @tfield property FA_ADJUST_FLAG
    else if(!strcmp(name, "FA_ADJUST_FLAG")) prop_id = PROP_FA_ADJUST_FLAG;
    /// @tfield property CARD_SELECT
    else if(!strcmp(name, "CARD_SELECT")) prop_id = PROP_CARD_SELECT;
    /// @tfield property FOLDER_NUMBER_A
    else if(!strcmp(name, "FOLDER_NUMBER_A")) prop_id = PROP_FOLDER_NUMBER_A;
    /// @tfield property FILE_NUMBER_A
    else if(!strcmp(name, "FILE_NUMBER_A")) prop_id = PROP_FILE_NUMBER_A;
    /// @tfield property CLUSTER_SIZE_A
    else if(!strcmp(name, "CLUSTER_SIZE_A")) prop_id = PROP_CLUSTER_SIZE_A;
    /// @tfield property FREE_SPACE_A
    else if(!strcmp(name, "FREE_SPACE_A")) prop_id = PROP_FREE_SPACE_A;
    /// @tfield property CARD_RECORD_A
    else if(!strcmp(name, "CARD_RECORD_A")) prop_id = PROP_CARD_RECORD_A;
    /// @tfield property FOLDER_NUMBER_B
    else if(!strcmp(name, "FOLDER_NUMBER_B")) prop_id = PROP_FOLDER_NUMBER_B;
    /// @tfield property FILE_NUMBER_B
    else if(!strcmp(name, "FILE_NUMBER_B")) prop_id = PROP_FILE_NUMBER_B;
    /// @tfield property CLUSTER_SIZE_B
    else if(!strcmp(name, "CLUSTER_SIZE_B")) prop_id = PROP_CLUSTER_SIZE_B;
    /// @tfield property FREE_SPACE_B
    else if(!strcmp(name, "FREE_SPACE_B")) prop_id = PROP_FREE_SPACE_B;
    /// @tfield property CARD_RECORD_B
    else if(!strcmp(name, "CARD_RECORD_B")) prop_id = PROP_CARD_RECORD_B;
    /// @tfield property FOLDER_NUMBER_C
    else if(!strcmp(name, "FOLDER_NUMBER_C")) prop_id = PROP_FOLDER_NUMBER_C;
    /// @tfield property FILE_NUMBER_C
    else if(!strcmp(name, "FILE_NUMBER_C")) prop_id = PROP_FILE_NUMBER_C;
    /// @tfield property CLUSTER_SIZE_C
    else if(!strcmp(name, "CLUSTER_SIZE_C")) prop_id = PROP_CLUSTER_SIZE_C;
    /// @tfield property FREE_SPACE_C
    else if(!strcmp(name, "FREE_SPACE_C")) prop_id = PROP_FREE_SPACE_C;
    /// @tfield property CARD_RECORD_C
    else if(!strcmp(name, "CARD_RECORD_C")) prop_id = PROP_CARD_RECORD_C;
    /// @tfield property USER_FILE_PREFIX
    else if(!strcmp(name, "USER_FILE_PREFIX")) prop_id = PROP_USER_FILE_PREFIX;
    /// @tfield property SELECTED_FILE_PREFIX
    else if(!strcmp(name, "SELECTED_FILE_PREFIX")) prop_id = PROP_SELECTED_FILE_PREFIX;
    /// @tfield property CARD_COVER
    else if(!strcmp(name, "CARD_COVER")) prop_id = PROP_CARD_COVER;
    /// @tfield property TERMINATE_SHUT_REQ
    else if(!strcmp(name, "TERMINATE_SHUT_REQ")) prop_id = PROP_TERMINATE_SHUT_REQ;
    /// @tfield property BUTTON_ASSIGNMENT
    else if(!strcmp(name, "BUTTON_ASSIGNMENT")) prop_id = PROP_BUTTON_ASSIGNMENT;
    /// @tfield property PIC_QUALITY
    else if(!strcmp(name, "PIC_QUALITY")) prop_id = PROP_PIC_QUALITY;
    /// @tfield property PIC_QUALITY2
    else if(!strcmp(name, "PIC_QUALITY2")) prop_id = PROP_PIC_QUALITY2;
    /// @tfield property PIC_QUALITY3
    else if(!strcmp(name, "PIC_QUALITY3")) prop_id = PROP_PIC_QUALITY3;
    /// @tfield property IMAGE_REVIEW_TIME
    else if(!strcmp(name, "IMAGE_REVIEW_TIME")) prop_id = PROP_IMAGE_REVIEW_TIME;
    /// @tfield property BATTERY_REPORT
    else if(!strcmp(name, "BATTERY_REPORT")) prop_id = PROP_BATTERY_REPORT;
    /// @tfield property BATTERY_HISTORY
    else if(!strcmp(name, "BATTERY_HISTORY")) prop_id = PROP_BATTERY_HISTORY;
    /// @tfield property BATTERY_CHECK
    else if(!strcmp(name, "BATTERY_CHECK")) prop_id = PROP_BATTERY_CHECK;
    /// @tfield property BATTERY_POWER
    else if(!strcmp(name, "BATTERY_POWER")) prop_id = PROP_BATTERY_POWER;
    /// @tfield property AE_MODE_MOVIE
    else if(!strcmp(name, "AE_MODE_MOVIE")) prop_id = PROP_AE_MODE_MOVIE;
    /// @tfield property WINDCUT_MODE
    else if(!strcmp(name, "WINDCUT_MODE")) prop_id = PROP_WINDCUT_MODE;
    /// @tfield property SCREEN_COLOR
    else if(!strcmp(name, "SCREEN_COLOR")) prop_id = PROP_SCREEN_COLOR;
    /// @tfield property ROLLING_PITCHING_LEVEL
    else if(!strcmp(name, "ROLLING_PITCHING_LEVEL")) prop_id = PROP_ROLLING_PITCHING_LEVEL;
    /// @tfield property VRAM_SIZE_MAYBE
    else if(!strcmp(name, "VRAM_SIZE_MAYBE")) prop_id = PROP_VRAM_SIZE_MAYBE;
    /// @tfield property ICU_AUTO_POWEROFF
    else if(!strcmp(name, "ICU_AUTO_POWEROFF")) prop_id = PROP_ICU_AUTO_POWEROFF;
    /// @tfield property AUTO_POWEROFF_TIME
    else if(!strcmp(name, "AUTO_POWEROFF_TIME")) prop_id = PROP_AUTO_POWEROFF_TIME;
    /// @tfield property REBOOT
    else if(!strcmp(name, "REBOOT")) prop_id = PROP_REBOOT;
    /// @tfield property DIGITAL_ZOOM_RATIO
    else if(!strcmp(name, "DIGITAL_ZOOM_RATIO")) prop_id = PROP_DIGITAL_ZOOM_RATIO;
    /// @tfield property INFO_BUTTON_FUNCTION
    else if(!strcmp(name, "INFO_BUTTON_FUNCTION")) prop_id = PROP_INFO_BUTTON_FUNCTION;
    /// @tfield property PROP_LIVE_VIEW_AF_SYSTEM
    else if(!strcmp(name, "PROP_LIVE_VIEW_AF_SYSTEM")) prop_id = PROP_LIVE_VIEW_AF_SYSTEM;
    /// @tfield property PROP_CONTINUOUS_AF
    else if(!strcmp(name, "PROP_CONTINUOUS_AF")) prop_id = PROP_CONTINUOUS_AF;
    /// @tfield property PROP_MOVIE_SERVO_AF
    else if(!strcmp(name, "PROP_MOVIE_SERVO_AF")) prop_id = PROP_MOVIE_SERVO_AF;
    /// @tfield property PROP_MOVIE_SERVO_AF_VALID
    else if(!strcmp(name, "PROP_MOVIE_SERVO_AF_VALID")) prop_id = PROP_MOVIE_SERVO_AF_VALID;
    /// @tfield property PROP_SHUTTER_AF_DURING_RECORD
    else if(!strcmp(name, "PROP_SHUTTER_AF_DURING_RECORD")) prop_id = PROP_SHUTTER_AF_DURING_RECORD;
    /// @tfield property REGISTRATION_DATA_UPDATE_FUNC
    else if(!strcmp(name, "REGISTRATION_DATA_UPDATE_FUNC")) prop_id = PROP_REGISTRATION_DATA_UPDATE_FUNC;
    /// @tfield property LIMITED_TV_VALUE_AT_AUTOISO
    else if(!strcmp(name, "LIMITED_TV_VALUE_AT_AUTOISO")) prop_id = PROP_LIMITED_TV_VALUE_AT_AUTOISO;
    /// @tfield property LOUDNESS_BUILT_IN_SPEAKER
    else if(!strcmp(name, "LOUDNESS_BUILT_IN_SPEAKER")) prop_id = PROP_LOUDNESS_BUILT_IN_SPEAKER;
    /// @tfield property LED_LIGHT
    else if(!strcmp(name, "LED_LIGHT")) prop_id = PROP_LED_LIGHT;
    /// @tfield property AFSHIFT_LVASSIST_STATUS
    else if(!strcmp(name, "AFSHIFT_LVASSIST_STATUS")) prop_id = PROP_AFSHIFT_LVASSIST_STATUS;
    /// @tfield property AFSHIFT_LVASSIST_SHIFT_RESULT
    else if(!strcmp(name, "AFSHIFT_LVASSIST_SHIFT_RESULT")) prop_id = PROP_AFSHIFT_LVASSIST_SHIFT_RESULT;
    /// @tfield property MULTIPLE_EXPOSURE_CTRL
    else if(!strcmp(name, "MULTIPLE_EXPOSURE_CTRL")) prop_id = PROP_MULTIPLE_EXPOSURE_CTRL;
    /// @tfield property MIRROR_DOWN_IN_MOVIE_MODE
    else if(!strcmp(name, "MIRROR_DOWN_IN_MOVIE_MODE")) prop_id = PROP_MIRROR_DOWN_IN_MOVIE_MODE;
    /// @tfield property SHUTTER_COUNTER
    else if(!strcmp(name, "SHUTTER_COUNTER")) prop_id = PROP_SHUTTER_COUNTER;
    /// @tfield property AFPOINT
    else if(!strcmp(name, "AFPOINT")) prop_id = PROP_AFPOINT;
    /// @tfield property BEEP
    else if(!strcmp(name, "BEEP")) prop_id = PROP_BEEP;
    /// @tfield property ELECTRIC_SHUTTER
    else if(!strcmp(name, "ELECTRIC_SHUTTER")) prop_id = PROP_ELECTRIC_SHUTTER;
    /// @tfield property LOGICAL_CONNECT
    else if(!strcmp(name, "LOGICAL_CONNECT")) prop_id = PROP_LOGICAL_CONNECT;
    /// @tfield property BV
    else if(!strcmp(name, "BV")) prop_id = PROP_BV;
    /// @tfield property LV_BV
    else if(!strcmp(name, "LV_BV")) prop_id = PROP_LV_BV;
    /// @tfield property STROBO_CHARGE_INFO_MAYBE
    else if(!strcmp(name, "STROBO_CHARGE_INFO_MAYBE")) prop_id = PROP_STROBO_CHARGE_INFO_MAYBE;
    /// @tfield property ONESHOT_RAW
    else if(!strcmp(name, "ONESHOT_RAW")) prop_id = PROP_ONESHOT_RAW;
    /// @tfield property AEB
    else if(!strcmp(name, "AEB")) prop_id = PROP_AEB;
    
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

static void update_cant_unload(lua_State * L)
{
    //are there any prop handlers for this script left?
    int any_active_handlers = 0;
    struct lua_prop * current;
    for(current = prop_handlers; current; current = current->next)
    {
        if(current->L == L && current->prop_handler_ref != LUA_NOREF)
        {
            any_active_handlers = 1;
            break;
        }
    }
    lua_set_cant_unload(L, any_active_handlers, LUA_PROP_UNLOAD_MASK);
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
                    update_cant_unload(L);
                    return 0;
                }
            }
            if(!lua_isnil(L, 3))
            {
                //script created a prop handler so it can't be unloaded
                lua_set_cant_unload(L, 1, LUA_PROP_UNLOAD_MASK);
                
                lua_register_prop_handler(prop_id);
                //create new prop handler
                current = malloc(sizeof(struct lua_prop));
                if (!current) return luaL_error(L, "malloc error");
                current->L = L;
                current->prop_id = prop_id;
                current->next = prop_handlers;
                prop_handlers = current;
                lua_pushvalue(L, 3);
                current->prop_handler_ref = luaL_ref(L, LUA_REGISTRYINDEX);
                lua_pushvalue(L, 1);
                current->self_ref = luaL_ref(L, LUA_REGISTRYINDEX);
            }
            else
            {
                update_cant_unload(L);
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

static const char * lua_property_fields[] =
{
    //TODO: put all those properties in here
    NULL
};

const luaL_Reg propertylib[] =
{
    {NULL, NULL}
};

LUA_LIB(property)
