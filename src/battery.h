#ifndef _battery_h_
#define _battery_h_

/* return battery level in percentage (0-100) */
int GetBatteryLevel();

/* battery ID, as registered in Canon menu */
int GetBatteryHist();

/* how many "green dots" (3 for a new battery, less for a used battery) */
int GetBatteryPerformance();

/* ML estimations */
int GetBatteryTimeRemaining();
int GetBatteryDrainRate();

/* todo: refactor this one with callbacks */
void RefreshBatteryLevel_1Hz();
#endif
