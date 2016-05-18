#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#include "sysemu/sysemu.h"
#include "eos.h"
#include "eos_ml_helpers.h"


static char* ml_fio_translate_filename(char* filename);


unsigned int eos_handle_ml_helpers ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    if(type & MODE_WRITE)
    {
        switch (address)
        {
            case REG_PRINT_CHAR:    /* print in blue */
                printf("\x1B[34m%c\x1B[0m", (uint8_t)value);
                return 0;

            case REG_PRINT_NUM:     /* print in green */
                printf("\x1B[32m%x (%d)\x1B[0m\n", (uint32_t)value, (uint32_t)value);
                return 0;

            case REG_SHUTDOWN:
                printf("Goodbye!\n");
                qemu_system_shutdown_request();
                return 0;
            
            case REG_BMP_VRAM:
                s->disp.bmp_vram = (uint32_t) value;
                return 0;

            case REG_IMG_VRAM:
                s->disp.img_vram = (uint32_t) value;
                if (value)
                {
                    eos_load_image(s, "LV-000.422", 0, -1, value, 0);
                }
                else
                {
                    printf("Image buffer disabled\n");
                }
                return 0;
            
            case REG_RAW_BUFF:
                s->disp.raw_buff = (uint32_t) value;
                if (value)
                {
                    /* fixme: hardcoded strip offset */
                    eos_load_image(s, "RAW-000.DNG", 33792, -1, value, 1);
                }
                else
                {
                    printf("Raw buffer disabled\n");
                }
                return 0;

            case REG_DISP_TYPE:
                s->disp.type = (uint32_t) value;
                return 0;
        }
    }
    else
    {
        switch (address)
        {
            case REG_GET_KEY:
            {
                if (s->keyb.head == s->keyb.tail)
                {
                    /* no key in queue */
                    return 0;
                }
                else
                {
                    /* return a key from the circular buffer */
                    return s->keyb.buf[(s->keyb.head++) & 15];
                }
            }

            case REG_BMP_VRAM:
                return s->disp.bmp_vram;

            case REG_IMG_VRAM:
                return s->disp.img_vram;
            
            case REG_RAW_BUFF:
                return s->disp.raw_buff;
            
            case REG_DISP_TYPE:
                return s->disp.type;
        }
        return 0;
    }
    return 0;
}


unsigned int eos_handle_ml_fio ( unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    static int arg0, arg1, arg2, arg3;
    static char buffer[1000] = {0};
    static uint32_t  buffer_index = 0;
    
    static char dirname[1000];
    static DIR * dir;
    static FILE* file;
    
    if(type & MODE_WRITE)
    {
        switch (address)
        {
            case REG_FIO_NUMERIC_ARG0:
                arg0 = value;
                return 0;

            case REG_FIO_NUMERIC_ARG1:
                arg1 = value;
                return 0;

            case REG_FIO_NUMERIC_ARG2:
                arg2 = value;
                return 0;

            case REG_FIO_NUMERIC_ARG3:
                arg3 = value;
                return 0;
            
            case REG_FIO_BUFFER:
                if (buffer_index < COUNT(buffer))
                {
                    buffer[buffer_index++] = value;
                }
                return 0;
            
            case REG_FIO_BUFFER_SEEK:
                buffer_index = value;
                return 0;
        }
    }
    else
    {
        switch (address)
        {
            case REG_FIO_NUMERIC_ARG0:
                return arg0;

            case REG_FIO_NUMERIC_ARG1:
                return arg1;

            case REG_FIO_NUMERIC_ARG2:
                return arg2;

            case REG_FIO_NUMERIC_ARG3:
                return arg3;

            case REG_FIO_BUFFER:
                if (buffer_index < COUNT(buffer))
                {
                    return buffer[buffer_index++];
                }

            case REG_FIO_GET_FILE_SIZE:
            {
                struct stat sb;
                char* filename = ml_fio_translate_filename(buffer);
                if (stat(filename, &sb) != -1)
                {
                    printf("[FIO wrapper] GetFileSize(%s) => %d\n", filename, (int)sb.st_size);
                    return sb.st_size;
                }
                else
                {
                    printf("[FIO wrapper] GetFileSize(%s) error\n", filename);
                    return -1;
                }
            }

            case REG_FIO_OPENFILE:
            {
                char* filename = ml_fio_translate_filename(buffer);

                if (file == 0)
                {
                    file = fopen(filename, "rb");
                    if (file)
                    {
                        printf("[FIO wrapper] openfile(%s)\n", filename);
                        return 1;
                    }
                    else
                    {
                        printf("[FIO wrapper] openfile(%s) error\n", filename);
                    }
                }
                else
                {
                    printf("[FIO wrapper] openfile(%s) already in use\n", filename);
                }
                return 0;
            }
            
            case REG_FIO_CLOSEFILE:
            {
                if (file)
                {
                    fclose(file);
                    file = 0;
                    printf("[FIO wrapper] closefile()\n");
                    return 1;
                }
                else
                {
                    printf("[FIO wrapper] closefile() nothing open\n");
                }
                return 0;
            }

            case REG_FIO_READFILE:
            {
                int size = MIN(arg0, sizeof(buffer));
                int pos = arg1;
                buffer[0] = buffer_index = 0;
                
                if (file)
                {
                    //~ printf("[FIO wrapper] reading %d bytes from pos=%d\n", size, pos);
                    fseek(file, pos, pos >= 0 ? SEEK_SET : SEEK_END);
                    return fread(buffer, 1, size, file);
                }
                else
                {
                    printf("[FIO wrapper] readfile() nothing open\n");
                }
                return 0;
            }
            
            case REG_FIO_OPENDIR:
            {
                char* path = ml_fio_translate_filename(buffer);

                if (dir == 0)
                {
                    dir = opendir(path);
                    if (dir)
                    {
                        int len = snprintf(dirname, sizeof(dirname), "%s", path);
                        if (dirname[len-1] == '/') dirname[len-1] = 0;
                        printf("[FIO wrapper] opendir(%s)\n", path);
                        return 1;
                    }
                    else
                    {
                        printf("[FIO wrapper] opendir(%s) error\n", path);
                    }
                }
                else
                {
                    printf("[FIO wrapper] opendir(%s) already in use\n", path);
                }
                return 0;
            }
            
            case REG_FIO_CLOSEDIR:
            {
                if (dir)
                {
                    closedir(dir);
                    dir = 0;
                    printf("[FIO wrapper] closedir()\n");
                    return 1;
                }
                else
                {
                    printf("[FIO wrapper] closedir() nothing open\n");
                }
                return 0;
            }

            case REG_FIO_READDIR:
            {
                buffer[0] = buffer_index = 0;
                arg0 = arg1 = arg2 = 0;
                
                if (dir)
                {
                    struct dirent * d = readdir(dir);
                    if (d)
                    {
                        snprintf(buffer, sizeof(buffer), "%s", d->d_name);

                        char fullpath[1000];
                        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, d->d_name);
                        
                        struct stat sb;
                        if (stat(fullpath, &sb) != -1)
                        {
                            arg0 = sb.st_size;
                            arg1 = S_ISDIR(sb.st_mode) ? 0x10 : 0;
                            arg2 = sb.st_mtime;
                        }
                        
                        printf("[FIO wrapper] readdir() => %s size=%d mode=%x time=%x\n", buffer, arg0, arg1, arg2);
                        return 1;
                    }
                }
                else
                {
                    printf("[FIO wrapper] closedir() nothing open\n");
                }
                return 0;
            }
        }
    }
    return 0;
}

static char* ml_fio_translate_filename(char* filename)
{
    static char buf[1000];
    char* f;
    
    for (f = filename; *f; f++)
    {
        if (*f == '.' && *(f+1) == '.')
        {
            printf("Hacker detected :) %s\n", filename);
            goto err;
        }
    }
    
    if (filename[0] == 'A' && filename[1] == ':' && filename[2] == '/')
    {
        snprintf(buf, sizeof(buf), "cfcard/%s", filename+3);
        goto ok;
    }
    
    if (filename[0] == 'B' && filename[1] == ':' && filename[2] == '/')
    {
        snprintf(buf, sizeof(buf), "sdcard/%s", filename+3);
        goto ok;
    }

err:
    printf("[FIO wrapper] invalid filename (%s)\n", filename);
    buf[0] = 0;
    return buf;

ok:
    /* files copied from a FAT32 card are all uppercase */
    for (f = buf+6; *f; f++)
    {
        *f = toupper(*f);
    }
    return buf;
}


