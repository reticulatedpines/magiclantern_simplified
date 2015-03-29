#include "ml-lua-shim.h"

int my_getc(FILE * stream)
{
    int c = 0;
    if(FIO_ReadFile(stream, &c, 1) == 1)
    {
        return c;
    }
    else
    {
        return EOF;
    }
}

FILE * my_fopen(const char * filename, const char * mode)
{
    if(mode == NULL) return NULL;
    if(mode[0] == 'r' && mode[1] == '+') return FIO_OpenFile(filename, O_RDWR | O_SYNC);
    else if(mode[0] == 'r') return FIO_OpenFile(filename, O_RDONLY | O_SYNC);
    else if(mode[0] == 'w') return FIO_CreateFile(filename);
    else if(mode[0] == 'a') return FIO_CreateFileOrAppend(filename);
    else return NULL;
}

FILE * my_freopen(const char * filename, const char * mode, FILE * f)
{
    FIO_CloseFile(f);
    return my_fopen(filename, mode);
}

int my_fclose(FILE * stream)
{
    FIO_CloseFile(stream);
    return 0;
}

int my_ferror(FILE * stream)
{
    return 0; //thou shalt not have file errors :P
}

int my_feof(FILE * stream)
{
    int c = 0;
    if(FIO_ReadFile(stream, &c, 1) == 1)
    {
        FIO_SeekSkipFile(stream, -1, SEEK_CUR);
        return 0;
    }
    else
    {
        return 1;
    }
}

int do_nothing()
{
    return 0;
}

char * my_fgets(char * str, int num, FILE * stream )
{
    int i = 0;
    for(i = 0; i < num; i++)
    {
        int c = my_getc(stream);
        if(c == EOF) break;
        str[i] = c;
        if(c == '\n' || c == '\r')
        {
            i++;
            break;
        }
    }
    str[i] = 0x0;
    return str;
}

FILE * my_tmpfile()
{
    //TODO: implement this?
    return NULL;
}

int my_ungetc(int character, FILE * stream)
{
    //this is not really correct
    FIO_SeekSkipFile(stream, -1, SEEK_CUR);
    return character;
}

char* my_getenv(const char* name)
{
    //ML doesn't have environment variables
    return NULL;
}

void my_abort()
{
    //what should we do here?
}