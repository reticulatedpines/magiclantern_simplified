#ifndef _CONSOLE_H_
#define _CONSOLE_H_

// console
OS_FUNCTION( 0x0800001,	void,	console_puts, const char* str );
OS_FUNCTION( 0x0800002,	int,	console_vprintf, const char* fmt, va_list ap );
OS_FUNCTION( 0x0800003,	int,	console_printf, const char* fmt, ... );



#endif // _CONSOLE_H_
