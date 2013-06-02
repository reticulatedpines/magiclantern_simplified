#ifndef _file_man_h_
#define _file_man_h_

/* the file handler should process a subset of these commands (0 or more) */
/* should return 0 if command was not processed, 1 on success, negative on error */
typedef int (*filetype_handler_func)( unsigned int cmd, char *filename, char *data );
#define FILETYPE_HANDLER(func) static int func ( unsigned int cmd, char *filename, char *data )

#define FILEMAN_CMD_INFO 0                  /* return a info string */
#define FILEMAN_CMD_VIEW_IN_MENU 1          /* for simple things, e.g. text, image */
#define FILEMAN_CMD_VIEW_OUTSIDE_MENU 2     /* for more complex things, e.g. playing a movie */

/* not yet implemented */
#define FILEMAN_CMD_VIEW_THUMBNAIL 3        /* small preview when hovering or selecting a file (not implemented yet) */
#define FILEMAN_CMD_OPEN 4                  /* open the file externally (e.g. edit a script, load a module) */

#ifndef _file_man_c_
extern WEAK_FUNC(ret_1) unsigned int fileman_register_type(char *ext, char *type, filetype_handler_func handler);
#endif

#endif
