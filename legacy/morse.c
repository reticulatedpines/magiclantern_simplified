// Morse decoder from http://www.nerdkits.com/videos/morsedecoder/
// Original author: mrobbins@mit.edu
// Ported to Magic Lantern by Alex Dumitrache <broscutamaker@gmail.com>

#if 0
#include "dryos.h"
#include "bmp.h"
#include "menu.h"
#include "propvalues.h"

#define INTERSYMBOL_TIMEOUT 50
#define LONGSHORT_CUTOFF 15

int morse_enabled = 0;

int morse_pressed()
{
    if (HALFSHUTTER_PRESSED) return 1;
    if (display_sensor && DISPLAY_SENSOR_POWERED) return 1;
    return 0;
}

int morse_key = 0;

int handle_morse_keys(struct event * event)
{
    if (!morse_enabled) return 1;
    switch (event->param)
    {
        case BGMT_PRESS_RIGHT:
        case BGMT_WHEEL_RIGHT:
        case BGMT_WHEEL_DOWN:
            morse_key = 1;
            return 0;
        case BGMT_PRESS_LEFT:
        case BGMT_WHEEL_LEFT:
        case BGMT_WHEEL_UP:
            morse_key = 2;
            return 0;
        case BGMT_PRESS_SET:
            morse_key = 10; // return
            return 0;
        case BGMT_TRASH:
            morse_key = 8; // backspace
            return 0;
        case BGMT_MENU:
            morse_enabled = 0;
            redraw();
            return 0;
    }
    return 1;
}

int morse_get_key() {
  int counter;
  
  // waits until the button is pressed
  // and then released. (OR timeout)
  //
  // returns timer cycles between press&release
  // or returns 255 on timeout
  
  // wait FOREVER until the button is released
  // (to prevent resets from triggering a new dit or dah)
  while(morse_pressed()) msleep(10);

  // turn off LED
  card_led_off();
  
  counter = 0;
  // wait until pressed
  while(!morse_pressed()) {
    msleep(10);
    counter++;
    
    if (morse_key) { int m = morse_key; morse_key = 0; return m; }
    
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

  return counter >= LONGSHORT_CUTOFF ? 2 : 1;
}

// defining the lookup table.
// bits   7 6 5 4 3 2 1 0
// MSByte defines the length
// LSByte defines dits (0) and dahs (1)
#define MORSE_SIZE 26+10+17
#define MORSE(s, x)  ((s<<8) | x)
#define DIT(x) (x<<1)
#define DAH(x) ((x<<1) | 1)
unsigned int morse_coded[MORSE_SIZE] = 
{
  MORSE(2, DIT(DAH(0))),    //A
  MORSE(4, DAH(DIT(DIT(DIT(0))))),  //B
  MORSE(4, DAH(DIT(DAH(DIT(0))))),  //C
  MORSE(3, DAH(DIT(DIT(0)))),   //D
  MORSE(1, DIT(0)),     //E
  MORSE(4, DIT(DIT(DAH(DIT(0))))),  //F
  MORSE(3, DAH(DAH(DIT(0)))),   //G
  MORSE(4, DIT(DIT(DIT(DIT(0))))),  //H
  MORSE(2, DIT(DIT(0))),    //I
  MORSE(4, DIT(DAH(DAH(DAH(0))))),  //J
  MORSE(3, DAH(DIT(DAH(0)))),   //K
  MORSE(4, DIT(DAH(DIT(DIT(0))))),  //L
  MORSE(2, DAH(DAH(0))),    //M
  MORSE(2, DAH(DIT(0))),    //N
  MORSE(3, DAH(DAH(DAH(0)))),   //O
  MORSE(4, DIT(DAH(DAH(DIT(0))))),  //P
  MORSE(4, DAH(DAH(DIT(DAH(0))))),  //Q
  MORSE(3, DIT(DAH(DIT(0)))),   //R
  MORSE(3, DIT(DIT(DIT(0)))),   //S
  MORSE(1, DAH(0)),     //T
  MORSE(3, DIT(DIT(DAH(0)))),   //U
  MORSE(4, DIT(DIT(DIT(DAH(0))))),  //V
  MORSE(3, DIT(DAH(DAH(0)))),   //W
  MORSE(4, DAH(DIT(DIT(DAH(0))))),  //X
  MORSE(4, DAH(DIT(DAH(DAH(0))))),  //Y
  MORSE(4, DAH(DAH(DIT(DIT(0))))),  //Z
  MORSE(5, DIT(DAH(DAH(DAH(DAH(0)))))), //1
  MORSE(5, DIT(DIT(DAH(DAH(DAH(0)))))), //2
  MORSE(5, DIT(DIT(DIT(DAH(DAH(0)))))), //3
  MORSE(5, DIT(DIT(DIT(DIT(DAH(0)))))), //4
  MORSE(5, DIT(DIT(DIT(DIT(DIT(0)))))), //5
  MORSE(5, DAH(DIT(DIT(DIT(DIT(0)))))), //6
  MORSE(5, DAH(DAH(DIT(DIT(DIT(0)))))), //7
  MORSE(5, DAH(DAH(DAH(DIT(DIT(0)))))), //8
  MORSE(5, DAH(DAH(DAH(DAH(DIT(0)))))), //9
  MORSE(5, DAH(DAH(DAH(DAH(DAH(0)))))), //0
  MORSE(6, DIT(DAH(DIT(DAH(DIT(DAH(0))))))), // .
  MORSE(6, DAH(DAH(DIT(DIT(DAH(DAH(0))))))), // ,
  MORSE(6, DIT(DIT(DAH(DAH(DIT(DIT(0))))))), // ?
  MORSE(6, DIT(DAH(DAH(DAH(DAH(DIT(0))))))), // '
  MORSE(6, DAH(DIT(DAH(DIT(DAH(DAH(0))))))), // !
  MORSE(5, DAH(DIT(DIT(DAH(DIT(0)))))), // /
  MORSE(5, DAH(DIT(DAH(DAH(DIT(0)))))), // (
  MORSE(6, DAH(DIT(DAH(DAH(DIT(DAH(0))))))), // )
  MORSE(5, DIT(DAH(DIT(DIT(DIT(0)))))), // &
  MORSE(6, DAH(DAH(DAH(DIT(DIT(DIT(0))))))), // :
  MORSE(6, DAH(DIT(DAH(DIT(DAH(DIT(0))))))), // ;
  MORSE(5, DAH(DIT(DIT(DIT(DAH(0)))))), // =
  MORSE(5, DIT(DAH(DIT(DAH(DIT(0)))))), // +
  MORSE(6, DAH(DIT(DIT(DIT(DIT(DAH(0))))))), // -
  MORSE(6, DIT(DIT(DAH(DAH(DIT(DAH(0))))))), // _
  MORSE(6, DIT(DAH(DIT(DIT(DAH(DIT(0))))))), // "
  MORSE(6, DIT(DAH(DAH(DIT(DAH(DIT(0))))))), // *

};
unsigned char morse_alpha[MORSE_SIZE] =
{ 
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
  '.', ',', '?', '\'','!', '/', '(', ')', '&', ':', ';', '=', '+', '-', '_', '"', '*',
};

unsigned morse_lookup(unsigned in) {
  // linearly go through the table (in program memory)
  // and find the matching one
  unsigned i;
  unsigned tmp;
  
  for(i=0; i<MORSE_SIZE; i++) {
    tmp = morse_coded[i];
    
    if(tmp == in) {
      // matched morse character
      return morse_alpha[i];
    }
  }
  return '?';
}

unsigned bitwise_reverse(unsigned in, unsigned max) {
  // maps bits backwards
  // i.e. for max=5
  // in  XXX43210
  // out YYY01234
  unsigned b = 0;
  
  unsigned i;
  for(i=0; i<max; i++) {
    if(in & (1<<i))
    b |= (1<< (max-1-i) );
  }

  return b;
}

char morse_code[10] = "";

const char morse_table[] = 
"A .-     H ....   O ---   V ...-   3 ...--  0 -----  ( -.--.  - -....- \n"
"B -...   I ..     P .--.  W .--    4 ....-  . .-.-.- ) -.--.- _ ..--.- \n"
"C -.-.   J .---   Q --.-  X -..-   5 .....  , --..-- & .-...  \" .-..-. \n"
"D -..    K -.-    R .-.   Y -.--   6 -....  ? ..--.. : ---... * .--.-. \n"
"E .      L .-..   S ...   Z --..   7 --...  ' .----. ; -.-.-.          \n"
"F ..-.   M --     T -     1 .----  8 ---..  ! -.-.-- = -...-           \n"
"G --.    N -.     U ..-   2 ..---  9 ----.  / --..-. + .-.-.           \n"
;

static void morse_print(
  void *      priv,
  int     x,
  int     y,
  int     selected
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
    bmp_printf(FONT_LARGE, 570, 50, "%s%s        ", morse_code, &current_char);
    bmp_printf(FONT_SMALL, 0, 395, morse_table);
}

void morse_task(void* unused) 
{
    msleep(1000);
    menu_add("Debug", morse_menus, COUNT(morse_menus));
    
  unsigned key=0;
  unsigned ditdahs=0; // counts dits and dahs along the top row
  int curchar=0, lastchar='_';
  unsigned spacetimes=0;  // counts number of intersymbol times (before we call it a space)
  
  TASK_LOOP //{
    if (!morse_enabled) { msleep(1000); continue; }
    if (!flicker_being_killed()) kill_flicker();

    key = morse_get_key();
    
    // decide what to do based on the timing
    if(key == 254) {
      // clear everything
      console_clear();
      snprintf(morse_code, sizeof(morse_code), "");
      ditdahs = 0;
      curchar = 0;
      lastchar = '_';
      spacetimes = 0;
      
    } else if(key == 255) {
    
      // intersymbol timeout: clear 1st row
      snprintf(morse_code, sizeof(morse_code), "");
      
      if(ditdahs > 0) {
        // lookup the character
        curchar = MORSE(ditdahs, bitwise_reverse(curchar, ditdahs));
        curchar = morse_lookup(curchar);
        
        // print it
        console_puts(&curchar);
        /*switch (curchar)
        {
            case 'L': 
                fake_simple_button(BGMT_PRESS_LEFT); 
                fake_simple_button(BGMT_UNPRESS_LEFT); 
                break;
            case 'R': 
                fake_simple_button(BGMT_PRESS_RIGHT);
                fake_simple_button(BGMT_UNPRESS_RIGHT);
                break;
            case 'U': 
                fake_simple_button(BGMT_PRESS_UP);
                fake_simple_button(BGMT_UNPRESS_UP);
                break;
            case 'D': 
                fake_simple_button(BGMT_PRESS_DOWN);
                fake_simple_button(BGMT_UNPRESS_DOWN);
                break;
            case 'M': 
                fake_simple_button(BGMT_MENU);
                break;
            case 'Z': 
                fake_simple_button(BGMT_TRASH);
                break;
            case 'V': 
                fake_simple_button(BGMT_LV);
                break;
            case 'Q': 
                fake_simple_button(BGMT_Q);
                break;
            case 'P': 
                fake_simple_button(BGMT_PLAY);
                break;
            case 'S': 
                fake_simple_button(BGMT_PRESS_SET);
                fake_simple_button(BGMT_UNPRESS_SET);
                break;
            case 'X': 
                lens_take_picture(64);
                break;
        }*/

        lastchar = curchar;
        spacetimes = 0;
      } else if(lastchar != ' ' && lastchar != 10 && lastchar != 8) {
        spacetimes++;
        if(spacetimes == 4) {
          // as long as the last character wasn't a space, print a space 
          // (as an underscore so we can see it)
          curchar = ' ';

          // print it
          console_puts(" ");
        
          lastchar = curchar;
        }
      }
      curchar = 0;
      ditdahs = 0;
    } else if (key == 8) { // backspace
      if (ditdahs)
      {
        snprintf(morse_code, sizeof(morse_code), "");
        ditdahs = 0;
      }
      else
      {
        console_puts(&key);
      }
      lastchar = key;
      curchar = 0;
    } else if (key > 2) { // some char code
          console_puts(&key);
          lastchar = key;
          curchar = 0;
          ditdahs = 0;
    } else {
      // dit or dah
      if(key == 2) {
        // dah
        STR_APPEND(morse_code, "-");
        curchar = DAH(curchar);
      } else if (key == 1) {
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

#else
#include "bmp.h"
int handle_morse_keys(struct event * event) { return 1; }
#endif
