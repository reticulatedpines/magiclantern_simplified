/** 
 * Routines for custom file prefix (e.g. IMG_1234.CR2 -> ABCD1234.CR2)
 **/

#ifndef _fileprefix_h_
#define _fileprefix_h_

/* 4-character null-terminated string */
char* get_file_prefix();

/* only one module (task, whatever) can set a custom prefix; all other requests will be denied */
/* returns a "key" with which you can restore the file prefix (so it won't get restored by mistake) */
int file_prefix_set(char* new_prefix);

int file_prefix_reset(int key);

#endif
