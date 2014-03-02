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

#endif
