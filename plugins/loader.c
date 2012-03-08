#define PLUGIN_CLIENT
#include "plugin.h"

static struct os_command* commands;
static unsigned int num_of_commands;
static unsigned int base_addr;

int __init(struct os_command cmds[], unsigned int num_cmds, unsigned int base) {
	commands = cmds;
	num_of_commands = num_cmds;
	base_addr = base;
	return load_std_functions();
}

void* get_function(unsigned int id) {
	struct os_command * cmd = commands;
	int i = num_of_commands;
	while (i) {
		if (cmd->id == id) return cmd->func;
		i--;
		cmd++;
	}
	return 0;
}

unsigned int get_base_ptr() {
	return base_addr;
}

void* fix_fptr(void* f) {
	return (void*)((char*)f+base_addr);
}

#define IMPORT_FUNC_R( v ) IMPORT_FUNC( v ); if (v) res++;

int load_std_functions() {
	int res = 0;
	// Importing from zebra.h
	IMPORT_FUNC_R( get_global_draw ); // #0x0B00001: int get_global_draw();
	IMPORT_FUNC_R( NotifyBox ); // #0x0B00002: void NotifyBox(int timeout, char* fmt, ...);
	// Importing from config.h
	IMPORT_FUNC_R( get_config_vars_start  ); // #0x0A00001: struct config_var* get_config_vars_start ();
	IMPORT_FUNC_R( get_config_vars_end  ); // #0x0A00002: struct config_var* get_config_vars_end ();
	// Importing from menuindexentries.h
	// Importing from firmware.h
	// Importing from all_headers.h
	// Importing from ptp-chdk.h
	// Importing from camera.h
	// Importing from dryos.h
	IMPORT_FUNC_R( msleep ); // #0x0000001: void msleep(int amount );
	IMPORT_FUNC_R( call ); // #0x0000002: void call(const char* name, ... );
	IMPORT_FUNC_R( FIO_Open ); // #0x0100001: FILE* FIO_Open(const char* filename, unsigned mode );
	IMPORT_FUNC_R( FIO_ReadFile ); // #0x0100002: int FIO_ReadFile(FILE* stream, void* ptr, size_t count );
	IMPORT_FUNC_R( FIO_WriteFile ); // #0x0100003: int FIO_WriteFile(FILE* stream, const void* ptr, size_t count );
	IMPORT_FUNC_R( FIO_CloseFile ); // #0x0100004: void FIO_CloseFile(FILE* stream );
	IMPORT_FUNC_R( FIO_CreateFile ); // #0x0100005: FILE* FIO_CreateFile(const char* name );
	IMPORT_FUNC_R( FIO_GetFileSize ); // #0x0100006: int FIO_GetFileSize(const char * filename, unsigned * size);
	IMPORT_FUNC_R( FIO_FindFirstEx ); // #0x0100007: struct fio_dirent * FIO_FindFirstEx(const char * dirname, struct fio_file * file);
	IMPORT_FUNC_R( FIO_FindNextEx ); // #0x0100008: int FIO_FindNextEx(struct fio_dirent * dirent, struct fio_file * file);
	IMPORT_FUNC_R( FIO_CleanupAfterFindNext_maybe ); // #0x0100009: void FIO_CleanupAfterFindNext_maybe(struct fio_dirent * dirent);
	IMPORT_FUNC_R( strlen ); // #0x0200001: size_t strlen(const char* str );
	IMPORT_FUNC_R( vsnprintf ); // #0x0200002: int vsnprintf(char* str, size_t n, const char* fmt, va_list ap );
	IMPORT_FUNC_R( snprintf ); // #0x0200003: int snprintf(char* str, size_t n, const char* fmt, ... );
	IMPORT_FUNC_R( strcmp ); // #0x0200004: int strcmp(const char* s1, const char* s2 );
	IMPORT_FUNC_R( strtol ); // #0x0200005: long strtol(const char * str, char ** endptr, int base );
	IMPORT_FUNC_R( strcpy ); // #0x0200006: char* strcpy(char* dst, const char * src );
	IMPORT_FUNC_R( memcpy ); // #0x0200008: void* memcpy(void *, const void *, size_t );
	IMPORT_FUNC_R( atoi ); // #0x020000A: int atoi(const char * );
	IMPORT_FUNC_R( streq ); // #0x020000B: int streq(const char *, const char * );
	IMPORT_FUNC_R( AllocateMemory ); // #0x020000C: void* AllocateMemory(size_t size );
	IMPORT_FUNC_R( FreeMemory ); // #0x020000D: void FreeMemory(void* ptr );
	IMPORT_FUNC_R( my_memcpy ); // #0x020000E: void my_memcpy(void* dst, const void* src, size_t size );
	IMPORT_FUNC_R( alloc_dma_memory ); // #0x020000F: void * alloc_dma_memory(size_t len);
	IMPORT_FUNC_R( free_dma_memory ); // #0x0200010: void free_dma_memory(const void * ptr);
	IMPORT_FUNC_R( strstr ); // #0x0200011: char* strstr(const char* str1, const char* str2);
	IMPORT_FUNC_R( strpbrk ); // #0x0200012: char* strpbrk(const char* str1, const char* str2);
	IMPORT_FUNC_R( strchr ); // #0x0200013: char* strchr(const char* str, int c);
	IMPORT_FUNC_R( sprintf ); // #0x0200014: int sprintf(char * str, const char * fmt, ...);
	IMPORT_FUNC_R( memcmp ); // #0x0200015: int memcmp(const void* s1, const void* s2,size_t n);
	IMPORT_FUNC_R( memchr ); // #0x0200016: void * memchr(const void *s, int c, size_t n);
	IMPORT_FUNC_R( strspn ); // #0x0200017: size_t strspn(const char *s1, const char *s2);
	IMPORT_FUNC_R( tolower ); // #0x0200018: int tolower(int c);
	IMPORT_FUNC_R( toupper ); // #0x0200019: int toupper(int c);
	IMPORT_FUNC_R( islower ); // #0x020001A: int islower(int x);
	IMPORT_FUNC_R( isupper ); // #0x020001B: int isupper(int x);
	IMPORT_FUNC_R( isalpha ); // #0x020001C: int isalpha(int x);
	IMPORT_FUNC_R( isdigit ); // #0x020001D: int isdigit(int x);
	IMPORT_FUNC_R( isxdigit ); // #0x020001E: int isxdigit(int x);
	IMPORT_FUNC_R( isalnum ); // #0x020001F: int isalnum(int x);
	IMPORT_FUNC_R( ispunct ); // #0x0200020: int ispunct(int x);
	IMPORT_FUNC_R( isgraph ); // #0x0200021: int isgraph(int x);
	IMPORT_FUNC_R( isspace ); // #0x0200022: int isspace(int x);
	IMPORT_FUNC_R( iscntrl ); // #0x0200023: int iscntrl(int x);
	IMPORT_FUNC_R( abs ); // #0x0300001: int abs(int number );
	IMPORT_FUNC_R( get_card_drive ); // #0x0400001: const char* get_card_drive(void );
	// Importing from lens.h
	IMPORT_FUNC_R( lens_take_picture ); // #0x0900001: int lens_take_picture(int wait, int allow_af );
	// Importing from compiler.h
	// Importing from bmp.h
	IMPORT_FUNC_R( bmp_printf ); // #0x0500001: void bmp_printf(unsigned fontspec, unsigned x, unsigned y, const char* fmt, ... );
	IMPORT_FUNC_R( read_file ); // #0x0500002: size_t read_file(const char * filename, void * buf, size_t size);
	// Importing from reloc.h
	// Importing from af_patterns.h
	// Importing from ptp-ml.h
	// Importing from state-object.h
	// Importing from tasks.h
	// Importing from font.h
	// Importing from gui.h
	// Importing from menuhelp.h
	// Importing from mvr.h
	// Importing from dialog.h
	// Importing from audio.h
	// Importing from menu.h
	IMPORT_FUNC_R( menu_add ); // #0x0700001: void menu_add(const char * name, struct menu_entry * new_entry, int count );
	IMPORT_FUNC_R( menu_draw_icon ); // #0x0700002: void menu_draw_icon(int x, int y, int type, intptr_t arg);
	// Importing from disable-this-module.h
	// Importing from vram.h
	// Importing from console.h
	IMPORT_FUNC_R( console_puts ); // #0x0800001: void console_puts(const char* str );
	IMPORT_FUNC_R( console_vprintf ); // #0x0800002: int console_vprintf(const char* fmt, va_list ap );
	IMPORT_FUNC_R( console_printf ); // #0x0800003: int console_printf(const char* fmt, ... );
	// Importing from ptp.h
	// Importing from arm-mcr.h
	// Importing from version.h
	// Importing from property.h
	IMPORT_FUNC_R( prop_request_change ); // #0x0600001: void prop_request_change(unsigned property, const void* addr, size_t len );
	IMPORT_FUNC_R( prop_get_value ); // #0x0600002: int prop_get_value(unsigned property, void** addr, size_t* len );
	// Importing from debug.h
	// Importing from cordic-16bit.h
	// Importing from consts.h
	// Importing from propvalues.h
	// Importing from plugin.h
	return res;
}
