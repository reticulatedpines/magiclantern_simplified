// Image analysis demo

void spotmeters()
{
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            int x = 72*2 * j + 72;
            int y = 48*2 * i + 48;
            struct yuv * yuv_pixel = get_spot_yuv(x, y, 20);
            bmp_printf(SHADOW_FONT(FONT_SMALL), x-50, y+10, "YUV %3d,%3d,%3d", yuv_pixel->Y, yuv_pixel->U, yuv_pixel->V);

            struct rgb * rgb_pixel = get_spot_rgb(x, y, 20);
            bmp_printf(SHADOW_FONT(FONT_SMALL), x-50, y+20, "RGB %3d,%3d,%3d", rgb_pixel->R, rgb_pixel->G, rgb_pixel->B);
        }
    }
}

console_hide();
click(PLAY);
sleep(1);

spotmeters();

wait_key();
clrscr();
