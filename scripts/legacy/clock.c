// Large clock

// This demo shows how to use double buffering for flicker-free graphics.
// And, of course, date/time functions.

console_hide();
if (!lv) set_gui_mode(1); // not in LiveView? go to PLAY mode
sleep(1);

void thick_line_polar(int x, int y, int radius, float angle, int thickness, int color)
{
    float s = sin(angle * M_PI / 180);
    float c = cos(angle * M_PI / 180);
    int i;
    for (i = -thickness/2; i <= thickness/2; i++)
    {
        draw_line_polar(x + s*i, y - c*i, radius, angle, color);
        // oversample to remove moire :)
        draw_line_polar(x + s*i + 1, y - c*i, radius, angle, color);
        draw_line_polar(x + s*i, y - c*i + 1, radius, angle, color);
    }
}

void draw_clock_face()
{
    fill_circle(360, 240, 230, COLOR_WHITE);
    fill_circle(360, 240, 228, COLOR_BLACK);
    
    for (int angle = 6; angle <= 360; angle += 6)
    {
        float s = sin(angle * M_PI / 180);
        float c = cos(angle * M_PI / 180);
        int x = 360 + 200 * s;
        int y = 240 - 200 * c;
        
        if (angle % 30 == 0)
            bmp_printf_center(SHADOW_FONT(FONT_LARGE), x+2, y, "%d", angle/30);
        else
            fill_circle(x, y, 1, COLOR_GRAY(50));
    }
}

void draw_clock_hands(struct tm * t)
{
    int ang_h = -90 + t->hour   * 360.0 / 12 + t->minute / 2 ;
    int ang_m = -90 + t->minute * 360.0 / 60 + t->second / 10;
    int ang_s = -90 + t->second * 360.0 / 60;

    thick_line_polar(360, 240, 120, ang_h, 10, COLOR_WHITE);
    thick_line_polar(360, 240, 170, ang_m,  6, COLOR_WHITE);
    thick_line_polar(360, 240, 175, ang_s,  2, COLOR_RED);
    
    fill_circle(360, 240, 10, COLOR_WHITE);
    draw_circle(360, 240, 11, COLOR_BLACK);
}

// disable GlobalDraw so it doesn't interfere with our graphics
int old_gdr = menu_get("Overlay", "Global Draw");
menu_set("Overlay", "Global Draw", 0);

// show the clock until we open some menu
while (get_gui_mode() <= 1)
{
    struct tm * t = get_time();
    
    double_buffering_start();
    bmp_printf(FONT_LARGE, 520, 420, "%02d/%02d/%02d\n", t->year, t->month, t->day);
    draw_clock_face();
    draw_clock_hands(t);
    double_buffering_end();
    
    int s = t->second;
    while (get_time()->second == s)
        sleep(0.1);
}

// restore Global Draw
menu_set("Overlay", "Global Draw", old_gdr);
