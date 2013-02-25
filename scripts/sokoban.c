/*
@title Sokoban Game
* 
* This is a playable demo for PicoC scripting.
* 
* It should help you understand:
* - Basic key processing (get_key)
* - Simple graphics (draw_rect, fill_rect, draw_circle, put_pixel etc)
* - PicoC programming basics
* 
* Enjoy!
* 
*/

#define LINES 9
#define COLUMNS 8

// http://www.sokobano.de/wiki/index.php?title=Sok_format

#define WALL '#'
#define SPACE ' '
#define PLAYER '@'
#define BOX '$'

#define TARGET '.'
#define PLAYER_ON_TARGET '+'
#define BOX_ON_TARGET '*'

char* maze[LINES] = {
    "  ##### ",
    "###   # ",
    "#.@$  # ",
    "### $.# ",
    "#.##$ # ",
    "# # . ##",
    "#$ *$$.#",
    "#   .  #",
    "########"
};

char targets[LINES][COLUMNS];

void print_maze()
{
    for (int i = 0; i < LINES; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            printf("%c%c", maze[i][j], targets[i][j] ? '*' : ' ');
        }
        printf("\n");
    }
}

void draw_wall(int x, int y, int a)
{
    fill_rect(x, y, a, a, COLOR_BLACK);
}

void draw_space(int x, int y, int a, int on_target)
{
    fill_rect(x, y, a, a, COLOR_GRAY(80));
    if (on_target)
        fill_circle(x + a/2, y + a/2, a/4, COLOR_ORANGE);
}

void draw_box(int x, int y, int a, int on_target)
{
    int pad = 3;

    for (int i = 0; i < pad; i++)
        draw_rect(x + i, y + i, a - i*2, a - i*2, COLOR_GRAY(80));

    int color = on_target ? COLOR_ORANGE : COLOR_YELLOW;
    fill_rect(x + pad, y + pad, a - pad*2, a - pad*2, color);
    draw_rect(x + pad, y + pad, a - pad*2, a - pad*2, COLOR_BLACK);
    pad++;
    draw_rect(x + pad, y + pad, a - pad*2, a - pad*2, COLOR_BLACK);

    draw_line(x + pad, y + pad, x + a - pad, y + a - pad, COLOR_BLACK);
    draw_line(x + pad - 1, y + pad, x + a - pad - 1, y + a - pad, COLOR_BLACK);
    
    draw_line(x + pad, y + a - pad, x + a - pad, y + pad, COLOR_BLACK);
    draw_line(x + pad - 1, y + a - pad, x + a - pad - 1, y + pad, COLOR_BLACK);
}

void draw_player(int x, int y, int a, int on_target)
{
    draw_space(x, y, a, 0);
    
    int xc = x + a/2;
    int yc = y + a/2;
    int r = a * 3/8;
    int y_eye = yc - r * 3/8;
    int y_mouth = yc + r * 3/8;
    fill_circle(xc, yc, r, COLOR_YELLOW);
    draw_circle(xc, yc, r, COLOR_BLACK);
    draw_circle(xc, yc, r-1, COLOR_BLACK);
    fill_circle(xc - r/3, y_eye, r/5, COLOR_BLACK);
    fill_circle(xc + r/3, y_eye, r/5, COLOR_BLACK);
    
    int r_mouth = r*2/3;
    for (int t = 45; t <= 180-45; t += 2)
    {
        int xp = xc + r_mouth * cos(t*M_PI/180);
        int yp = yc - r/8 + r_mouth * sin(t*M_PI/180);
        put_pixel(xp, yp, COLOR_BLACK);
        put_pixel(xp, yp+1, COLOR_BLACK);
    }
}

void draw_maze()
{
    int a = MIN(400 / LINES, 700 / COLUMNS);
    int total_w = a * COLUMNS;
    int total_h = a * LINES;
    int x0 = 360 - total_w / 2;
    int y0 = 240 - total_h / 2;
    
    for (int i = 0; i < LINES; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            // top-left corner of current cell
            int x = x0 + j * a;
            int y = y0 + i * a;
            
            switch (maze[i][j])
            {
                case BOX:
                    draw_box(x, y, a, targets[i][j]);
                    break;
                case SPACE:
                    draw_space(x, y, a, targets[i][j]);
                    break;
                case WALL:
                    draw_wall(x, y, a);
                    break;
                case PLAYER:
                    draw_player(x, y, a, targets[i][j]);
                    break;
            }
            
        }
    }
    
    for (i = 1; i < 5; i++)
        draw_rect(x0-i, y0-i, total_w+2*i, total_h+2*i, COLOR_WHITE);
}

void victory()
{
    beep();
    draw_maze();
    bmp_printf(FONT_LARGE, 300, 200, "YOU WIN :)");
}

void split_target()
{
    for (int i = 0; i < LINES; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            if (maze[i][j] == TARGET)
            {
                targets[i][j] = 1;
                maze[i][j] = SPACE;
            }
            else if (maze[i][j] == BOX_ON_TARGET)
            {
                targets[i][j] = 1;
                maze[i][j] = BOX;
            }
            else if (maze[i][j] == PLAYER_ON_TARGET)
            {
                targets[i][j] = 1;
                maze[i][j] = PLAYER;
            }
            else targets[i][j] = 0;
        }
    }
}

void get_current_pos(int* l, int* c)
{
    for (int i = 0; i < LINES; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            if (maze[i][j] == PLAYER)
            {
                *l = i; 
                *c = j;
                return;
            }
        }
    }
    // error
    *l = *c = -1;
}

void move(int dl, int dc)
{
    int l, c;
    get_current_pos(&l, &c);
    
    if (maze[l][c] != PLAYER) { printf("whoops"); return; }
    
    switch (maze[l+dl][c+dc]) // what's in the destination box?
    {
        case SPACE: // simple, just move
        {
            maze[l][c] = SPACE;
            maze[l+dl][c+dc] = PLAYER;
            break;
        }
        
        case BOX: // move only if we can push the box in a free space
        {
            if (maze[l+dl*2][c+dc*2] == SPACE)
            {
                maze[l+dl*2][c+dc*2] = BOX;
                maze[l+dl][c+dc] = PLAYER;
                maze[l][c] = SPACE;
            }
        }
    }
}

int check_solution()
{
    for (int i = 0; i < LINES; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            if ((targets[i][j]) && (maze[i][j] != BOX))
                return 0; // at least one box not on target position
        }
    }
    return 1; // looks alright
}

void main()
{
    console_hide();
    click(PLAY);
    sleep(1);
    
    fill_rect(0, 0, 720, 480, COLOR_EMPTY);
    set_canon_gui(0);
    
    split_target();
    
    while(!check_solution())
    {
        draw_maze();
        // print_maze();
        int key = get_key();
        switch(key)
        {
            case LEFT:
            case WHEEL_LEFT:
                move(0, -1);
                break;
            case RIGHT:
            case WHEEL_RIGHT:
                move(0, 1);
                break;
            case UP:
            case WHEEL_UP:
                move(-1, 0);
                break;
            case DOWN:
            case WHEEL_DOWN:
                move(1, 0);
                break;
            case SET:
                break;
            default:
                printf("Exiting...\n");
                set_canon_gui(1);
                return;
        }
    }
    
    printf("You win!\n");
    victory();
    
    sleep(5);
    set_canon_gui(1);
}

main();
