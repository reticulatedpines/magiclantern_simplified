// battery info

#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>
#include <version.h>

#define DISPLAY_BATTERY_LEVEL_1 60 //%
#define DISPLAY_BATTERY_LEVEL_2 20 //%

struct battery_info {
    int num_of_batt; // 1 if battery is in the body, ? if grip attached
    int level;       // 0-100%
    int performance; // battery performancee 0?,1,2,3
    int expo;        // expo taken with this charge
    uint32_t serial; // serial number
    int num_of_hist; // number of registered batterys
    int act_hist;    // the actual one from the registered, 0: if the battery is not registered
    char name[6];    // LP-E6, ???
};

struct battery_history {
  uint32_t serial;   // serial number
  int level;         // 0-100%
  int year;          // year-1900  |
  int month;         // month-1    | the date when the camera last sees the battery
  int day;           // day        |
};

static struct battery_history bat_hist[6];
static struct battery_info bat_info;

static CONFIG_INT("battery.drain.rate.rev", battery_seconds_same_level_ok, 108); // initial estimation: 3 hours battery life
static int battery_seconds_same_level_tmp = 0;
static int battery_level_transitions = 0;

PROP_HANDLER(PROP_BATTERY_REPORT) // also in memory address 7D.203: 7AF60, length 96 bytes
{
    bat_info.level = buf[1] & 0xff;
    bat_info.performance = (buf[1] >> 8) & 0xff;
    bat_info.serial = (buf[5] & 0xff000000) + SWAP_ENDIAN(buf[6] << 8);
    bat_info.num_of_batt = buf[0];
    bat_info.expo = (buf[2] >> 8) & 0xffff; //expo taken with the battery 
    for (int i=0;i<MIN(bat_info.num_of_hist,6);i++) 
       if (bat_hist[i].serial == bat_info.serial) bat_info.act_hist = i+1;
    // from buf[2] >> 24 : battery name (byte 11-...) LP-E6 or ???
}

PROP_HANDLER(PROP_BATTERY_HISTORY) // also in memory address 7D.203: 7AFC0, length 76 bytes
{
    bat_info.num_of_hist = buf[0];
    bat_info.act_hist = 0;
    for (int i=0;i<MIN(bat_info.num_of_hist,6);i++) 
    {
        bat_hist[i].serial = buf[1+i*3];
        if (bat_hist[i].serial == bat_info.serial) bat_info.act_hist = i+1;
        bat_hist[i].level = buf[2+i*3] & 0xffff;
        bat_hist[i].year = (buf[2+i*3] >> 16);
        bat_hist[i].month = buf[3+i*3] & 0xffff;
        bat_hist[i].day = (buf[3+i*3] >> 16);
    }
}

int GetBatteryLevel()
{
    return bat_info.level;
}
int GetBatteryPerformance()
{
    return bat_info.performance;
}
int GetBatteryHist()
{
   return bat_info.act_hist;
}
int GetBatteryTimeRemaining()
{
    return battery_seconds_same_level_ok * bat_info.level;
}
int GetBatteryDrainRate() // percents per hour
{
    return 3600 / battery_seconds_same_level_ok;
}

// called every second
void RefreshBatteryLevel_1Hz()
{
    static int k = 0;
    k++;
    
    if (k % 10 == 0 &&
        lens_info.job_state == 0) // who knows what race conditions are here... I smell one :)
    {
        int x = 31;
        prop_request_change(PROP_BATTERY_REPORT, &x, 1); // see PROP_Request PROP_BATTERY_REPORT
    }
    
    msleep(50);
    
    // check how many seconds battery indicator was at the same percentage
    // this is a rough indication of how fast the battery is draining
    static int old_battery_level = -1;
    if (bat_info.level == old_battery_level)
    {
        battery_seconds_same_level_tmp++;
    }
    else
    {
        battery_level_transitions++;
        
        // first transition is at startup
        // second transition will overestimate the battery drain rate
        // subsequent transitions are OK
        
        if (battery_level_transitions > 2)
            battery_seconds_same_level_ok = battery_seconds_same_level_tmp;
        battery_seconds_same_level_tmp = 0;
    }
    old_battery_level = bat_info.level;
}
