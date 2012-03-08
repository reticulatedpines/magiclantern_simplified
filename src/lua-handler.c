/** \file
 * LUA handler and function wrappers
 */
/*
 * Copyright (C) 2012 Zsolt Sz. Sztupak <mail@sztupy.hu>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */
#include "lua-handler.h"
#include "plugin.h"
#include "config.h"
#include "lua.h"
#include "lauxlib.h"

static struct ext_plugin * luaplug = 0;

const char *(*PLluaL_checklstring) (lua_State *L, int numArg, size_t *l);
int (*PLluaL_checknumber) (lua_State *L, int numArg);
int (*PLlua_toboolean) (lua_State *L, int idx);
int (*PLlua_type) (lua_State *L, int idx);
void (*PLlua_pushnil) (lua_State *L);
void (*PLlua_pushnumber) (lua_State *L, lua_Number n);
const char *(*PLlua_pushlstring) (lua_State *L, const char *s, size_t l);
const char *(*PLlua_pushstring) (lua_State *L, const char *s);
void  (*PLlua_pushboolean) (lua_State *L, int b);
void (*PLlua_createtable) (lua_State *L, int narr, int nrec);
void (*PLlua_setfield) (lua_State *L, int idx, const char *k);
int  (*PLlua_error) (lua_State *L);
lua_State *(*PLlua_newstate) (lua_Alloc f, void *ud);
void (*PLluaL_openlibs) (lua_State *L);
int (*PLluaopen_base) (lua_State *L);
void (*PLluaL_setfuncs) (lua_State *L, const luaL_Reg *l, int nup);
int (*PLlua_sethook) (lua_State *L, lua_Hook func, int mask, int count);
int (*PLluaL_loadstring) (lua_State *L, const char *s);
const char *(*PLlua_tolstring) (lua_State *L, int idx, size_t *len);
int  (*PLlua_pcallk) (lua_State *L, int nargs, int nresults, int errfunc, int ctx, lua_CFunction k);
void (*PLlua_close) (lua_State *L);


void PLlua_load_plug() {
	luaplug = load_plugin(CARD_DRIVE "plugins/lua.bin");
	if (luaplug) {
		PLluaL_checklstring = get_function(luaplug, 1);
		PLluaL_checknumber = get_function(luaplug, 2);
		PLlua_toboolean = get_function(luaplug, 3);
		PLlua_type = get_function(luaplug, 4);
		PLlua_pushnil = get_function(luaplug, 5);
		PLlua_pushnumber = get_function(luaplug, 6);
		PLlua_pushlstring = get_function(luaplug, 7);
		PLlua_pushstring = get_function(luaplug, 8);
		PLlua_pushboolean = get_function(luaplug, 9);
		PLlua_createtable = get_function(luaplug, 10);
		PLlua_setfield = get_function(luaplug, 11);
		PLlua_error = get_function(luaplug, 12);
		PLlua_newstate = get_function(luaplug, 13);
		PLluaL_openlibs = get_function(luaplug, 14);
		PLluaopen_base = get_function(luaplug, 15);
		PLluaL_setfuncs = get_function(luaplug, 16);
		PLlua_sethook = get_function(luaplug, 17);
		PLluaL_loadstring = get_function(luaplug, 18);
		PLlua_tolstring = get_function(luaplug, 19);
		PLlua_pcallk = get_function(luaplug, 20);
		PLlua_close = get_function(luaplug, 21);
	}
}

extern struct config_var    _config_vars_start[];
extern struct config_var    _config_vars_end[];

static lua_State* L;
static volatile int need_script_terminate = 0;
static volatile int is_script_running = 0;

#define FUNC( X ) { #X, luaML_##X }
#define FUNC_DEF( X ) static int luaML_##X( lua_State * L )

/************************************************
 * cprint(s): writes string 's' to the console
 ************************************************/
FUNC_DEF( cprint ) {
	size_t l;
	const char *s = PLluaL_checklstring(L, 1, &l);
	console_printf("%s",s);
	return 0;
}

/************************************************
 * bmpprint(x,y,s): writes string 's' to the
 * coordinates 'x','y'
 ************************************************/
FUNC_DEF( bmpprint ) {
	int x, y;
	size_t l;
	x = PLluaL_checknumber( L, 1 );
	y = PLluaL_checknumber( L, 2 );
	const char *s = PLluaL_checklstring(L, 3, &l);
	bmp_printf(FONT_LARGE, x, y, "%s", s);
	return 0;
}


/************************************************
 * openlibs(): loads the standard lua libraries
 * into memory, except io, math and os
 ************************************************/
FUNC_DEF( openlibs ) {
	PLluaL_openlibs( L );
	return 0;
}

/************************************************
 * msleep(n): sleeps 'n' milliseconds
 ************************************************/
FUNC_DEF( msleep ) {
	int s = PLluaL_checknumber( L, 1 );
	while (s>0) {
		if (need_script_terminate) {
			PLlua_pushstring(L, "Script abort requested!");
			need_script_terminate = 0;
			return PLlua_error(L);
		}
		if (s>50) {
			msleep(50);
			s-=50;
		} else {
			msleep(s);
			s=0;
		}
	}
	return 0;
}


/************************************************
 * res = getconfig(cname): gets the value of the
 * config called 'cname', and puts into 'res'.
 * Return nil if the variable is not found
 ************************************************/
FUNC_DEF( getconfig ) {
	size_t l;
	const char* s = PLluaL_checklstring(L, 1, &l);
	struct config_var * var = _config_vars_start;
	for ( ; var < _config_vars_end; var ++ ) {
		if (!streq( var->name, s)) continue;
		if (var->type == 0 ) {
			PLlua_pushnumber(L, *(unsigned*) var->value);
			return 1;
		} else {
			PLlua_pushstring(L, *(char **) var->value);
			return 1;
		}
	}
	PLlua_pushnil( L );
	return 1;
}

/************************************************
 * res = setconfig(cname, nval): sets the value
 * of the config called 'cname' to 'nval'
 * returns true if success, false othervise
 ************************************************/
FUNC_DEF( setconfig ) {
	size_t l;
	const char* s = PLluaL_checklstring(L, 1, &l);
	struct config_var * var = _config_vars_start;
	for ( ; var < _config_vars_end; var ++ ) {
		if (!streq( var->name, s)) continue;
		if (var->type == 0 ) {
			int val = PLluaL_checknumber(L, 2);
			*(int*) var->value = val;
			PLlua_pushboolean(L, true);
			return 1;
		} else {
			size_t val_len;
			const char* val = PLluaL_checklstring(L, 2, &val_len);
			char* nvalue = AllocateMemory( val_len + 1 );
			if (nvalue) {
				my_memcpy(nvalue, (void*)val, val_len + 1);
				*(char **) var->value = nvalue;
				PLlua_pushboolean(L, true);
			} else {
				PLlua_pushboolean(L, false);
			}
			return 1;
		}
	}
	PLlua_pushboolean( L, false );
	return 1;
}

/************************************************
 * tbl = configs(): gets a table of all config
 * variables in the system
 ************************************************/
FUNC_DEF( configs ) {
	size_t config_num = (_config_vars_end - _config_vars_start) / sizeof(struct config_var);
	struct config_var * var = _config_vars_start;

	PLlua_createtable(L, 0, config_num);
	for ( ; var < _config_vars_end; var ++ ) {
		if (var->name) {
			if (var->type == 0) {
				PLlua_pushnumber(L, *(int *)var->value);
			} else {
				PLlua_pushstring(L, *(char **)var->value);
			}
			PLlua_setfield(L, -2, var->name);
		}
	}
	return 1;
}

/************************************************
 * ret = getprop(id):
 * gets the value of property 'id'. If the property
 * has a size of 4 it will be an int else it will
 * be a string. nil is returned in case of an error
 ************************************************/
FUNC_DEF( getprop ) {
	int id = PLluaL_checknumber( L, 1 );
	void* data = 0;
	size_t len = 0;
	int err = prop_get_value(id, (void**)&data, &len);
	if (err) {
		PLlua_pushnil(L);
	} else {
		if (len==4) {
			PLlua_pushnumber( L, ((int*)data)[0] );
		} else {
			PLlua_pushlstring( L, (char*)data, len);
		}
	}
	return 1;
}

/************************************************
 * ret = getintprop(id):
 * gets the value of property 'id'. This function
 * will always return an int, or nil in case of
 * an error.
 ************************************************/
FUNC_DEF( getintprop ) {
	int id = PLluaL_checknumber( L, 1 );
	void* data = 0;
	size_t len = 0;
	int err = prop_get_value(id, (void**)&data, &len);
	if (err) {
		PLlua_pushnil(L);
	} else {
		PLlua_pushnumber( L, ((int*)data)[0] );
	}
	return 1;
}

/************************************************
 * ret = getstrprop(id):
 * gets the value of property 'id'. This function
 * will always return a string, or nil in case of
 * an error.
 ************************************************/
FUNC_DEF( getstrprop ) {
	int id = PLluaL_checknumber( L, 1 );
	void* data = 0;
	size_t len = 0;
	int err = prop_get_value(id, (void**)&data, &len);
	if (err) {
		PLlua_pushnil(L);
	} else {
		PLlua_pushlstring( L, (char*)data, len);
	}
	return 1;
}

/************************************************
 * setprop(id,value):
 * Sets the value of property 'id' to 'value'.
 ************************************************/
FUNC_DEF( setprop ) {
	int id = PLluaL_checknumber( L, 1 );
	int ltype = PLlua_type(L, 2);
	switch (ltype) {
		case LUA_TBOOLEAN:
		case LUA_TNUMBER:
		case LUA_TNIL:
			{
				int data;
				switch (ltype) {
					case LUA_TBOOLEAN: data = PLlua_toboolean(L, 2); break;
					case LUA_TNUMBER: data = PLluaL_checknumber(L, 2); break;
					case LUA_TNIL: data = 0; break;
				}
				prop_request_change( id, &data, 4);
				break;
			}
		case LUA_TSTRING:
			{
				size_t l;
				const char* data = PLluaL_checklstring( L, 2, &l );
				prop_request_change( id, data, l);
				break;
			}
	}
	return 0;
}

/************************************************
 * shoot(wait, allow_af):
 * Takes a picture.
 ************************************************/
FUNC_DEF( shoot ) {
	int wait = PLluaL_checknumber(L, 1);
	int allow_af = PLlua_toboolean(L, 2);
	lens_take_picture(wait, allow_af);
	return 0;
}

/************************************************
 * eoscall(name):
 * Runs EOS subroutine called 'name'.
 ************************************************/
FUNC_DEF( eoscall ) {
	size_t l;
	const char* name = PLluaL_checklstring(L, 1, &l);
	call(name);
	return 0;
}

/************************************************
 * dump(start,length,name):
 * Dumps memory from 'start' to 'start'+'length'
 * to filename 'name'
 ************************************************/
FUNC_DEF( dump ) {
	size_t l;
	int start = PLluaL_checknumber(L, 1);
	int length = PLluaL_checknumber(L, 2);
	const char* fname = PLluaL_checklstring(L,3,&l);
	char fullfname[50];
	snprintf(fullfname, sizeof(fullfname), "%s%s",CARD_DRIVE, fname);
    FILE * f = FIO_CreateFile(fullfname);
	if (f != (void*) -1) {
		FIO_WriteFile(f, (void*)start, length);
		FIO_CloseFile(f);
	}
	msleep(10);
	return 0;
}

// functions that will be always loaded,
// and resides in the global namespace
static const luaL_Reg base_functions[] = {
	FUNC( cprint ),
	FUNC( bmpprint ),
	FUNC( openlibs ),
	FUNC( msleep ),
	FUNC( getconfig ),
	FUNC( setconfig ),
	FUNC( configs ),
	FUNC( getprop ),
	FUNC( getintprop ),
	FUNC( getstrprop ),
	FUNC( setprop ),
	FUNC( shoot ),
	FUNC( eoscall ),
	FUNC( dump ),
	{ NULL, NULL },
};

// memory allocation handler

CONFIG_INT("lua.memsleep", PLlua_memsleep, 10);

#define LUA_CHECK_COUNT 100

static int panic (lua_State *L) {
	console_printf("PANIC: unprotected error in call to Lua API (%s)\n",
	PLlua_tostring(L, -1));
	return 0;  /* return to Lua to abort */
}

static void *PLlua_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud; (void)osize;  /* not used */
  if (ptr == 0 && nsize == 0) return NULL;
  if (nsize == 0) {
    FreeMemory(ptr);
    return NULL;
  }
  else
  {
    if( nsize <= osize && ptr ) {
      return ptr;
    }
    void* res = AllocateMemory( nsize );
    if( !res )
      return NULL;
    if ( ptr ) {
      msleep((unsigned int)ud);
      my_memcpy(res, ptr, osize);
      msleep((unsigned int)ud);
      FreeMemory(ptr);
    }
    return res;
  }
}

// exits if we terminate the script
static void PLlua_count_hook(lua_State *L, lua_Debug *ar)
{
	if (need_script_terminate) {
		PLlua_pushstring(L, "Script abort requested!");
		need_script_terminate = 0;
		PLlua_error(L);
	}
}

// directory handler and other files are
// taken from the approriate sections from
// the cropmarks handler

#define MAX_SCRIPT_NAME_LEN 15
#define MAX_SCRIPTS 9
int num_scripts = 0;
int script_index = 0;
static char script_names[MAX_SCRIPTS][MAX_SCRIPT_NAME_LEN];
static char* script_source_str;

// Cropmark sorting code contributed by Nathan Rosenquist
static void sort_scripts()
{
    int i = 0;
    int j = 0;

    char aux[MAX_SCRIPT_NAME_LEN];

    for (i=0; i<num_scripts; i++)
    {
        for (j=i+1; j<num_scripts; j++)
        {
            if (strcmp(script_names[i], script_names[j]) > 0)
            {
                strcpy(aux, script_names[i]);
                strcpy(script_names[i], script_names[j]);
                strcpy(script_names[j], aux);
            }
        }
    }
}

int is_valid_script_filename(char* filename)
{
    int n = strlen(filename);
    if ((n > 4) && (streq(filename + n - 4, ".LUA") || streq(filename + n - 4, ".lua")) && (filename[0] != '.') && (filename[0] != '_'))
        return 1;
    return 0;
}

static void find_scripts()
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( CARD_DRIVE "SCRIPTS/", &file );
    if( IS_ERROR(dirent) )
    {
        NotifyBox(2000, "SCRIPTS dir missing" );
        msleep(100);
        NotifyBox(2000, "Please copy all ML files!" );
        return;
    }
    int k = 0;
    do {
        if (is_valid_script_filename(file.name))
        {
            if (k >= MAX_SCRIPTS)
            {
                NotifyBox(2000, "TOO MANY SCRIPTS (max=%d)", MAX_SCRIPTS);
                break;
            }
            snprintf(script_names[k], MAX_SCRIPT_NAME_LEN, "%s", file.name);
            k++;
        }
    } while( FIO_FindNextEx( dirent, &file ) == 0);
    FIO_CleanupAfterFindNext_maybe(dirent);
    num_scripts = k;
    sort_scripts();
}

static void* script_load(const char* filename) {
    unsigned size;
	unsigned char* buf;
	unsigned char* retval;
    if( FIO_GetFileSize( filename, &size ) != 0 )
        goto getfilesize_fail;

	buf = alloc_dma_memory(size);
	if (!buf)
		goto malloc_fail;

	if ((unsigned)read_file(filename, buf, size)!=size)
		goto read_fail;

	retval = AllocateMemory(size+1);
	if (!retval)
		goto copy_fail;

	memcpy(retval, buf, size);
	retval[size] = '\0';

	free_dma_memory(buf);

	return retval;
copy_fail:
read_fail:
	free_dma_memory(buf);
malloc_fail:
getfilesize_fail:
    DebugMsg( DM_MAGIC, 3, "script load failed");
    return NULL;
}

static void reload_script(int i)
{
	static int old_i = -1;
	char scriptname[100];
	while (is_script_running) {
		need_script_terminate = 1;
		msleep(100);
	}
	if (i == old_i) return;
	old_i = i;
	if (script_source_str) {
		FreeMemory(script_source_str);
	}
	i = COERCE(i, 0, num_scripts-1);
	if (i==-1 || num_scripts== -1) return;
	snprintf(scriptname, sizeof(scriptname), CARD_DRIVE "SCRIPTS/%s", script_names[i]);
	script_source_str = script_load(scriptname);
	if (!script_source_str) bmp_printf(FONT_LARGE, 0, 50, "LOAD ERROR %d:%s   ", i, scriptname);
}

static void
PLlua_script_run( void * priv, int delta )
{
	while (is_script_running) {
		need_script_terminate = 1;
		msleep(100);
	}
	if (need_script_terminate) need_script_terminate = 0;
	reload_script(script_index);
	is_script_running = 1;
	return;
}

static void
PLlua_script_display(void* priv, int x, int y, int selected) {
	int index = script_index;
	bmp_printf(selected ? MENU_FONT_SEL : MENU_FONT,
		x, y, "Run lua script     : %s",
		num_scripts>0 ? script_names[index] : "NO SCRIPTS");

    menu_draw_icon(x, y, MNI_BOOL_GDR(is_script_running));
}

static void
PLlua_script_display_submenu( void * priv, int x, int y, int selected )
{
    //~ extern int retry_count;
    int index = script_index;
    index = COERCE(index, 0, num_scripts);
    bmp_printf(
        selected ? MENU_FONT_SEL : MENU_FONT,
        x, y,
        "Script (%d/%d) : %s",
         index+1, num_scripts,
		 num_scripts > 0 ? script_names[index] : "NO SCRIPTS"
    );
}

static void
PLlua_script_toggle( void* priv, int sign )
{
    script_index = mod(script_index + sign, num_scripts);
	reload_script(script_index);
}


static struct menu_entry PLlua_menus[] = {
	{
		.name       = "Run LUA script",
		.priv       = NULL,
		.display    = PLlua_script_display,
		.select     = PLlua_script_run,
		.children	= (struct menu_entry[]) {
			{
				.name = "Script",
				.priv = &script_index,
				.select = PLlua_script_toggle,
				.display = PLlua_script_display_submenu,
				.icon_type = IT_ALWAYS_ON,
				.help = "You can write your own scripts in Notepad",
			},
			{
				.name = "Script wait",
				.priv = &PLlua_memsleep,
				.min  = 1,
				.max  = 50,
				.help = "Set the wait time in the script. Larger values mean stability, lower means performance",
			},
			{
				.name = "Run LUA Script",
				.priv = (unsigned*)&is_script_running,
				.min  = 0,
				.max  = 1,
				.select = PLlua_script_run,
				.icon_type = IT_ACTION,
				.help = "Starts the script",
			},
			{
				.name = "Abort LUA Script",
				.priv = (unsigned*)&need_script_terminate,
				.min  = 0,
				.max  = 1,
				.icon_type = IT_ACTION,
				.help = "Abort the running script",
			},
			MENU_EOL,
		},
		.help       = "Starts the LUA script in the background",
	},
};


static void
PLlua_task( void* unused )
{
	msleep(1000);
	find_scripts();
	for (;;) {
		msleep(100);
		if (is_script_running) {
			if (!luaplug) {
				PLlua_load_plug();
				if (!luaplug) is_script_running = 0;
			}
			if (luaplug && script_source_str) {
				int ret;
				// PLlua_alloc uses msleep while reallocing.
				// Setting PLlua_memsleep to different values
				// can switch the handler between performance and stability
				L = PLlua_newstate(PLlua_alloc, (void*)PLlua_memsleep);
				if ( L ) {
					PLluaopen_base ( L );
					PLluaL_setfuncs( L, base_functions, 0 );
					PLlua_sethook(L, PLlua_count_hook, LUA_MASKCOUNT, LUA_CHECK_COUNT );
					if ((ret = PLluaL_loadstring( L,script_source_str))!=0) {
						if (ret == LUA_ERRSYNTAX) {
							size_t lmsg;
							const char *msg = PLlua_tolstring(L, -1, &lmsg);
							if (msg) {
								console_printf("Syntax error %d (%s)\n",lmsg,msg);
							} else {
								console_printf("Syntax error\n");
							}
						} else {
							console_printf("Load error: %d\n", ret);
						}
					} else if ((ret = PLlua_pcallk( L, 0, LUA_MULTRET, 0, 0, 0))!=0) {
						if (ret == LUA_ERRRUN) {
							size_t lmsg;
							const char *msg = PLlua_tolstring(L, -1, &lmsg);
							if (msg) {
								console_printf("Runtime error %d (%s)\n",lmsg,msg);
							} else {
								console_printf("Runtime error\n");
							}
						} else {
							console_printf("Call error: %d\n", ret);
						}
					}
					PLlua_close( L );
				}
			}
		}
		is_script_running = 0;
		need_script_terminate = 0;
	}
}

static void
PLlua_init( void *unused )
{
	menu_add("Tweaks", PLlua_menus, COUNT(PLlua_menus) );
	return;
}

INIT_FUNC( __FILE__, PLlua_init );
TASK_CREATE( "PLlua_task", PLlua_task, 0, 0x1f, 0x1000 );
