#ifndef fio_5d3_h
#define fio_5d3_h

#define CARD_A 0
#define CARD_B 1
#define CARD_C 2

struct card_info {
    char * drive_letter;
    char * type;
    int cluster_size;
    int free_space_raw;
    int file_number;
    int folder_number;
};

struct card_info * get_ml_card();
struct card_info * get_shooting_card();

struct card_info * get_card(int cardId);

int get_free_space_32k (const struct card_info * card);

/* returns true if the specified file or directory exists */
int is_file(char* path);
int is_dir(char* path);

/* returns a numbered file name that does not already exist.
 * example:
 * char filename[100];
 * int num;
 * num = get_numbered_file_name("IMG_%04d.BMP", 9999, filename, sizeof(filename));
 * => num = 1234, filename = IMG_1234.BMP
 * 
 * Notes:
 * - numbering starts at 0
 * - zero-byte files are overwritten
 * - if you already have files 0, 1 and 3, it will return 2
 * - if all the files numbered from 0 to nmax are used, the function will return -1 and the filename string will be numbered with 0.
 */
int get_numbered_file_name(const char* pattern, int nmax, char* filename, int maxlen);

#endif
