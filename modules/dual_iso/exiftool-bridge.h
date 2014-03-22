#ifndef _EXIFTOOL_BRIDGE_H
#define _EXIFTOOL_BRIDGE_H

void copy_tags_from_source(const char* source, const char* dest);
unsigned int get_model_id(const char* filename);
void read_white_balance(const char* filename, float* red_balance, float* blue_balance);

#endif
