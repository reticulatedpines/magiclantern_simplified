#ifndef _picstyle_h_
#define _picstyle_h_

int get_prop_picstyle_from_index(int index);
int get_prop_picstyle_index(int pic_style);

/* todo: move them to picstyle.c */
const char * get_picstyle_name(int raw_picstyle);
const char * get_picstyle_shortname(int raw_picstyle);

int lens_get_sharpness(void);
int lens_get_contrast(void);
int lens_get_saturation(void);
int lens_get_color_tone(void);

void lens_set_sharpness(int value);
void lens_set_contrast(int value);
void lens_set_saturation(int value);
void lens_set_color_tone(int value);

int lens_get_from_other_picstyle_sharpness(int index);
int lens_get_from_other_picstyle_contrast(int index);
int lens_get_from_other_picstyle_saturation(int index);
int lens_get_from_other_picstyle_color_tone(int index);

#ifdef PROP_PICSTYLE_SETTINGS
    #error this should no longer be in consts.h
#endif

#if NUM_PICSTYLES == 9      /* old cameras */
    #define PROP_PICSTYLE_SETTINGS(i) (PROP_PICSTYLE_SETTINGS_STANDARD - 1 + i)
#elif NUM_PICSTYLES == 10   /* new cameras also have the "Auto" picture style */
    #define PROP_PICSTYLE_SETTINGS(i) ((i) == 1 ? PROP_PICSTYLE_SETTINGS_AUTO : PROP_PICSTYLE_SETTINGS_STANDARD - 2 + i)
#endif

#endif
