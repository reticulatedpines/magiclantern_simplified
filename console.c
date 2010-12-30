// script console

#include "bmp.h"
#include "dryos.h"
#include "menu.h"

int console_printf(const char* fmt, ...); // how to replace the normal printf?
#define printf console_printf

#define CONSOLE_W 50
#define CONSOLE_H 25
#define BUFSIZE (CONSOLE_W * CONSOLE_H + 1000)
char* console_buffer = "Script Console\n";
char* console_draw_buffer = 0;
int console_visible = 0;

static void
console_test( void * priv )
{
	console_visible = !console_visible;
	if (console_visible) printf("Hello World!\n");
	else printf("The quick brown fox jumps over the lazy dog. abcdefgfdwenfoewjfrfrejfrej\n");
}

static struct menu_entry script_menu[] = {
	{
		.priv		= "Console Test",
		.display	= menu_print,
		.select		= console_test,
	},
};

void console_init()
{
	console_buffer = AllocateMemory(BUFSIZE);
	console_draw_buffer = AllocateMemory(BUFSIZE);
	console_buffer[0] = 0;
	menu_add( "Script", script_menu, COUNT(script_menu) );
}

int console_printf(const char* fmt, ...)
{
	if (!console_buffer) return 0;
	int L = strlen(console_buffer);
	//~ bmp_printf(FONT_MED, 0, 0, "console buf: L=%d ", L);
	va_list			ap;
	va_start( ap, fmt );
	int len = vsnprintf( console_buffer + L, BUFSIZE - L, fmt, ap );
	va_end( ap );
	//~ bmp_printf(FONT_MED, 0, 30, "console buf: %s ", console_buffer);
	return len;
}

void console_draw() // reformat the buffer to handle newlines & wrap long lines
{
	#define NEW_CHAR(x) if (j >= BUFSIZE) return; console_draw_buffer[j] = x; j++;
	if (!console_buffer) return 0;
	if (!console_draw_buffer) return 0;
	
	int x = 0, y = 0;
	int i = 0; // index in console_buffer
	int j = 0; // index in console_draw_buffer
	int N = strlen(console_buffer);
	for (i = 0; i < N; i++)
	{
		char c = console_buffer[i];
		if (c == '\n') // fill the remaining of the line with spaces
		{
			while (x < CONSOLE_W)
			{
				NEW_CHAR(' ');
				x++;
			}
			x = 0;
			y++;
		}
		NEW_CHAR(c);
		x++;

		if (x > CONSOLE_W)
		{
			NEW_CHAR('\n');
			x = 0;
			y++;
		}
	}
	while (y < CONSOLE_H)
	{
		while (x < CONSOLE_W)
		{
			NEW_CHAR(' ');
			x++;
		}
		NEW_CHAR('\n');
		x = 0;
		y++;
	}
	NEW_CHAR(0);

	unsigned x0 = 720/2 - font_med.width * CONSOLE_W/2;
	unsigned y0 = 480/2 - font_med.height * CONSOLE_H/2;
	unsigned w = font_med.width * CONSOLE_W;
	unsigned h = font_med.height * CONSOLE_H;

	bmp_puts(FONT(FONT_MED,COLOR_WHITE,COLOR_BG_DARK), &x0, &y0, console_draw_buffer);
}


static void
console_task( void )
{
	while(1)
	{
		if (console_visible && !gui_menu_shown())
		{
			console_draw();
			msleep(10);
		}
		else
			msleep(1000);
	}
}

TASK_CREATE( "console_task", console_task, 0, 0x10, 0x1000 );


INIT_FUNC(__FILE__, console_init);


