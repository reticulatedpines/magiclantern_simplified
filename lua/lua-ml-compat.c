#include "lua-ml-compat.h"


int do_fwrite(void* ptr, size_t size, size_t count, FILE* stream) {
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
		res = do_fwrite((void*)fmt, strlen(fmt)+1, 1, stream);
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
	if (strchr(mode,"r")) {
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

char *strstr(const char *haystack, const char *needle)
{
	size_t needlelen;
	/* Check for the null needle case.  */
	if (*needle == '\0')
		return (char *) haystack;
	needlelen = strlen(needle);
	for (; (haystack = strchr(haystack, *needle)) != NULL; haystack++)
		if (memcmp(haystack, needle, needlelen) == 0)
			return (char *) haystack;
	return NULL;
}

char* strchr(const char* s, int c) {
	while (*s != '\0' && *s != (char)c)
		s++;
	return ( (*s == c) ? (char *) s : NULL );
}

char* strpbrk(const char* s1, const char* s2)
{
	const char *sc1;
	for (sc1 = s1; *sc1 != '\0'; sc1++)
		if (strchr(s2, *sc1) != NULL)
			return (char *)sc1;
	return NULL;
}

void do_abort() {
}

char* do_getenv(char* name) {
	return NULL;
}

#define MAX_VSNPRINTF_SIZE 4096

int sprintf(char* str, const char* fmt, ...)
{
	int num;// = strlen(fmt)+1;
	va_list            ap;

//	memcpy(str,fmt,num);
	va_start( ap, fmt );
	num = vsnprintf( str, MAX_VSNPRINTF_SIZE, fmt, ap );
	va_end( ap );
	return num;
}

int memcmp(const void* s1, const void* s2,size_t n)
{
	const unsigned char *us1 = (const unsigned char *) s1;
	const unsigned char *us2 = (const unsigned char *) s2;
	while (n-- != 0) {
		if (*us1 != *us2)
			return (*us1 < *us2) ? -1 : +1;
		us1++;
		us2++;
	}
	return 0;
}

int toupper(int c)
{
	if(('a' <= c) && (c <= 'z'))
		return 'A' + c - 'a';
	return c;
}

int tolower(int c)
{
	if(('A' <= c) && (c <= 'Z'))
		return 'a' + c - 'A';
	return c;
}

void *memchr(const void *s, int c, size_t n)
{
	const unsigned char *src = s;
	unsigned char uc = c;
	while (n-- != 0) {
		if (*src == uc)
			return (void *) src;
		src++;
	}
	return NULL;
}

size_t strspn(const char *s1, const char *s2)
{
	const char *sc1;
	for (sc1 = s1; *sc1 != '\0'; sc1++)
		if (strchr(s2, *sc1) == NULL)
			return (sc1 - s1);
	return sc1 - s1;
}

int islower(int x) { return ((x)>='a') && ((x)<='z'); }
int isupper(int x) { return ((x)>='A') && ((x)<='Z'); }
int isalpha(int x) { return islower(x) || isupper(x); }
int isdigit(int x) { return ((x)>='0') && ((x)<='9'); }
int isxdigit(int x) { return isdigit(x) || (((x)>='A') && ((x)<='F')) || (((x)>='a') && ((x)<='f')); }
int isalnum(int x) { return isalpha(x) || isdigit(x); }
int ispunct(int x) { return strchr("!\"#%&'();<=>?[\\]*+,-./:^_{|}~",x)!=0; }
int isgraph(int x) { return ispunct(x) || isalnum(x); }
int isspace(int x) { return strchr(" \r\n\t",x)!=0; }
int iscntrl(int x) { return strchr("\x07\x08\r\n\x0C\x0B\x09",x)!=0; }
