#ifndef _CONSOLE_H_
#define _CONSOLE_H_

// console
void console_puts( const char* str );
int console_vprintf( const char* fmt, va_list ap );
int console_printf( const char* fmt, ... );



#endif // _CONSOLE_H_
