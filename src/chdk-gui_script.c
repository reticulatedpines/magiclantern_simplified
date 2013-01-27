// Ported from CHDK
// (C) The CHDK team


#include "dryos.h"
#include "bmp.h"
#include "string.h"

#define SCRIPT_NUM_PARAMS           26

#define MENUITEM_BOOL           1
#define MENUITEM_INT            2
#define MENUITEM_F_UNSIGNED     0x0010
#define MENUITEM_F_MINMAX       0x0060
#define MENU_MINMAX(min, max)   (((max)<<16)|(min&0xFFFF))
#define MENU_MIN_SIGNED(arg)    ((short)(arg & 0xFFFF))
#define MENU_MAX_SIGNED(arg)    ((short)((arg>>16) & 0xFFFF))
#define MENU_MIN_UNSIGNED(arg)  ((unsigned short)(arg & 0xFFFF))
#define MENU_MAX_UNSIGNED(arg)  ((unsigned short)((arg>>16) & 0xFFFF))

const char *script_source_str = NULL;       // ptr to content of script
//static char cfg_name[100] = "\0";           // buffer to make cfg files name (paramsetnum, param_names)
//static char cfg_param_name[100] = "\0";     // buffer to make cfg param files name (params, state_before_tmprun)

// ================ SCRIPT PARAMETERS ==========
char script_title[25];                                      // Title of current script

int conf_script_vars[SCRIPT_NUM_PARAMS];

// 1. Values of script parameters are stored in conf.script_vars
// 2. Encoding scheme is: array[VAR-'a'] = value

#define MAX_PARAM_NAME_LEN  27

char script_params[SCRIPT_NUM_PARAMS][MAX_PARAM_NAME_LEN+1];// Parameter title
static short script_param_order[SCRIPT_NUM_PARAMS];         // Ordered as_in_script list of variables ( [idx] = id_of_var )
                                                            // to display in same order in script
static int script_range_values[SCRIPT_NUM_PARAMS];          // Min/Max values for param validation
static short script_range_types[SCRIPT_NUM_PARAMS];         // Specifies if range values is signed (-9999-32767) or unsigned (0-65535)
                                                            // Note: -9999 limit on negative values is due to current gui_menu code (and because menu only displays chars)
static const char **script_named_values[SCRIPT_NUM_PARAMS]; // Array of list values for named parameters
static int script_named_counts[SCRIPT_NUM_PARAMS];          // Count of # of entries in each script_list_values array
static char *script_named_strings[SCRIPT_NUM_PARAMS];       // Base storage for named value string data
static int script_loaded_params[SCRIPT_NUM_PARAMS];         // Copy of original values of parameters 
                                                            // (detect are they changed or not)

//-------------------------------------------------------------------

const char* skip_whitespace(const char* p)  { while (*p==' ' || *p=='\t') p++; return p; }                                  // Skip past whitespace
const char* skip_token(const char* p)       { while (*p && *p!='\r' && *p!='\n' && *p!=' ' && *p!='\t') p++; return p; }    // Skip past current token value
const char* skip_toeol(const char* p)       { while (*p && *p!='\r' && *p!='\n') p++; return p; }                           // Skip to end of line
const char* skip_eol(const char *p)         { p = skip_toeol(p); if (*p == '\r') p++; if (*p == '\n') p++; return p; }      // Skip past end of line

//=======================================================
//             PROCESSING "@ACTION" FUNCTIONS
//=======================================================


//-------------------------------------------------------------------
static void process_title(const char *title)
{
    register const char *ptr = title;
    register size_t i=0;

    ptr = skip_whitespace(ptr);
    while (i<(sizeof(script_title)-1) && ptr[i] && ptr[i]!='\r' && ptr[i]!='\n')
    {
        script_title[i]=ptr[i];
        ++i;
    }
    script_title[i]=0;

    script_setup_title(script_title);
}

//-------------------------------------------------------------------
// Process one entry "@param VAR TITLE" to check if it exists
//      param = ptr right after descriptor (should point to var)
// RETURN VALUE: 0 if not found, 1..26 = id of var
// Used to ensure that a param loaded from an old saved paramset does
// not overwrite defaults from script
//-------------------------------------------------------------------
static int check_param(const char *param)
{
    register const char *ptr = param;
    register unsigned int n=0, /*i=0,*/ l;

    ptr = skip_whitespace(ptr);
    if (ptr[0] && (ptr[0]>='a' && ptr[0]<='a'+SCRIPT_NUM_PARAMS) && (ptr[1]==' ' || ptr[1]=='\t'))
    {
        n = ptr[0]-'a';                                 // VAR
        ptr = skip_whitespace(ptr+2);                   // skip to TITLE
        l = skip_toeol(ptr) - ptr;                      // get length of TITLE
        if (l > MAX_PARAM_NAME_LEN)
            l = MAX_PARAM_NAME_LEN;
        if (l != strlen(script_params[n]))              // Check length matches existing TITLE length
            n = 0;
        else if (strncmp(ptr,script_params[n],l) != 0)  // Check that TITLE matches existing TITLE
            n = 0;
        else
            n++;
    }
    return n; // n=1 if '@param a' was processed, n=2 for 'b' ... n=26 for 'z'. n=0 if failed.
}

//-------------------------------------------------------------------
// Process one entry "@param VAR TITLE"
//      param = ptr right after descriptor (should point to var)
// RESULT: script_params[VAR] - parameter title
// RETURN VALUE: 0 if err, 1..26 = id of var
//-------------------------------------------------------------------
static int process_param(const char *param)
{
    register const char *ptr = param;
    register int n=0, /*i=0,*/ l;

    ptr = skip_whitespace(ptr);
    if (ptr[0] && (ptr[0]>='a' && ptr[0]<='a'+SCRIPT_NUM_PARAMS) && (ptr[1]==' ' || ptr[1]=='\t'))
    {
        n = ptr[0]-'a';
        ptr = skip_whitespace(ptr+2);
        l = skip_toeol(ptr) - ptr;                  // get length of TITLE
        if (l > MAX_PARAM_NAME_LEN)
            l = MAX_PARAM_NAME_LEN;
        strncpy(script_params[n],ptr,l);
        n++;
    }
    return n; // n=1 if '@param a' was processed, n=2 for 'b' ... n=26 for 'z'. n=0 if failed.
}

//-------------------------------------------------------------------
// Process one entry "@default VAR VALUE"
//      param = ptr right after descriptor (should point to var)
//-------------------------------------------------------------------
static void process_default(const char *param)
{
    register const char *ptr = param;
    register int n;

    ptr = skip_whitespace(ptr);
    if (ptr[0] && (ptr[0]>='a' && ptr[0]<='a'+SCRIPT_NUM_PARAMS) && (ptr[1]==' ' || ptr[1]=='\t'))
    {
        n = ptr[0]-'a';
        ptr = skip_whitespace(ptr+2);
        conf_script_vars[n] = strtol(ptr, NULL, 0);
        script_loaded_params[n] = conf_script_vars[n];
    } // ??? else produce error message
}

//-------------------------------------------------------------------
// Process one entry "@range VAR MIN MAX"
//      param = ptr right after descriptor (should point to var)
//-------------------------------------------------------------------
static void process_range(const char *param)
{
    register const char *ptr = param;
    register int n;

    ptr = skip_whitespace(ptr);
    if (ptr[0] && (ptr[0]>='a' && ptr[0]<='a'+SCRIPT_NUM_PARAMS) && (ptr[1]==' ' || ptr[1]=='\t'))
    {
        n = ptr[0]-'a';
        ptr = skip_whitespace(ptr+2);
        int min = strtol(ptr,NULL,0);
        ptr = skip_whitespace(skip_token(ptr));
        int max = strtol(ptr,NULL,0);
        script_range_values[n] = MENU_MINMAX(min,max);
        if ((min == 0) && (max == 1))
            script_range_types[n] = MENUITEM_BOOL;
        else if ((min >= 0) && (max >= 0)) 
            script_range_types[n] = MENUITEM_INT|MENUITEM_F_MINMAX|MENUITEM_F_UNSIGNED;
        else
            script_range_types[n] = MENUITEM_INT|MENUITEM_F_MINMAX;
    } // ??? else produce error message
}

#if 0 // not yet working
//-------------------------------------------------------------------
// Process one entry "@values VAR A B C D ..."
//      param = ptr right after descriptor (should point to var)
//-------------------------------------------------------------------
static void process_values(const char *param)
{
    register const char *ptr = param;
    register int n;

    ptr = skip_whitespace(ptr);
    if (ptr[0] && (ptr[0]>='a' && ptr[0]<='a'+SCRIPT_NUM_PARAMS) && (ptr[1]==' ' || ptr[1]=='\t'))
    {
        n = ptr[0]-'a';
        ptr = skip_whitespace(ptr+2);
        int len = skip_toeol(ptr) - ptr;
        script_named_strings[n] = malloc(len+1);
        strncpy(script_named_strings[n], ptr, len);
        script_named_strings[n][len] = 0;

        const char *p = script_named_strings[n];
        int cnt = 0;
        while (*p)
        {
            p = skip_whitespace(skip_token(p));
            cnt++;
        }
        script_named_counts[n] = cnt;
        script_named_values[n] = malloc(cnt * sizeof(char*));

        p = script_named_strings[n];
        cnt = 0;
        while (*p)
        {
            script_named_values[n][cnt] = p;
            p = skip_token(p);
            if (*p)
            {
                *((char*)p) = 0;
                p = skip_whitespace(p+1);
            }
            cnt++;
        }
    } // ??? else produce error message
}
#endif

//=======================================================
//                 SCRIPT LOADING FUNCTIONS
//=======================================================

//-------------------------------------------------------------------
// PURPOSE: Parse script (script_source_str) for @xxx
// PARAMETERS:  fn - full path of script
// RESULTS:  script_title
//           script_params
//           script_params_order
//           script_loaded_params, conf.script_vars
//-------------------------------------------------------------------
void script_scan(const char *fn, const char * script_source_str)
{
    register const char *ptr = script_source_str;
    register int i, j=0, n;
    char *c;

    // Build title

    c=strrchr(fn, '/');
    strncpy(script_title, (c)?c+1:fn, sizeof(script_title));
    script_title[sizeof(script_title)-1]=0;

    // Reset everything

    for (i=0; i<SCRIPT_NUM_PARAMS; ++i)
    {
        conf_script_vars[i] = 0;
        script_loaded_params[i] = 0;
        script_params[i][0]=0;
        script_param_order[i]=0;
        script_range_values[i] = 0;
        if (script_named_values[i]) free(script_named_values[i]);
        script_named_values[i] = 0;
        if (script_named_strings[i]) free(script_named_strings[i]);
        script_named_strings[i] = 0;
        script_named_counts[i] = 0;
        script_range_types[i] = 0;
    }

    // Fillup order, defaults

    while (ptr[0])
    {
        ptr = skip_whitespace(ptr);
        if (ptr[0]=='@') {
            if (strncmp("@title", ptr, 6)==0)
            {
                process_title(ptr+6);
            }
            else if (strncmp("@param", ptr, 6)==0)
            {
                n = process_param(ptr+6); // n=1 if '@param a' was processed, n=2 for 'b' ... n=26 for 'z'. n=0 if failed.
                if (n>0 && n<=SCRIPT_NUM_PARAMS)
                {
                  script_param_order[j]=n;
                  j++;
                }
            }
            else if (strncmp("@default", ptr, 8)==0)
            {
                process_default(ptr+8);
            }
            else if (strncmp("@range", ptr, 6)==0)
            {
                process_range(ptr+6);
            }
            #if 0
            else if (strncmp("@values", ptr, 7)==0)
            {
                process_values(ptr+7);
            }
            #endif
        }
        ptr = skip_eol(ptr);
    }
}


void script_update_menu()
{
    int j = 0;
    for (int i = 0; i < SCRIPT_NUM_PARAMS; i++)
    {
        int p = script_param_order[i];
        if (!p) continue;
        p--;
        int min = MENU_MIN_SIGNED(script_range_values[p]);
        int max = MENU_MAX_SIGNED(script_range_values[p]);
        if (script_range_types[p] == 0) // no min/max, use some default range
        {
            min = -1000;
            max = 1000;
        }
        
        // if our name is short, fill it with spaces
        int n = strlen(script_params[p]);
        int k;
        for (k = n; k < MAX_PARAM_NAME_LEN-2; k++)
            script_params[p][k] = ' ';
        script_params[p][k] = 0;

        script_setup_param(j++, script_params[p], &conf_script_vars[p], min, max);
    }
}

#include "picoc.h"

// call this right before running the script
void script_define_param_variables()
{
    //int j = 0;
    for (unsigned int i = 0; i < SCRIPT_NUM_PARAMS; i++)
    {
        short p = script_param_order[i];
        if (!p) continue;
        p--;
        //int* v = &conf_script_vars[p];
        short _varname = 'a' + p;
        short* varname = &_varname;
        extern struct ValueType IntType;
        VariableDefinePlatformVar(NULL, varname, &IntType, (union AnyValue *)&conf_script_vars[p], FALSE);
        console_printf("   Param %s = %d; // %s\n", varname, conf_script_vars[p], script_params[p]);
    }
}
