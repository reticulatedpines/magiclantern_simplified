
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


int main (int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Invalid parameter count (%d)\n", argc);
        return -1;
    }
    FILE *f = fopen(argv[1], "r+");
    
    if(!f)
    {
        printf("Failed to open file\n");
        return -1;
    }
    
    uint32_t checksum = 0;
    uint32_t data = 0;
    uint32_t read = 0;
    
    /* make sure we read from the beginning */
    fseek(f, 0, SEEK_SET);
    
    while(!feof(f))
    {
        if(fread(&data, 4, 1, f) != 1)
        {
            break;
        }
        
        checksum ^= data;
        read++;
    }

    /* modify checksum */
    data ^= checksum;
    
    fseek(f, -4, SEEK_END);
   
    if(fwrite(&data, 4, 1, f) != 1)
    {
        printf("Failed to write\n");
        return -1;
    }
    
    fclose(f);
    
    return 0;
}