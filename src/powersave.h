#ifndef _powersave_h_
#define _powersave_h_

/* reset the powersave timer (as if you would press a button) */
void powersave_prolong();

/* disable powersave timer */
void powersave_prohibit();

/* re-enable powersave timer */
void powersave_permit();

#endif
