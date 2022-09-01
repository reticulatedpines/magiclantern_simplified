#include <stdio.h>
#include <dryos.h>
#include <module.h>
#include <menu.h>
#include <config.h>
#include <bmp.h>
#include <console.h>
#include <math.h>

uint32_t pt_colors[] =
{
    COLOR_WHITE, /* Background color */
    COLOR_RED,
    COLOR_GREEN1,
    COLOR_ORANGE,
    COLOR_DARK_RED,
    COLOR_MAGENTA,
};

// bmp manually drawing seems very slow
#define PT_CUSTOM_DRAW_BLOCK
void pt_draw_block(int bx, int by, uint32_t col)
{
    bmp_fill(pt_colors[col], 30 + bx * 20, 30 + by * 20, 20, 20);
}

#include "ptetris.h"

int running = 0;

extern int menu_redraw_blocked;

void tetris_task()
{
    int sleeploop = 0;

    pt_reset();
    running = 1;
    TASK_LOOP
    {
        clrscr();

        while (!running)
        {
            msleep(1);
        }

        pt_render();

        bmp_printf(FONT_LARGE, 300, 10, "Score: %d", pt.score);
        bmp_printf(FONT_LARGE, 300, 70, "Press [Q] to Quit");

        msleep(200);

        if (pt_step())
        {
            bmp_printf(FONT_LARGE, 300, 70 + 60, "Game Over");
            goto quit;
        }
    }

quit:
    menu_redraw_blocked = 0;
    clrscr();
    running = 0;
}

unsigned int tetris_keypress(unsigned int key)
{
    if (!running)
    {
        return 1;
    }

    /* Mess with "RNG" */
    pt_rand_x += key;

    /* Tell tetris_task to stop loop when
    receiving button input */
    running = 0;

    switch(key)
    {
    case MODULE_KEY_Q:
        clrscr();
        menu_redraw_blocked = 0;
        return 0;
    case MODULE_KEY_PRESS_LEFT:
        pt_handle_input(PT_LEFT);
        break;
    case MODULE_KEY_PRESS_RIGHT:
        pt_handle_input(PT_RIGHT);
        break;
    case MODULE_KEY_PRESS_UP:
        pt_handle_input(PT_ROT);
        break;
    case MODULE_KEY_PRESS_DOWN:
        pt_handle_input(PT_DOWN);
        break;
    }

    pt_render();

    running = 1;
    return 0;
}

static MENU_SELECT_FUNC(tetris_start)
{
    menu_redraw_blocked = 1;
    task_create("tetris_task", 0x1e, 0x4000, tetris_task, (void*)0);
}

struct menu_entry tetris_menu[] =
{
    {
        .name = "ML Tetris",
        .select = tetris_start,
        .help = "Tetris on your DSLR",
    },
};

unsigned int tetris_init()
{
    menu_add("Games", tetris_menu, COUNT(tetris_menu));
    return 0;
}

unsigned int tetris_deinit()
{
    return 0;
}

MODULE_CBRS_START()
MODULE_CBR(CBR_KEYPRESS, tetris_keypress, 0)
MODULE_CBRS_END()

MODULE_INFO_START()
MODULE_INIT(tetris_init)
MODULE_DEINIT(tetris_deinit)
MODULE_INFO_END()

