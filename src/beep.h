#ifndef _beep_h_
#define _beep_h_

/* beep playing */
void beep();
void beep_custom(int duration, int frequency, int wait);
void beep_times(int times);
void unsafe_beep(); /* also beeps while recording */

/* wav recording */
void WAV_StartRecord(char* filename);
void WAV_StopRecord();
#endif

