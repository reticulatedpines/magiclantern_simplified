// Morse decoder from http://www.nerdkits.com/videos/morsedecoder/
// Original author: mrobbins@mit.edu
// Ported to Magic Lantern by Alex Dumitrache <broscutamaker@gmail.com>

#if 0
#include "dryos.h"
#include "bmp.h"
#include "menu.h"
#include "propvalues.h"

#define INTERSYMBOL_TIMEOUT 100
#define LONGSHORT_CUTOFF 20

int morse_pressed()
{
    if (get_halfshutter_pressed()) return 1;
    if (display_sensor && DISPLAY_SENSOR_POWERED) return 1;
    return 0;
}

uint8_t time_next_keypress() {
  uint16_t counter;
  
  // waits until the button is pressed
  // and then released. (OR timeout)
  //
  // returns timer cycles between press&release
  // or returns 255 on timeout
  
  // wait FOREVER until the button is released
  // (to prevent resets from triggering a new dit or dah)
  while(morse_pressed()) {}

  // turn off LED
  card_led_off();
  
  counter = 0;
  // wait until pressed
  while(!morse_pressed()) {
    msleep(10);
    counter++;
    
    if(counter == INTERSYMBOL_TIMEOUT) {
      // timeout #1: key wasn't pressed within X timer cycles.
      // (should happen between different symbols)
      return 255;
    }
  }

  // turn on LED
  card_led_on();
  
  // wait one cycle as a cheap "debouncing" mechanism
  msleep(10); // probably not needed in ML
  
  // wait until released
  counter = 0;
  while(morse_pressed()) {
    msleep(10);
    counter++;
    
    morse_redraw(counter < LONGSHORT_CUTOFF ? '.' : '-');
    
    if(counter == 255) {
      // timeout #2: key wasn't released within 255 timer cycles.
      // (should happen only to reset the screen)
      return 254;
    }
  }
  
  // turn off LED
  card_led_off();
  
  return counter;
}

// defining the lookup table.
// bits   7 6 5 4 3 2 1 0
// 765 define the length (0 to 7)
// 43210 define dits (0) and dahs (1)
#define MORSE_SIZE 26
#define MORSE(s, x)  ((s<<5) | x)
#define DIT(x) (x<<1)
#define DAH(x) ((x<<1) | 1)
unsigned char morse_coded[MORSE_SIZE] = 
{
  MORSE(2, DIT(DAH(0))),		//A
  MORSE(4, DAH(DIT(DIT(DIT(0))))),	//B
  MORSE(4, DAH(DIT(DAH(DIT(0))))),	//C
  MORSE(3, DAH(DIT(DIT(0)))),		//D
  MORSE(1, DIT(0)),			//E
  MORSE(4, DIT(DIT(DAH(DIT(0))))),	//F
  MORSE(3, DAH(DAH(DIT(0)))),		//G
  MORSE(4, DIT(DIT(DIT(DIT(0))))),	//H
  MORSE(2, DIT(DIT(0))),		//I
  MORSE(4, DIT(DAH(DAH(DAH(0))))),	//J
  MORSE(3, DAH(DIT(DAH(0)))),		//K
  MORSE(4, DIT(DAH(DIT(DIT(0))))),	//L
  MORSE(2, DAH(DAH(0))),		//M
  MORSE(2, DAH(DIT(0))),		//N
  MORSE(3, DAH(DAH(DAH(0)))),		//O
  MORSE(4, DIT(DAH(DAH(DIT(0))))),	//P
  MORSE(4, DAH(DAH(DIT(DAH(0))))),	//Q
  MORSE(3, DIT(DAH(DIT(0)))),		//R
  MORSE(3, DIT(DIT(DIT(0)))),		//S
  MORSE(1, DAH(0)),			//T
  MORSE(3, DIT(DIT(DAH(0)))),		//U
  MORSE(4, DIT(DIT(DIT(DAH(0))))),	//V
  MORSE(3, DIT(DAH(DAH(0)))),		//W
  MORSE(4, DAH(DIT(DIT(DAH(0))))),	//X
  MORSE(4, DAH(DIT(DAH(DAH(0))))),	//Y
  MORSE(4, DAH(DAH(DIT(DIT(0))))),	//Z
};
unsigned char morse_alpha[MORSE_SIZE] =
{ 
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

unsigned char morse_lookup(unsigned char in) {
  // linearly go through the table (in program memory)
  // and find the matching one
  uint8_t i;
  unsigned char tmp;
  
  for(i=0; i<MORSE_SIZE; i++) {
    tmp = morse_coded[i];
    
    if(tmp == in) {
      // matched morse character
      return morse_alpha[i];
    }
  }
  return '?';
}

unsigned char bitwise_reverse(unsigned char in, uint8_t max) {
  // maps bits backwards
  // i.e. for max=5
  // in  XXX43210
  // out YYY01234
  unsigned char b = 0;
  
  uint8_t i;
  for(i=0; i<max; i++) {
    if(in & (1<<i))
    b |= (1<< (max-1-i) );
  }

  return b;
}

char morse_code[100] = "";
char morse_string[100] = "";
#define STR_APPEND(orig,fmt,...) snprintf(orig + strlen(orig), sizeof(orig) - strlen(orig), fmt, ## __VA_ARGS__);

const char morse_table[] = 
"A .-     H ....   O ---   V ...- \n"
"B -...   I ..     P .--.  W .--  \n"
"C -.-.   J .---   Q --.-  X -..- \n"
"D -..    K -.-    R .-.   Y -.-- \n"
"E .      L .-..   S ...   Z --.. \n"
"F ..-.   M --     T -            \n"
"G --.    N -.     U ..-          \n"
;

int morse_enabled = 0;

static void morse_print(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Morse code input   : %s",
		morse_enabled ? "ON" : "OFF"
	);
}

struct menu_entry morse_menus[] = {
	{
		.name = "Morse code input",
        .priv = &morse_enabled,
		.select = menu_binary_toggle,
		.display = morse_print,
	},
};

void morse_redraw(int current_char)
{
    bmp_printf(FONT_LARGE, 0, 50, "%s%s     \n%s     ", morse_code, &current_char, morse_string);
    bmp_printf(FONT_SMALL, 0, 200, morse_table);
}

void morse_task(void* unused) 
{
    msleep(1000);
    menu_add("Debug", morse_menus, COUNT(morse_menus));
    
  unsigned counter=0;
  unsigned ditdahs=0; // counts dits and dahs along the top row
  int curchar=0, lastchar='_';
  unsigned chars=0;   // counts chars along the bottom row
  unsigned spacetimes=0;  // counts number of intersymbol times (before we call it a space)
  
  while(1) {
    if (!morse_enabled) { msleep(1000); continue; }
      
    counter = time_next_keypress();
    
    // decide what to do based on the timing
    if(counter == 254) {
      // clear everything
      clrscr();
      snprintf(morse_code, sizeof(morse_code), "");
      snprintf(morse_string, sizeof(morse_string), "");
      ditdahs = 0;
      chars = 0;
      curchar = 0;
      lastchar = '_';
      spacetimes = 0;
      
    } else if(counter == 255) {
    
      // intersymbol timeout: clear 1st row
      snprintf(morse_code, sizeof(morse_code), "");
      
      if(ditdahs > 0) {
        // lookup the character
        curchar = MORSE(ditdahs, bitwise_reverse(curchar, ditdahs));
        curchar = morse_lookup(curchar);
        
        // print it
        STR_APPEND(morse_string, "%s", &curchar);

        chars++;
        lastchar = curchar;
        spacetimes = 0;
      } else if(lastchar != '_') {
        spacetimes++;
        if(spacetimes == 4) {
          // as long as the last character wasn't a space, print a space 
          // (as an underscore so we can see it)
          curchar = '_';

          // print it
          STR_APPEND(morse_string, "%s", &curchar);
        
          chars++;
          lastchar = curchar;
        }
      }
      
      curchar = 0;
      ditdahs = 0;
    } else {

      // dit or dah
      if(counter >= LONGSHORT_CUTOFF) {
        // dah
        STR_APPEND(morse_code, "-");
        curchar = DAH(curchar);
      } else {
        // dit
        STR_APPEND(morse_code, ".");
        curchar = DIT(curchar);
      }
      
      ditdahs++;
      spacetimes = 0;
    }
    morse_redraw(0);
  }
  
  return 0;
}


TASK_CREATE( "morse_task", morse_task, 0, 0x19, 0x1000 );

#endif
