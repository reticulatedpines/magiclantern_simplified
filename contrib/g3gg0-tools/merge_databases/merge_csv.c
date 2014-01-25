
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "qsort.h"

char *strdup(const char *s);


void strlwr(char *str)
{
    while(*str)
    {
        *str = tolower(*str);
        str++;
    }
}

void free_csv_line(char **csv)
{
    int column = 0;
    while(csv[column])
    {
        free(csv[column]);
        csv[column] = NULL;
        column++;
    }
    free(csv);
}

void free_csv(char ***csv)
{
    int line = 0;
    while(csv[line])
    {
        free_csv_line(csv[line]);
        csv[line] = NULL;
        line++;
    }
    
    free(csv);
}

void free_files(char ****files)
{
    int file = 0;
    while(files[file])
    {
        free_csv(files[file]);
        files[file] = NULL;
        file++;
    }
    
    free(files);
}

int count_entries(void *table)
{
    int entries = 0;
    
    while(((void **)table)[entries])
    {
        entries++;
    }
    return entries;
}

void *realloc_helper(void *orig_ptr, int *allocated, int *index, size_t size)
{
    void *new_ptr = orig_ptr;
    
    /* increase array size */
    if(*index + 1 >= *allocated)
    {
        *allocated += 100;
        new_ptr = realloc(*(void **)orig_ptr, size * *allocated);
        if(!new_ptr)
        {
            return NULL;
        }
        *(void **)orig_ptr = new_ptr;
    }
    
    return new_ptr;
}

char **split_csv_line(char *line)
{
    int allocated = 3;
    int columns = 0;
    char **csv = malloc(sizeof(char *) * allocated);
    char *line_ptr = line;
    
    /* terminate array */
    csv[columns] = NULL;
    
    while(1)
    {
        /* no field found or end of string? */
        if(!line_ptr || !*line_ptr || *line_ptr == '\n' || *line_ptr == '\r')
        {
            break;
        }
        
        char *next_ptr = strchr(line_ptr, ';');
        if(next_ptr)
        {
            *next_ptr = '\000';
            next_ptr++;
        }
        
        /* increase array size */
        if(!realloc_helper(&csv, &allocated, &columns, sizeof(char *)))
        {
            free_csv_line(csv);
            return NULL;
        }
     
        csv[columns] = strdup(line_ptr);
        columns++;
        csv[columns] = NULL;
        
        line_ptr = next_ptr;
    }
    
    if(columns != 3)
    {
        printf("WARNING: Input file doesn't have 3 fields in CSV format. Expect problems.\n");
    }
    
    return csv;
}

char ***read_csv(char *file)
{
    FILE *f = fopen(file, "rb");
    
    if(!f)
    {
        printf("Failed to open '%s'\n", file);
        return NULL;
    }
    
    int allocated = 10;
    int lines = 0;
    char ***csv = malloc(sizeof(char **) * allocated);
    
    /* terminate array */
    csv[lines] = NULL;
    
    while(!feof(f))
    {
        /* increase array size */
        if(!realloc_helper(&csv, &allocated, &lines, sizeof(char **)))
        {
            fclose(f);
            free_csv(csv);
            return NULL; 
        }
     
        /* fill new line */
        char line[1024];
        if(fgets(line, sizeof(line), f) > 0)
        {
            csv[lines] = split_csv_line(line);
            strlwr(csv[lines][0]);
            lines++;
            csv[lines] = NULL;
        }
    }
    
    fclose(f);
    
    printf("    Read %d lines from %s\n", lines, file);
    
    return csv;
}

char ***read_stubs(char *file)
{
    FILE *f = fopen(file, "rb");
    
    if(!f)
    {
        printf("Failed to open '%s'\n", file);
        return NULL;
    }
    
    int allocated = 10;
    int ignored = 0;
    int lines = 0;
    char ***csv = malloc(sizeof(char **) * allocated);
    
    /* terminate array */
    csv[lines] = NULL;
    
    while(!feof(f))
    {
        /* update array size if needed */
        if(!realloc_helper(&csv, &allocated, &lines, sizeof(char **)))
        {
            printf("Failed to reallocate list\n");
            fclose(f);
            free_csv(csv);
        }
     
        char line[1024];
           
        /* fill new line */
        if(fgets(line, sizeof(line), f) > 0)
        {
            if(!strstr(line, "NSTUB"))
            {
                ignored++;
                continue;
            }
            
            char *start = line;
            /* trim leading spaces */
            while(*start && *start == ' ')
            {
                start++;
            }
            
            if(strncasecmp(start, "NSTUB", 5))
            {
                ignored++;
                continue;
            }
            
            start = strstr(start, "(");
            if(!start)
            {
                ignored++;
                continue;
            }
            start++;
            
            /* trim leading spaces */
            while(*start && *start == ' ')
            {
                start++;
            }
            
            char *addr = start;
            
            /* now find name and cut there */
            start = strstr(start, ",");
            if(!start)
            {
                ignored++;
                continue;
            }
            *start = '\000';
            start++;

            /* trim trailing spaces of address */
            while(strrchr(addr, ' '))
            {
                *strrchr(addr, ' ') = '\000';
            }
            
            /* trim leading spaces */
            while(*start && *start == ' ')
            {
                start++;
            }
            char *name = start;
            
            /* find string end */
            start = strstr(start, ")");
            if(!start)
            {
                ignored++;
                continue;
            }
            *start = '\000';
            
            while(strrchr(name, ' '))
            {
                *strrchr(name, ' ') = '\000';
            }
            
            /* finally fill it into our structure */
            csv[lines] = malloc(sizeof(char *) * 4);
            csv[lines][0] = strdup(addr);
            csv[lines][1] = strdup(name);
            csv[lines][2] = strdup("");
            csv[lines][3] = NULL;
            lines++;
            csv[lines] = NULL;
        }
    }
    
    fclose(f);
    
    printf("    Read %d lines from %s (%d ignored)\n", lines, file, ignored);
    
    return csv;
}

void sort_by_name(char **table)
{
    #define islt(a,b) (strcmp((*a),(*b))<0)
    QSORT(char*, table, count_entries(table), islt);
    #undef islt
}

/* go through the list and filter all elements that are the same as the one before */
void filter_uniq(char **table)
{
    char **dst_ptr = table;
    
    if(!*table)
    {
        return;
    }
    
    table++;
    while(*table)
    {
        if(strcmp(*table,*dst_ptr))
        {
            dst_ptr++;
            *dst_ptr = *table;
        }
        else
        {
            free(*table);
        }
        
        if(table != dst_ptr)
        {
            *table = NULL;
        }
        table++;
    }
}

char **build_address_list(char ****file_list)
{
    int allocated = 10;
    int addresses = 0;
    char **address_list = malloc(sizeof(char *) * allocated);
    
    address_list[addresses] = NULL;
    
    int file = 0;
    while(file_list[file])
    {
        int line = 0;
        while(file_list[file][line])
        {
            char *addr = file_list[file][line][0];
            
            if(!addr)
            {
                break;
            }
            if(!realloc_helper(&address_list, &allocated, &addresses, sizeof(char *)))
            {
                printf("Failed to reallocate address list\n");
                return NULL;
            }
            
            address_list[addresses] = strdup(addr);
            strlwr(address_list[addresses]);
            addresses++;
            address_list[addresses] = NULL;
            
            line++;
        }
        file++;
    }
    
    sort_by_name(address_list);
    filter_uniq(address_list);
    printf("    Unique addresses: %d\n", count_entries(address_list));
    
    return address_list;
}

char **find_address(char *addr, char ***file)
{
    int line = 0;
    while(file[line])
    {
        int cmp = strcmp(addr, file[line][0]);
        if(cmp == 0)
        {
            return file[line];
        }
        
        line++;
    }
    
    return NULL;
}

void mark_if_differ(char **line, int files)
{
    int differs = 0;
    char *last_name = NULL;
    
    for(int file = 0; file < files; file++)
    {
        char *name = line[2 + 3*file + 1];
        if(!strncmp(name, "sub_", 4))
        {
            /* ignore these too */
        }
        else if(strlen(name) > 2)
        {
            /* differs to last manually set name? */
            if(last_name && strcmp(last_name, name))
            {
                differs = 1;
            }
            last_name = name;
        }
    }
    
    if(differs)
    {
        line[1][0] = 'X';
    }
}

void find_preferred_name(char **line, int files)
{
    int have_name = -1;
    int have_default_name = -1;
 
    /* dont do anything if merge is required */
    if(line[1][0] == 'X')
    {
        return;
    }
    
    for(int file = 0; file < files; file++)
    {
        char *name = line[2 + 3*file + 1];
        if(!strncmp(name, "sub_", 4))
        {
            have_default_name = file;
        }
        else if(strlen(name) > 2)
        {
            have_name = file;
        }
    }
    
    if(have_name >= 0)
    {
        line[2 + 3*have_name + 0][0] = 'X';
    }
    else if(have_default_name >= 0)
    {
        line[2 + 3*have_default_name + 0][0] = 'X';
    }
}

char ***merge_lists(char **address_list, char ****file_list, char **filenames)
{
    int allocated = 10;
    int lines = 0;
    char ***csv = malloc(sizeof(char **) * allocated);
    
    int files = count_entries(file_list);
    
    int address = 0;
    while(address_list[address])
    {
        char *addr = address_list[address];
        
        if(!realloc_helper(&csv, &allocated, &lines, sizeof(char **)))
        {
            printf("Failed to reallocate address list\n");
            return NULL;
        }
        
        /* per line: address, difference, file_use, file_name, file_proto, NULL */
        csv[lines] = malloc(sizeof(char *) * (2 + files * 3 + 1));
        csv[lines][0] = strdup(addr);
        csv[lines][1] = strdup(" ");
        csv[lines][2 + files * 3] = NULL;
        
        //printf("Addr: %s\n", addr);
        
        char *last_name = NULL;
        int file = 0;
        while(file_list[file])
        {
            char **entry = find_address(addr, file_list[file]);
            
            if(entry && entry[1] && entry[2])
            {
                csv[lines][2 + file * 3 + 0] = strdup(" ");
                csv[lines][2 + file * 3 + 1] = strdup(entry[1]);
                csv[lines][2 + file * 3 + 2] = strdup(entry[2]);
            }
            else
            {
                csv[lines][2 + file * 3 + 0] = strdup(" ");
                csv[lines][2 + file * 3 + 1] = strdup("");
                csv[lines][2 + file * 3 + 2] = strdup("");
            }
            file++;
        }
        
        mark_if_differ(csv[lines], files);
        find_preferred_name(csv[lines], files);
        
        lines++;
        address++;
    }
    
    return csv;
}

void print_csv(char ***csv)
{
    int line = 0;
    while(csv[line])
    {
        int column = 0;
        while(csv[line][column])
        {
            printf("%s;", csv[line][column]);
            column++;
        }
        printf("\n");
        line++;
    }
}


int main(int argc, char *argv[])
{
    int allocated = 10;
    int files = 0;
    char ****file_list = malloc(sizeof(char ***) * allocated);
    char *filenames[64];
    
    file_list[files] = NULL;
    
    for(int arg = 1; arg < argc; arg++)
    {
        char *file = argv[arg];
        char *ext = strrchr(file, '.');
        char ***new_file = NULL;
        
        if(ext && !strncasecmp(ext, ".s", 2))
        {
            printf("Reading stubs: '%s'\n", file);
            new_file = read_stubs(file);
        }
        else if(ext && !strncasecmp(ext, ".csv", 4))
        {
            printf("Reading csv: '%s'\n", file);
            new_file = read_csv(file);
        }
        else
        {
            printf("Unknown: '%s'\n", file);
        }
        
        if(new_file)
        {
            if(!realloc_helper(&file_list, &allocated, &files, sizeof(char ***)))
            {
                printf("Failed to reallocate file list\n");
                free_files(file_list);
                exit(1);
            }
            filenames[files] = strdup(file);
            file_list[files] = new_file;
            files++;
            file_list[files] = NULL;
        }
    }
    
    char **address_list = build_address_list(file_list);
    
    char ***merged = merge_lists(address_list, file_list, filenames);
    print_csv(merged);
}