#include <module.h>
#include <dryos.h>
#include <bmp.h>
#include <config.h>
#include <menu.h>
#include <font.h>
#include <beep.h>
#include "arkanoid.h"

#include <cordic-16bit.h>
#define M_PI 3.1415926536897932384626f

// start elem
static element elem = {0};
// basic elem loop
#define ELEM_LOOP(code) { element *temp; element *e; for(temp = e = &elem; temp; temp = e = temp->next) { code; } }

// elem calculations
#define COERCE_ABS(i, min, max) if(i < min) { i = (min) - (i - (min)); } if(i > max) { i = (max) - (i - (max)); }
#define EXCLUDE_RANGE(i, min, max) if(i > min && i < max) { if(i > ((max) - (min)) / 2) i = max; else i = min;}

// elem types
#define ELEM_NULL 0
#define ELEM_PAD 1
#define ELEM_BALL 2
#define ELEM_BRICK 3
#define ELEM_FALL_BRICK 4
#define ELEM_ML 5
#define ELEM_PRESENT 6

// is arkanoid active?
static bool arkanoid_running = 0;
// running states
#define ARK_IDLE 0  // reset
#define ARK_INRO 1
#define ARK_PRESENT 2
#define ARK_LOGO 3
#define ARK_NEW_GAME 4
#define ARK_PLAY 5
// should be read only
static int arkanoid_state = -1;
// set to whatever you want
static int arkanoid_next_state = ARK_IDLE;

// sound
static int sound_event;
#define SOUND_EVENT_COLLISION 1
#define SOUND_EVENT_BALL_LOST 2
#define SOUND_EVENT_ALL_BALLS_LOST 4

// last key pressed
static int last_key;
// count bricks
static int brick_count;
// count bals
static int ball_count;

// configs
static CONFIG_INT("games.arkanoid.level", level, 1);
static CONFIG_INT("games.arkanoid.sound", sound, 0);

// extern
extern int menu_redraw_blocked;
extern int menu_shown;

// logo graphics
#define LOGO_ARR_LEN 153
static int logo_arr[LOGO_ARR_LEN+1][2] = {{268,48}, {147,57}, {229,46}, {269,72}, {229,70}, {159,81}, {195,83}, {269,83}, {229,81}, {279,45}, {147,46}, {148,81}, {290,50}, {158,46}, {288,61}, {167,52}, {170,63}, {211,74}, {206,83}, {197,47}, {288,75}, {251,83}, {240,82}, {203,65}, {297,83}, {167,74}, {193,57}, {209,46}, {186,78}, {279,67}, {269,61}, {147,69}, {229,58}, {133,216}, {328,215}, {193,219}, {459,195}, {548,204}, {459,255}, {559,263}, {258,200}, {526,201}, {387,205}, {439,200}, {441,212}, {124,239}, {314,252}, {195,249}, {258,226}, {526,238}, {526,227}, {548,249}, {387,249}, {408,222}, {119,250}, {178,264}, {308,263}, {195,264}, {258,253}, {258,239}, {526,263}, {525,250}, {387,262}, {424,244}, {139,202}, {341,189}, {192,205}, {467,189}, {548,190}, {458,243}, {457,230}, {548,234}, {257,187}, {526,188}, {387,190}, {440,188}, {147,188}, {332,200}, {191,190}, {478,187}, {561,189}, {154,201}, {349,199}, {397,200}, {218,190}, {204,188}, {490,187}, {573,192}, {161,214}, {371,250}, {349,228}, {416,232}, {230,197}, {500,193}, {505,204}, {584,200}, {509,239}, {507,228}, {596,222}, {596,233}, {297,188}, {164,228}, {169,241}, {358,212}, {402,211}, {227,248}, {219,238}, {222,222}, {269,231}, {270,219}, {152,230}, {317,240}, {233,210}, {506,217}, {592,210}, {492,264}, {508,250}, {502,259}, {592,244}, {582,254}, {287,199}, {172,252}, {375,262}, {367,238}, {362,226}, {237,262}, {293,262}, {257,265}, {279,241}, {285,250}, {141,230}, {115,262}, {324,228}, {208,229}, {478,264}, {572,260}, {280,210}, {129,228}, {338,229}, {194,232}, {457,218}, {457,206}, {548,219}, {466,263}, {548,262}, {257,212}, {526,214}, {387,219}, {387,234}, {441,223}, {440,236}, {441,248}, {432,254}, {441,263}};

static element* new_elem(int type) {
    // allocate new elem
    element *new = (element*)malloc(sizeof(element));
    memset(new, 0, sizeof(element));
    new->type = type;
    
    // add next reference at the end of elems
    element *end = &elem;
    while(end->next) end = end->next;
    end->next = new;
    
    // add previous reference
    new->prev = end;
    
    return new;
}

static void delete_elem(element *e) {
    e->prev->next = e->next;
    e->next->prev = e->prev;
    free(e);
}

static void reset_elems() {
    element *e = (&elem)->next;
    element *temp;
    while(e) {
        temp = e;
        e = e->next;
        free(temp);
    }
    elem.next = 0;
}

static void arkanoid_draw_elem(element * e, int x, int y, int color)
{
    switch(e->type) {
        case ELEM_PAD:
            bmp_draw_rect_chamfer(color, x, y, e->w, e->h, 4, 0);
            bmp_draw_rect_chamfer(color, x + 1, y + 1, e->w - 2, e->h - 2, 4, 0);
            break;
        case ELEM_BALL:
            bmp_draw_rect_chamfer(color, x, y, e->w, e->h, (e->w + e->h) / 8, 0);
            break;
        case ELEM_BRICK:
            bmp_draw_rect_chamfer(color, x, y, e->w, e->h, 2, 0);
            break;
        case ELEM_FALL_BRICK:
            bmp_draw_rect_chamfer(color, x, y, e->w, e->h, 2, 0);
            bfnt_draw_char(e->c1, x, y, color, 0);
            break;
        case ELEM_ML:
            bmp_printf(FONT(FONT_LARGE, color, 0), 720 / 2 - 13 * font_large.width / 2, 480 / 2 - font_large.width / 2, "Magic Lantern");
            break;
        case ELEM_PRESENT:
            bmp_printf(FONT(FONT_LARGE, color, 0), 720 / 2 - 7 * font_large.width / 2, 480 / 2 - font_large.width / 2, "present");
            break;
    }
    //bmp_printf(FONT_MED, (int)e->x, (int)e->y, "%d", e->c1);
}

static void arkanoid_redraw()
{
    // erase elements that changed their position (to minimize flicker)
    ELEM_LOOP (
        if (e->old_x != e->x || e->old_y != e->y)
        {
            arkanoid_draw_elem(e, e->old_x, e->old_y, 0);
        }
    )

    ELEM_LOOP
    (
        // remove deleted elements from simulation
        if (e->deleted)
        {
            arkanoid_draw_elem(e, e->x, e->y, 0);
            delete_elem(e);
            continue;
        }
        
        // draw the rest
        arkanoid_draw_elem(e, e->x, e->y, e->color);
        
        // keep track of old position
        e->old_x = e->x;
        e->old_y = e->y;
    )
}

static int last_delta() {
    if(last_key == MODULE_KEY_PRESS_LEFT) return -1;
    if(last_key == MODULE_KEY_PRESS_RIGHT) return 1;
    return 0;
}

static void fade(element *e, int fade_delta) {
    e->fade_delta = fade_delta;
}

static void fade_set(element *e, int fade_delta, int start) {
    e->fade_delta = fade_delta;
    e->fade = start;
}

static void generate_level() {
    int x, y;
    int width = 0;
    int i = - NUM_ML_ICONS - 2 + rand()%100;
    for(y = 20; y < 380;y += 44)
    {
        for(x = 50; x < 720 - 50; )
        {
            if((level * level < rand() % 100))
            {
                x += rand()%20;
                continue;
            }
            
            element *e = new_elem(ELEM_BRICK);
            brick_count++; 
            
            while(i++<100)
            {
                width = bfnt_char_get_width(i);
                if (width > 0 && width < 100) break;
            }
            
            e->x = x;
            e->y = y;
            e->w = width;
            e->h = 40;
            e->c1 = i;
            fade(e, 1 + (rand() % 5));
            
            x += e->w + 5;
        }
        
        if((rand() % (level * 100)) < 95) y += 22;
    }
}

static int hit_test_test(element *a, element *b) {
    if (
            a->x + a->w >= b->x &&
            a->x <= b->x + b->w &&
            a->y + a->h >= b->y &&
            a->y <= b->y + b->h
        ) return 1;
    return 0;
}

static void set_direction(element *e, int angle) {
    int s,c;
    
    bool reverse = 0;
    angle %= 360;
    if(angle > 180) { //cordic funcion doesnt have full 2PI range
        angle %= 180;
        reverse = 1;
    }
    
    float p = M_PI * (- 0.5f) + (angle / 180.0f) * M_PI;
    
    cordic((p*MUL), &s, &c, 11); //11 for 1 degree accuracy
    
    if(!reverse) {
        s *= -1;
        c *= -1;
    }
    
    e->deltaX = s / MUL;
    e->deltaY = c / MUL;
}

static element* new_ball() {
    element *e = new_elem(ELEM_BALL);
    
    e->w = 10;
    e->h = 10;
    e->x = 720 / 2 - e->w / 2;
    e->y = 480 / 2 - e->h / 2;
    e->color = COLOR_WHITE;
    
    set_direction(e, rand() % 360);
    
    e->speed = 3 + rand()%300 / 100.0;
    
    return e;
}

static void handle_fades(element *e) {
    if(e->fade_delta == 0) return;
    
    e->fade += e->fade_delta;
    e->fade = COERCE(e->fade, 0, 100);
    e->color = COLOR_GRAY(e->fade);
    
    if(e->fade == 0 || e->fade == 100) e->fade_delta = 0;
    if(e->fade == 0) e->deleted = 1;
}

// state transition, to be called only from arkanoid task
static void arkanoid_game_init() {
    brick_count = ball_count = 0;
    
    reset_elems();
    
    element *p = new_elem(ELEM_PAD);
    p->w = MIN(60 * level, 720);
    p->h = 20;
    p->x = 720 / 2 - p->w / 2;
    p->y = 450;
    p->speed = 15;
    p->color = COLOR_WHITE;
    
    
    int i = 1;
    while(i++ <= level)
    {
        element *e = new_ball();
        ball_count++;
        e->x = (720 / 2) - (level * (e->w + 5) / 2) + (i * (e->w + 5));
        e->y = p->y - e->h;
        e->speed = 0;
        set_direction(e, -90);
    }
    
    generate_level();
}

// state transition, to be called only from arkanoid task
static void arkanoid_game_start() {
    ELEM_LOOP
    (
        if(e->type != ELEM_BALL) continue;
        e->speed = 5 + (level * 5);
    )
}

static void hit_test(element *a) {
    ELEM_LOOP
    (
        if(e->type != ELEM_PAD && e->type != ELEM_BRICK) continue;
        if(!hit_test_test(a, e)) continue;
        
        sound_event |= SOUND_EVENT_COLLISION;
        
        if(e->type == ELEM_PAD) {
            int angle = 180 - ABS((a->x + a->w / 2) - e->x) / e->w * 180;
            angle = COERCE(angle, 10, 170);
            set_direction(a, angle);
        }
        else {
            int ball_center;
            ball_center = a->x + a->w / 2;
            int x_len = MIN(
                ABS(ball_center - e->x),
                ABS(ball_center - (e->x + e->w))
            );
            
            ball_center = a->y + a->h / 2;
            int y_len = MIN(
                ABS(ball_center - e->y),
                ABS(ball_center - (e->y + e->h))
            );
            
            if(x_len < y_len)a->deltaX *= -1;
            else a->deltaY *= -1;
            
            e->type = ELEM_FALL_BRICK;
            e->z = 1;
            e->color = COLOR_GRAY(30);
            e->speed = 5 + rand()%5;
            
            if(--brick_count == 0) {
                level = MIN(level + 1, 10);
                arkanoid_next_state = ARK_NEW_GAME;
            }
        }
    )
}

// state transition, to be called only from arkanoid task
static void arkanoid_logo() {
    // hide all leave balls and count balls
    int bals = 0;
    ELEM_LOOP
    (
        if(e->type != ELEM_BALL) fade(e, -10);
        else bals++;
    )
    
    // add new balls
    bals -= LOGO_ARR_LEN + 50;
    while(bals++ < 0) {
        element* e = new_ball();
        fade(e, 5);
    }
    
    // logo assoc
    ELEM_LOOP
    (
        if(e->type != ELEM_BALL) continue;
        e->c1 = -1;
    )
    
    element* closest = &elem;
    for(int i = 0; i != LOGO_ARR_LEN; i++) {
        int dist = INT_MAX;
        ELEM_LOOP
        (
            if(e->type != ELEM_BALL || e->c1 != -1) continue;
            
            int manhattan = ABS(e->x - logo_arr[i][0]) + ABS(e->y - logo_arr[i][1]);
            if(manhattan < dist) {
                dist = manhattan;
                closest = e;
            }
        )
        closest->c1 = i;
        closest->speed = 0.2;
    }
}

// state transition, to be called only from arkanoid task
static void arkanoid_intro() {
    reset_elems();
    
    element *e = new_elem(ELEM_ML);
    fade(e, 2);
}

static void arkanoid_present() {
    element *e = new_elem(ELEM_PRESENT);
    fade(e, 4);
}








static void ml_ef(element* e) {
    if(arkanoid_state != ARK_INRO) return;
    
    element* b = new_ball();
    fade(b, 2);
    
    if(e->fade == 100) fade(e, -2);
    
    if(e->fade == 0) arkanoid_next_state = ARK_PRESENT;
}

static void present_ef(element* e) {
    if(arkanoid_state != ARK_PRESENT) return;
    
    element* b = new_ball();
    fade(b, 3);

    if(e->fade == 100) fade(e, -3);
    
    if(e->fade == 0) arkanoid_next_state = ARK_LOGO;
}

static void ball_coerce(element* e) {
    if(e->x < 0 || e->x > 720 - e->w) e->deltaX *= -1;
    if(e->y < 0 || e->y > 480 - e->h) e->deltaY *= -1;

    COERCE_ABS(e->x, 0, 720 - e->w);
    COERCE_ABS(e->y, 0, 480 - e->h);
}

static void ball_ef(element* e) {
    float plusX;
    float plusY;
    
    // logo assoc
    if(arkanoid_state == ARK_LOGO && e->c1 != -1) {
        plusX = logo_arr[e->c1][0] - e->x;
        plusY = logo_arr[e->c1][1] - e->y;
        e->x += plusX * 0.1;
        e->y += plusY * 0.1;
        if(!( rand() % 2)) set_direction(e, rand() % 360);
    }
    
    // movement is computed as deltaX/Y (sin cos (-1, +1) ) * number of steps (speed)
    float step = (float)e->speed;
    int x, y;
    int last_x = INT_MAX;
    int last_y = INT_MAX;
    while(step-- > 0) {
        
        float cur_step = step < 1 ? step : 1;
        
        e->x += e->deltaX * cur_step;
        e->y += e->deltaY * cur_step;
        
        ball_coerce(e);
        
        // collision only and ball lose in gameplay
        if(arkanoid_state == ARK_PLAY) {
            x = (int)e->x;
            y = (int)e->y;
            
            if(ABS(x - last_x) > e->w || ABS(y - last_y) > e->h) {
                hit_test(e);
                last_y = y;
                last_x = x;
            }
            
            if(e->y > 460) {
                e->deleted = 1;
                if(--ball_count == 0) {
                    arkanoid_next_state = ARK_LOGO;
                    sound_event |= SOUND_EVENT_ALL_BALLS_LOST;
                }
                else
                {
                    sound_event |= SOUND_EVENT_BALL_LOST;
                }
                break;
            }
        }
    }
}

static void pad_ef(element* e) {
    if(arkanoid_state != ARK_PLAY) return;
    e->x = COERCE(e->x + last_delta() * e->speed, 0, 720 - e->w);
}

static void fall_brick_ef(element* e) {
    e->y += e->speed;
    if(e->y > 480) e->deleted = 1;
}

static void arkanoid_task()
{
    clrscr();
    
    arkanoid_running = 1;
    menu_redraw_blocked = 1;
    last_key = 0;
    
    TASK_LOOP
    {
        // if menu is not shown OR if halfshutter is pressed > quit
        if(!gui_menu_shown() || last_key == MODULE_KEY_PRESS_HALFSHUTTER) goto quit;
        
        // pause
        if(last_key == MODULE_KEY_TRASH) goto frame_skip;
        
        // change the state
        if (arkanoid_next_state != arkanoid_state)
        {
            switch (arkanoid_next_state)
            {
                case ARK_IDLE:
                case ARK_INRO:
                    arkanoid_intro();
                    // rewrite the AKR_IDLE
                    arkanoid_next_state = ARK_INRO;
                    break;
                case ARK_PRESENT:
                    arkanoid_present();
                    break;
                case ARK_LOGO:
                    arkanoid_logo();
                    break;
                case ARK_NEW_GAME:
                    arkanoid_game_init();
                    break;
                case ARK_PLAY:
                    arkanoid_game_start();
                    break;
            }
            arkanoid_state = arkanoid_next_state;
            clrscr();
        }
        
        ELEM_LOOP
        (
            handle_fades(e);
            
            switch (e->type) {
                case ELEM_ML: ml_ef(e); break;
                case ELEM_PRESENT: present_ef(e); break;
                case ELEM_BALL: ball_ef(e); break;
                case ELEM_PAD: pad_ef(e); break;
                case ELEM_FALL_BRICK: fall_brick_ef(e); break;
            }
        )
        
        if (sound)
        {
            // play the most important event
            if (sound_event & SOUND_EVENT_ALL_BALLS_LOST)
            {
                info_led_on();
                beep_custom(1000, 200, 0);
                info_led_off();
            }
            else if (sound_event & SOUND_EVENT_BALL_LOST)
            {
                info_led_on();
                beep_custom(300, 200, 0);
                info_led_off();
            }
            else if (sound_event & SOUND_EVENT_COLLISION)
            {
                // regular collision
                beep_custom(20, 750 + rand()%500, 0);
            }
            sound_event = 0;
        }
        
        
        frame_skip:
        
        arkanoid_redraw();
        msleep(40);
    }
    
    quit:
    
    clrscr();
    arkanoid_running = 0;
    menu_redraw_blocked = 0;
}

static MENU_SELECT_FUNC(arkanoid_start) {
    task_create("arkanoid_task", 0x1c, 0x1000, arkanoid_task, (void*)0);
}

static MENU_SELECT_FUNC(level_select) {
    level += delta;
    level = COERCE(level, 1, 10);
    
    // if we are playing reset the game
    if (arkanoid_state == ARK_PLAY) arkanoid_next_state = ARK_LOGO;
}

static struct menu_entry arkanoid_menu[] =
{
    {
        .name = "Arkanoid",
        .select = arkanoid_start,
        .help = "Second game for Magic Lantern. Try it in LiveView.",
        .children = (struct menu_entry[])
        {
            {
                .name = "Level",
                .priv = &level,
                .select = level_select,
                .max = 10,
                .min = 1,
            },
            {
                .name = "Sound",
                .priv = &sound,
                .max = 1,
            },
            MENU_EOL,
        }
    }
};

static unsigned int arkanoid_init() {
    menu_add("Games", arkanoid_menu, COUNT(arkanoid_menu));
    return 0;
}

static unsigned int arkanoid_deinit()
{
    return 0;
}

static unsigned int arkanoid_keypress(unsigned int key)
{
    // if arakanoid is not running do nothing
    if(!arkanoid_running) return 1;
    
    // save last key to decide later what we should do
    last_key = key;
    
    switch(key) {
        // Q and PicStyle reset the game
        case MODULE_KEY_Q:
        case MODULE_KEY_PICSTYLE:
            arkanoid_next_state = ARK_IDLE;
            break;
        
        // trash pauses the game
        case MODULE_KEY_TRASH:
            // implemented in arkanoid_task
            break;
        
        // halfshutter quits the game
        case MODULE_KEY_PRESS_HALFSHUTTER:
            // implemented in arkanoid_task
            break;
        
        // arrows control the rest
        case MODULE_KEY_PRESS_LEFT:
        case MODULE_KEY_PRESS_RIGHT:        
            switch(arkanoid_state) {
                case ARK_INRO:
                    arkanoid_next_state = ARK_LOGO;
                    break;
                case ARK_LOGO:
                    arkanoid_next_state = ARK_NEW_GAME;
                    break;
                case ARK_NEW_GAME:
                    arkanoid_next_state = ARK_PLAY;
                    break;
                case ARK_PLAY:
                    /* nothing to do, just keep playing */
                    break;
            }
            break;
    }
    
    return 0;
    
}

MODULE_INFO_START()
    MODULE_INIT(arkanoid_init)
    MODULE_DEINIT(arkanoid_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_KEYPRESS, arkanoid_keypress, 0)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(level)
    MODULE_CONFIG(sound)
MODULE_CONFIGS_END()
