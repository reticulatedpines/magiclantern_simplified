#include "lua-ml-compat.h"
#include "lua.h"
#include "lauxlib.h"

#define O_RDONLY             00
#define O_SYNC           010000

int __errno;

int do_fwrite(const void* ptr, size_t size, size_t count, FILE* stream) {
	if (stream==stderr || stream==stdout) {
		console_puts(ptr);
	} else if (stream) {
		return FIO_WriteFile(stream, ptr, size * count);
	}
	return 0;
}

int do_fflush(FILE* stream) {
	return 0;
}

int do_fprintf(FILE* stream, const char * fmt, ...) {
	int res = 0;
	if (stream==stderr || stream==stdout) {
		va_list ap;
		va_start( ap, fmt );
		res = console_vprintf(fmt, ap);
		va_end( ap );
	} else if (stream) {
		// should do format
		res = do_fwrite((const void*)fmt, strlen(fmt)+1, 1, stream);
	}
	return res;
};

int do_getc ( FILE * stream ) {
	if (stream && stream!=stdout && stream!=stderr) {
		char buf;
		int res = FIO_ReadFile(stream, &buf, 1);
		if (res) return res;
	}
	return EOF;
}

int do_feof( FILE * stream ) {
	return 0;
}

int do_fread(void* ptr, size_t size, size_t count, FILE* stream) {
	if (stream && stream!=stdout && stream!=stderr) {
		return FIO_ReadFile(stream, ptr, size*count);
	}
	return 0;
}

FILE* do_fopen(const char* filename, const char* mode) {
	if (strchr(mode,'r')) {
		return FIO_Open(filename,O_RDONLY | O_SYNC);
	} else {
		return FIO_Open(filename,0);
	}
}

FILE* do_freopen(const char * filename, const char* mode, FILE * stream) {
	return NULL;
}

int do_ferror(FILE * stream) {
	if (stream) return 0;
	return 1;
}

int do_fclose(FILE * stream) {
	FIO_CloseFile(stream);
	return 0;
}

void do_abort() {
}

char* do_getenv(const char* name) {
	return NULL;
}

char* do_fgets(char* str, int num, FILE * stream) {
	int i;
	int c;
	int done = 0;
	if (str == 0 || num <= 0 || stream == 0)
		return 0;
	if (stream == stderr || stream == stdout)
		return 0;
	for (i = 0; !done && i < num - 1; i++) {
		c = do_getc(stream);
		if (c == EOF) {
			done = 1;
			i--;
		} else {
			str[i] = c;
			if (c == '\n')
				done = 1;
		}
	}
	str[i] = '\0';
	if (i == 0)
		return 0;
	else
		return str;
}
