
#include <module.h>
#include <dryos.h>
#include <property.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>

#include "mxml-src/mxml.h"

/* mxml is using these. we have to implement them by hand */
unsigned int errno = 0;
FILE *stderr = NULL;

int putc(int c, FILE *stream)
{
    unsigned char data = c;
    
    FIO_WriteFile(stream, &data, 1);
    
    return data;
}

int getc(FILE *stream)
{
    unsigned char data = 0;
    
    FIO_ReadFile(stream, &data, 1);
    
    return data;
}

char *strerror(int errnum)
{
    return "UNKNOWN";
}

void *calloc(size_t nmemb, size_t size)
{
    void *buf = malloc(nmemb * size);
    memset(buf, 0x00, nmemb*size);
    
    return buf;
}

void *realloc(void *ptr, size_t size)
{
    void *buf = malloc(size);
    
    /* this will read beyond end of ptr and writing that threash to buf, but thats okay */
    memcpy(buf, ptr, size);
    free(ptr);
    
    return buf;
}

/* not using mxmlSaveFd, so read/write is unused */
ssize_t write(int fd, const void *buf, size_t count)
{
    return 0;
}

unsigned int module_config_load(char *filename, module_entry_t *module)
{
    module_config_t *config = module->config;
    
    if(config)
    {
        mxml_node_t *node = NULL;
        mxml_node_t *tree = NULL;
        
        /* read and parse the xml */
        FILE *file = FIO_Open(filename, O_RDONLY | O_SYNC);
        if( file == INVALID_PTR )
        {
            return 1;
        }
        tree = mxmlLoadFile(NULL, file, MXML_NO_CALLBACK);
        
        FIO_CloseFile(file);
        
        /* is this a valid config xml? */
        if(strcmp(tree->value.element.name, "config") || !tree->child )
        {
            return 1;
        }
                
        /* get config root */
        node = tree->child;
        
        /* check for all registered config variables */
        while(config->name)
        {
            mxml_node_t *element = node;
            
            while(element)
            {
                /* check for xml elements with the variable name */
                if(!strcmp(element->value.element.name, config->ref->name))
                {
                    /* the child element is the number as text */
                    if(element->child && element->child->type == MXML_TEXT)
                    {
                        *config->ref->value = atoi(element->child->value.text.string);
                    }
                    else
                    {
                        return 2;
                    }
                }
                element = element->next;
            }
            config++;
        }
        
    }
    return 0;
}

unsigned int module_config_save(char *filename, module_entry_t *module)
{
    module_config_t *config = module->config;
    
    if(config)
    {
        mxml_node_t *node = NULL;
        mxml_node_t *tree = mxmlNewElement(MXML_NO_PARENT, "config");
        
        /* go through out structure where we reference the old CONFIG structure elements */
        while(config->name)
        {
            node = mxmlNewElement(tree, config->ref->name);
            node = mxmlNewInteger(node, *config->ref->value);
            config++;
        }
        
        /* create the config file */
        FILE *file = FIO_CreateFileEx(filename);
        if( file == INVALID_PTR )
        {
            mxmlDelete(tree);
            return 1;
        }
        
        /* save to card and release structures */
        mxmlSaveFile(tree, file, MXML_NO_CALLBACK);
        mxmlDelete(tree);
        
        FIO_CloseFile(file);
    }
    return 0;
}

static unsigned int mxml_init()
{
    return 0;
}

static unsigned int mxml_deinit()
{
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(mxml_init)
    MODULE_DEINIT(mxml_deinit)
MODULE_INFO_END()

MODULE_STRINGS_START()
    MODULE_STRING("Description", "XML Library")
    MODULE_STRING("License", "LGPLv2")
    MODULE_STRING("Author", "g3gg0")
    MODULE_STRING("Credits", "minixml.org")
MODULE_STRINGS_END()

