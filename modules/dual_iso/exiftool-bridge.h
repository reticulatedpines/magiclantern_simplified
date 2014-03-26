#ifndef _EXIFTOOL_BRIDGE_H
#define _EXIFTOOL_BRIDGE_H

void copy_tags_from_source(const char* source, const char* dest);
unsigned int get_model_id(const char* filename);

/*
This function uses EXIF information to calculate the following two ratios:
  Red balance is the ratio G/R for a neutral color (typically > 1)
  Blue balance is the ratio G/B for a neutral color (typically > 1)
Use only on dual ISO shots!
*/
void read_white_balance(const char* filename, float* red_balance, float* blue_balance);

#endif
