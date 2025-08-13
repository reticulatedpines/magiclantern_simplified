#ifndef fio_5d3_h
#define fio_5d3_h

#include <stdio.h>
#include "compiler.h"

#define CARD_A 0
#define CARD_B 1
#define CARD_C 2

struct card_info {
    char * drive_letter;
    char * type;            /* SD/CF/EXT */
    int cluster_size;
    int free_space_raw;
    int file_number;
    int folder_number;
    char * maker;           /* only for some cameras; NULL otherwise */
    char * model;
};

struct card_info * get_ml_card();
struct card_info * get_shooting_card();

struct card_info * get_card(int cardId);

int get_free_space_32k (const struct card_info * card);

/* returns true if the specified file or directory exists */
int is_file(const char* path);
int is_dir(const char* path);

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

/** \name File I/O flags.
 *
 * \note I don't know how many of these are supported
 * @{
 */
#define O_RDONLY             00
#define O_WRONLY             01
#define O_RDWR               02
#define O_CREAT            0100 /* not fcntl */
#define O_EXCL             0200 /* not fcntl */
#define O_NOCTTY           0400 /* not fcntl */
#define O_TRUNC           01000 /* not fcntl */
#define O_APPEND          02000
#define O_NONBLOCK        04000
#define O_NDELAY        O_NONBLOCK
#define O_SYNC           010000
#define O_FSYNC          O_SYNC
#define O_ASYNC          020000
/* @} */

/* whence value for FIO_SeekSkipFile */
#define 	SEEK_SET   0
#define 	SEEK_CUR   1
#define 	SEEK_END   2

/**
 * File mode attributes, for FindFirst/FindNext
 */
#define     ATTR_NORMAL     0x00          /* normal file */ 
#define     ATTR_READONLY   0x01          /* file is readonly */ 
#define     ATTR_HIDDEN     0x02          /* file is hidden */ 
#define     ATTR_SYSTEM     0x04          /* file is a system file */ 
#define     ATTR_VOLUME     0x08          /* entry is a volume label */ 
#define     ATTR_DIRECTORY  0x10          /* entry is a directory name */ 
#define     ATTR_ARCHIVE    0x20          /* file is new or modified */ 

#define FIO_MAX_PATH_LENGTH 0x80

/** We don't know anything about this one. */
struct fio_dirent;

/** Directory entry returned by FIO_FindFirstEx() */
#ifdef CONFIG_DIGIC_78X
struct fio_file {
    uint16_t mode;
    uint16_t unk_01;
    uint32_t unk_02;
    uint32_t size;
    uint32_t unk_03;
    uint32_t timestamp;
    uint32_t timestamp2;
    char name[FIO_MAX_PATH_LENGTH];
    uint32_t unk_04[4];
};
SIZE_CHECK_STRUCT(fio_file, 0x28 + FIO_MAX_PATH_LENGTH);
#elif defined(CONFIG_DIGIC_45) || defined(CONFIG_DIGIC_VI)
struct fio_file {
    uint16_t mode;
    uint16_t unk_01;
    uint32_t size;
    uint32_t timestamp;
    uint32_t unk_02;
    char name[FIO_MAX_PATH_LENGTH];
    uint32_t unk_04[4];
};
SIZE_CHECK_STRUCT(fio_file, 0x20 + FIO_MAX_PATH_LENGTH);
#elif defined(MODULE)
// In a module context, this is an opaque pointer.
// This prevents modules using fields in fio_file, which,
// as can been seen above, differ in offsets on different cams.
// Modules are supposed to be cam independent so this is not allowed.
//
// Modules can convert fio_file to file_info (which has the same field names),
// using convert_fio_file_info().
struct fio_file;
#endif

// Cam independent struct for converting the above cam specific struct
// to a format modules can use.
struct file_info
{
    uint16_t mode;
    uint32_t size;
    uint64_t timestamp;
    char name[FIO_MAX_PATH_LENGTH];
};
SIZE_CHECK_STRUCT(file_info, 0x10 + FIO_MAX_PATH_LENGTH);

// This func allows a module to get the cam to alloc space,
// since the module doesn't know the size.
struct fio_file *alloc_fio_file(void);
struct file_info convert_fio_file_info(struct fio_file *);

// file IO
extern FILE* FIO_OpenFile( const char* filename, unsigned mode );
extern int FIO_ReadFile( FILE* stream, void* ptr, size_t count );
extern int FIO_WriteFile( FILE* stream, const void* ptr, size_t count );
extern void FIO_CloseFile( FILE* stream );

/* Returns 0 for success */
extern int FIO_GetFileSize( const char * filename, uint32_t * size );

extern struct fio_dirent * FIO_FindFirstEx( const char * dirname, struct fio_file * file );
extern int FIO_FindNextEx( struct fio_dirent * dirent, struct fio_file * file );
extern void FIO_FindClose( struct fio_dirent * dirent );
extern int FIO_RenameFile(const char * src, const char * dst);
extern int FIO_RemoveFile(const char * filename);
extern int FIO_GetFileSize(const char * filename, uint32_t * size);
extern uint32_t FIO_GetFileSize_direct(const char * filename);   /* todo: use just this one */

/* returns absolute position after seeking */
/* note: seeking past the end of a file does not work on all cameras */
extern int64_t FIO_SeekSkipFile( FILE* stream, int64_t position, int whence );

/* ML wrappers */
extern FILE* FIO_CreateFile( const char* name );
extern FILE* FIO_CreateFileOrAppend( const char* name );
extern int FIO_CopyFile(const char * src, const char * dst);
extern int FIO_MoveFile(const char * src, const char * dst);   /* copy and erase */

extern int FIO_CreateDirectory(const char * dirname);

/* for ML startup */
void _find_ml_card();
void _card_tweaks();

/* dump anything from RAM to a file */
void save_mem_to_file(void *start, uint32_t size, char *filename);

/* dump 0x10000000 bytes (256MB) from 0x10000000 * k */
void dump_big_seg(int k, char* filename);

size_t read_file( const char * filename, void * buf, size_t size);

uint8_t* read_entire_file(const char * filename, int* buf_size);

const char* get_dcim_dir();
const char* get_dcim_dir_suffix();

extern int __attribute__((format(printf,2,3)))
my_fprintf(
        FILE *                  file,
        const char *            fmt,
        ...
);

#endif
