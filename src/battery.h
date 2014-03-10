#ifndef _battery_h_
#define _battery_h_

/* return battery level in percentage (0-100) */
int GetBatteryLevel();

/* ML estimations */
int GetBatteryTimeRemaining();
int GetBatteryDrainRate();

/* todo: refactor this one with callbacks */
void RefreshBatteryLevel_1Hz();
#endif
