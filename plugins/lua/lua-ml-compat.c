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

char* do_getenv(const char* name) {
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


// exported functions for plugin

EXTERN_FUNC( 1, const char *, PLluaL_checklstring, lua_State *L, int numArg, size_t *l ) {
	return luaL_checklstring(L,numArg,l);
}
EXTERN_FUNC( 2, int , PLluaL_checknumber, lua_State *L, int numArg ) {
	return luaL_checknumber(L, numArg);
}
EXTERN_FUNC( 3, int , PLlua_toboolean, lua_State *L, int idx ) {
	return lua_toboolean(L, idx);
}
EXTERN_FUNC( 4, int , PLlua_type, lua_State *L, int idx ) {
	return lua_type(L, idx);
}
EXTERN_FUNC( 5, void , PLlua_pushnil, lua_State *L ) {
	lua_pushnil(L);
}
EXTERN_FUNC( 6, void , PLlua_pushnumber, lua_State *L, lua_Number n ) {
	lua_pushnumber(L, n);
}
EXTERN_FUNC( 7, const char *, PLlua_pushlstring, lua_State *L, const char *s, size_t l ) {
	return lua_pushlstring(L, s, l);
}
EXTERN_FUNC( 8, const char *, PLlua_pushstring, lua_State *L, const char *s ) {
	return lua_pushstring(L, s);
}
EXTERN_FUNC( 9, void  , PLlua_pushboolean, lua_State *L, int b ) {
	lua_pushboolean(L, b);
}
EXTERN_FUNC( 10, void , PLlua_createtable, lua_State *L, int narr, int nrec ) {
	lua_createtable(L, narr, nrec);
}
EXTERN_FUNC( 11, void , PLlua_setfield, lua_State *L, int idx, const char *k ) {
	lua_setfield(L, idx, k);
}
EXTERN_FUNC( 12, int  , PLlua_error, lua_State *L ) {
	return lua_error(L);
}
EXTERN_FUNC( 13, lua_State *, PLlua_newstate, lua_Alloc f, void *ud ) {
	return lua_newstate(f, ud);
}
EXTERN_FUNC( 14, void , PLluaL_openlibs, lua_State *L ) {
	luaL_openlibs(L);
}
EXTERN_FUNC( 15, int , PLluaopen_base, lua_State *L ) {
	return luaopen_base(L);
}
EXTERN_FUNC( 16, void , PLluaL_setfuncs, lua_State *L, const luaL_Reg *l, int nup ) {
	luaL_setfuncs(L, l, nup);
}
EXTERN_FUNC( 17, int , PLlua_sethook, lua_State *L, lua_Hook func, int mask, int count ) {
	return lua_sethook(L, func, mask, count);
}
EXTERN_FUNC( 18, int , PLluaL_loadstring, lua_State *L, const char *s ) {
	return luaL_loadstring(L, s);
}
EXTERN_FUNC( 19, const char *, PLlua_tolstring, lua_State *L, int idx, size_t *len ) {
	return lua_tolstring(L, idx, len);
}
EXTERN_FUNC( 20, int  , PLlua_pcallk, lua_State *L, int nargs, int nresults, int errfunc, int ctx, lua_CFunction k ) {
	return lua_pcallk(L, nargs, nresults, errfunc, ctx, k);
}
EXTERN_FUNC( 21, void , PLlua_close, lua_State *L ) {
	lua_close(L);
}
