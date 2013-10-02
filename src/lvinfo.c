#include <dryos.h>
#include <property.h>
#include <menu.h>
#include <bmp.h>
#include <lvinfo.h>

#define MAX_ITEMS 64
#define MIN_SPACING 24
#define TOTAL_WIDTH 720

/* all registered info items go here */
/* note: these are somewhat private; they get first sorted in top/bottom bars,
 * and most of the code works at bar level, without accessing _info_items directly */
static struct lvinfo_item * _info_items[MAX_ITEMS];
static int _info_items_count = 0;
static int layout_dirty = 0;
static struct semaphore * lvinfo_sem = 0;

static int default_font = FONT_MED_LARGE;   /* used in normal situations */
static int small_font = FONT_MED;           /* used if the layout gets really tight */

void lvinfo_add_items(struct lvinfo_item * items, int count)
{
    if (lvinfo_sem) take_semaphore(lvinfo_sem, 0);
    for (int i = 0; i < count && _info_items_count < MAX_ITEMS; i++)
    {
        _info_items[_info_items_count] = &items[i];
        _info_items_count++;
    }
    layout_dirty = 1;
    if (lvinfo_sem) give_semaphore(lvinfo_sem);
}

void lvinfo_add_item(struct lvinfo_item * item)
{
    lvinfo_add_items(item, 1);
}

static int is_active(struct lvinfo_item * item)
{
    return item->width && !item->hidden && !item->disabled;
}

/* call the update functions and compute the metrics */
static void lvinfo_update_items(struct lvinfo_item * items[], int count, int override_font)
{
    /* everybody measure themselves! */
    for (int i = 0; i < count; i++)
    {
        items[i]->width = 0;
        items[i]->height = 0;
        items[i]->hidden = 0;
        
        if (override_font) items[i]->fontspec = override_font;
        int fnt = items[i]->fontspec;
        items[i]->color_fg = FONT_FG(fnt);
        items[i]->color_bg = FONT_BG(fnt);

        if (items[i]->update)
        {
            items[i]->update(items[i], 0);
        }

        /* no width/height specified? use defaults */
        if (!items[i]->width && items[i]->value)
        {
            items[i]->width = bmp_string_width(fnt, items[i]->value);
        }
        if (!items[i]->height)
        {
            items[i]->height = fontspec_font(fnt)->height;
        }
    }
}

static int lvinfo_check_if_needs_reflow(struct lvinfo_item * items[], int count, int bar_x, int bar_width)
{
    int too_tight = 0;
    int max_spacing = INT_MIN;
    int min_spacing = INT_MAX;
    int prev_right = bar_x;

    for (int i = 0; i <= count; i++)
    {
        /* how far we are from the previous one? */
        if (i == count || is_active(items[i]))
        {
            int now_left = (i < count) ?
                items[i]->x - items[i]->width/2 :   /* normal case: spacing between items */
                bar_x + bar_width ;                 /* special case: after last item */
            
            int spacing = now_left - prev_right;
            if (i == 0 || i == count)               /* spacing at the borders is normally half of spacing between items */
                spacing *= 2;                       /* multiply by 2 so we can compare these things */
            
            min_spacing = MIN(min_spacing, spacing);
            max_spacing = MAX(max_spacing, spacing);

            /* for debugging */
            //~ bmp_fill(i == 0 || i == count ? COLOR_BLUE : COLOR_RED, prev_right, 100, now_left - prev_right, 2);
            
            if (spacing < MIN_SPACING * 2/3)
            {
                too_tight = 1;
            }
            prev_right = items[i]->x + items[i]->width/2;
        }
    }
    
    int imbalanced = (max_spacing - min_spacing > MIN_SPACING*2);
    return too_tight || imbalanced;
}


/* assign some items from the global list to top/bottom bars */
static void lvinfo_distribute_items(int which_bar, struct lvinfo_item * items[], int* count, int* space)
{
    for (int i = 0; i < _info_items_count; i++)
    {
        if (!_info_items[i]->placed && (which_bar == -1 || _info_items[i]->which_bar == which_bar))
        {
            *space -= _info_items[i]->width + (is_active(_info_items[i]) ? MIN_SPACING : 0);

            if (*space > 0)
            {
                items[(*count)++] = _info_items[i];
                _info_items[i]->placed = 1;
            }
        }
    }
}

static void lvinfo_mark_all_as_not_placed()
{
    for (int i = 0; i < _info_items_count; i++)
    {
        _info_items[i]->placed = 0;
    }
}

/* how many items are still left to be placed? */
static int lvinfo_remaining_items()
{
    int count = 0;
    for (int i = 0; i < _info_items_count; i++)
    {
        if (!_info_items[i]->placed)
        {
            count++;
        }
    }
    return count;
}

/* distribute spacing evenly between items */
static void lvinfo_justify_items(struct lvinfo_item * items[], int count, int total_width)
{
    int used_items = 0;
    int used_width = 0;
    for (int i = 0; i < count; i++)
    {
        int active = is_active(items[i]);
        used_items += active ? 1 : 0;
        used_width += active ? items[i]->width : 0;
    }
    
    /* how much we can stretch? */
    int extra_spacing = total_width - used_width;

    /* todo: use Bresenham algorithm to get rid of these floats */
    float spacing_per_item = (float) extra_spacing / used_items;
    float x = spacing_per_item / 2;
    
    for (int i = 0; i < count; i++)
    {
        items[i]->x = x + items[i]->width/2;

        if (is_active(items[i]))
        {
            x += items[i]->width + spacing_per_item;
        }
    }
}

/* heuristic that tells whether we should try to enlarge the font */
static int lvinfo_should_enlarge(struct lvinfo_item * items[], int count, int total_width)
{
    int used_items = 0;
    int used_width = 0;
    for (int i = 0; i < count; i++)
    {
        int active = is_active(items[i]);
        used_items += active ? 1 : 0;
        used_width += active ? items[i]->width : 0;
    }
    
    int extra_spacing = total_width - used_width;
    int spacing_per_item = extra_spacing / used_items;
        
    return spacing_per_item > MIN_SPACING * 5/4;
}

/* shrink some items or hide low-priority ones */
/* returns: INT_MIN if nothing was discarded, INT_MAX if error, otherwise it returns the priority of last item discarded */
static int lvinfo_squeeze_space(struct lvinfo_item * items[], int count, int total_width)
{
    int used_items = 0;
    int used_width = 0;
    for (int i = 0; i < count; i++)
    {
        int active = is_active(items[i]);
        used_items += active ? 1 : 0;
        used_width += active ? items[i]->width + MIN_SPACING : 0;
    }
    
    /* how much we can stretch? */
    int spacing_needed = used_width - total_width;
    
    /* any real need to squeeze space? */
    if (spacing_needed <= MIN_SPACING)
        return INT_MIN;

    /* sort by priority (lowest first) */
    struct lvinfo_item * prio_items[MAX_ITEMS];
    memcpy(prio_items, items, sizeof(prio_items));
    for (int i = 0; i < count-1; i++)
    {
        for (int j = i+1; j < count; j++)
        {
            if (prio_items[i]->priority > prio_items[j]->priority)
            {
                struct lvinfo_item * aux = prio_items[i];
                prio_items[i] = prio_items[j];
                prio_items[j] = aux;
            }
        }
    }

    /* sort by width, largest first */
    struct lvinfo_item * big_items[MAX_ITEMS];
    memcpy(big_items, items, sizeof(big_items));
    for (int i = 0; i < count-1; i++)
    {
        for (int j = i+1; j < count; j++)
        {
            if (big_items[i]->width < big_items[j]->width)
            {
                struct lvinfo_item * aux = big_items[i];
                big_items[i] = big_items[j];
                big_items[j] = aux;
            }
        }
    }

    /* shrink items, starting with the largest ones */
    /* if we have to shrink 3 or more items, shrink them all */
    int shrunk = 0;
    for (int i = 0; i < count-1; i++)
    {
        if (is_active(big_items[i]))
        {
            int old_width = big_items[i]->width;
            lvinfo_update_items(&big_items[i], 1, small_font);
            int new_width = big_items[i]->width;
            spacing_needed -= (old_width - new_width);
            shrunk++;
            if (spacing_needed <= MIN_SPACING && shrunk < 3)
            {
                /* succeeded by shrinking 1 or 2 items? */
                return INT_MIN;
            }
        }
    }

    if (spacing_needed <= MIN_SPACING)
    {
        return INT_MIN;
    }

    /* discard all items until there's enough space; lower priority discarded first */
    for (int i = 0; i < count-1; i++)
    {
        if (is_active(prio_items[i]))
        {
            prio_items[i]->hidden = 1;
            spacing_needed -= (prio_items[i]->width + MIN_SPACING);
            if (spacing_needed <= MIN_SPACING)
            {
                return prio_items[i]->priority;
            }
        }
    }
    /* should be unreachable */
    return INT_MAX;
}

static void lvinfo_valign_items(struct lvinfo_item * items[], int count, int bar_y, int bar_height)
{
    for (int i = 0; i < count; i++)
    {
        items[i]->y = bar_y + (bar_height - items[i]->height) / 2 + 2;
    }
}

static void lvinfo_sort_by_position(struct lvinfo_item * items[], int count)
{
    /* sort by preferred position */
    /* we need to use a stable sorting algorithm, so items with the same preferred position will not get swapped */
    int done = 0;
    while (!done)
    {
        done = 1;
        for (int i = 0; i < count-1; i++)
        {
            if (items[i]->preferred_position > items[i+1]->preferred_position)
            {
                struct lvinfo_item * aux = items[i];
                items[i] = items[i+1];
                items[i+1] = aux;
                done = 0;
            }
        }
    }
}

/* top/bottom bar */
static struct lvinfo_item * top_items[MAX_ITEMS];
static struct lvinfo_item * bot_items[MAX_ITEMS];
static int top_count = 0;
static int bot_count = 0;

static void lvinfo_refresh_layout()
{
    /* try 3 layouts:
     * normal (large font),
     * tight if there are still items that didn't fit,
     * and really tight, which attempts to squeeze everything and leave it up to the display routine to sort it out
     **/
    for (int tight = 0; tight <= 2; tight++)
    {
        /* reset the "placed" flag so we can rebuild the layout from scratch */
        lvinfo_mark_all_as_not_placed();
        
        /* reset top/bottom bars */
        top_count = bot_count = 0;
        
        /* distribute stuff to top/bottom bars */
        lvinfo_update_items(_info_items, _info_items_count, tight ? small_font : default_font);
        
        int top_space = tight == 2 ? TOTAL_WIDTH * 10 : TOTAL_WIDTH;
        int bot_space = top_space;
        
        /* first, move the items that can't be placed elsewhere */
        lvinfo_distribute_items(LV_TOP_BAR_ONLY,        top_items, &top_count, &top_space);
        lvinfo_distribute_items(LV_BOTTOM_BAR_ONLY,     bot_items, &bot_count, &bot_space);
        
        /* next, try to follow the preferences */
        lvinfo_distribute_items(LV_PREFER_TOP_BAR,      top_items, &top_count, &top_space);
        lvinfo_distribute_items(LV_PREFER_BOTTOM_BAR,   bot_items, &bot_count, &bot_space);
        
        /* still some items that couldn't fit according to preferences? move to the other bar */
        lvinfo_distribute_items(LV_PREFER_TOP_BAR,      bot_items, &bot_count, &bot_space);
        lvinfo_distribute_items(LV_PREFER_BOTTOM_BAR,   top_items, &top_count, &top_space);

        /* fill the remaining space with items that don't care where they are placed */
        lvinfo_distribute_items(LV_WHEREVER_IT_FITS,    top_items, &top_count, &top_space);
        lvinfo_distribute_items(LV_WHEREVER_IT_FITS,    bot_items, &bot_count, &bot_space);
        
        /* finished? hope so; otherwise, go back and try a tighter layout */
        if (lvinfo_remaining_items() == 0)
        {
            break;
        }
    }
    
    /* sort items */
    lvinfo_sort_by_position(top_items, top_count);
    lvinfo_sort_by_position(bot_items, bot_count);
    
    /* distribute spacing evenly between items */
    lvinfo_justify_items(top_items, top_count, TOTAL_WIDTH);
    lvinfo_justify_items(bot_items, bot_count, TOTAL_WIDTH);
}

static void lvinfo_display_bar(struct lvinfo_item * items[], int count, int bar_x, int bar_y, int bar_width, int bar_height)
{
    int default_bg = FONT_BG(default_font);
    int default_bg_out = (default_bg == COLOR_BG_DARK ? 0 : default_bg);
    
    int prev_right = bar_x;
    int prev_bg = default_bg_out;
    for (int i = 0; i < count; i++)
    {
        /* don't process empty items */
        if (!is_active(items[i]))
            continue;
        
        /* position */
        int x = items[i]->x;
        int w = items[i]->width;
        int now_left = x - w/2;
        int now_right = x + w/2;
        int y = items[i]->y;
        int y0 = bar_y;
        int x0 = now_left;

        /* range checking */
        if (now_left < bar_x)
            continue;
        if (now_right > bar_x + bar_width)
            continue;

        /* font */
        int fnt = items[i]->fontspec;
        
        /* override colors */
        fnt = FONT(fnt, items[i]->color_fg, items[i]->color_bg);
        
        int bg = FONT_BG(fnt);

        /* fill the gap between this item and previous one */
        /* the Voronoi cell associated with each item will get filled by the same background color */
        if (prev_right >= 0 && now_left > prev_right)
        {
            int gap = now_left - prev_right + 1;
            bmp_fill(prev_bg, prev_right, y0, gap/2, bar_height);
            bmp_fill(bg, prev_right+gap/2, y0, gap/2, bar_height);
        }

        /* clear the space for current box */
        bmp_fill(bg, x0, y0, w, bar_height);
        
        /* for debugging: show the center of each item */
        //~ bmp_fill(COLOR_RED, x-1, y0-2, 2, 2);

        if (items[i]->custom_drawing)
        {
            /* anybody asked for custom drawing? */
            if (items[i]->update)
            {
                items[i]->update(items[i], 1);
            }
        }
        else
        {
            /* no custom draw? use our default print routine */
            bmp_printf(fnt, x, y, items[i]->value);
        }
        prev_right = x + w/2;
        prev_bg = bg;
    }

    /* fill the remaining space till the far right */
    if (count > 0)
    {
        int now_left = TOTAL_WIDTH;
        int gap = now_left - prev_right;
        bmp_fill(prev_bg, prev_right, bar_y, gap / 2, bar_height);
        bmp_fill(default_bg_out, prev_right + gap / 2, bar_y, gap / 2, bar_height);
    }
}

static void lvinfo_align_and_display(struct lvinfo_item * items[], int count, int bar_x, int bar_y, int bar_width, int bar_height)
{
    /* choose a default font */
    /* try to borrow the color from the cropmarks; if it's fully transparent, use transparent gray */
    int bg = (items == top_items) ? TOPBAR_BGCOLOR : BOTTOMBAR_BGCOLOR;
    if (bg == 0) bg = COLOR_BG_DARK;
    default_font = FONT(FONT_MED_LARGE, COLOR_WHITE, bg) | FONT_ALIGN_CENTER;
    small_font = FONT(FONT_MED, COLOR_WHITE, bg) | FONT_ALIGN_CENTER;
    
    int font_changed = 0;

    for (int i = 0; i < count; i++)
    {
        /* colors changed? reset the font to large and refresh the layout */
        /* this will also update the text and dimensions for all items */
        int prev_fnt = items[i]->fontspec;
        int colors_changed = (prev_fnt & 0xFFFF) != (default_font & 0xFFFF);
        if (colors_changed) font_changed++;
        lvinfo_update_items(&items[i], 1, colors_changed ? default_font : 0);
    }

    /* should we try to display everything in large font? */
    /* if it doesn't look bad, keep the previous layout */
    int should_enlarge = lvinfo_should_enlarge(items, count, bar_width);

    if (should_enlarge)
    {
        for (int i = 0; i < count; i++)
        {
            /* check each item; if it was small and now it should be enlarged, update the font */
            int prev_fnt = items[i]->fontspec;
            int font_should_change = (prev_fnt & FONT_MASK) != (default_font & FONT_MASK);
            if (font_should_change)
            {
                font_changed++;
                lvinfo_update_items(&items[i], 1, default_font);
            }
        }
    }
    
    int needs_reflow = font_changed || lvinfo_check_if_needs_reflow(items, count, bar_x, bar_width);
    if (needs_reflow)
    {
        /* some items got too tight? try to re-distribute the spacing between them */
        lvinfo_justify_items(items, count, bar_width);

        /* things got really tight */
        int still_needs_reflow = lvinfo_check_if_needs_reflow(items, count, bar_x, bar_width);
        if (still_needs_reflow)
        {
            int severity = lvinfo_squeeze_space(items, count, bar_width);
            lvinfo_justify_items(items, count, bar_width);
            if (severity >= 0)
            {
                /* important items were disabled */
                /* it may be better if we try to rebuild the layout from scratch */
                layout_dirty = 1;
            }
        }
    }

    /* center items vertically */
    lvinfo_valign_items(items, count, bar_y, bar_height);

    /* and... finally, display them! */
    lvinfo_display_bar(items, count, bar_x, bar_y, bar_width, bar_height);
}

void lvinfo_display(int top, int bottom)
{
    take_semaphore(lvinfo_sem, 0);

    static int refresh_timer = INT_MIN;
    if (layout_dirty && should_run_polling_action(2000, &refresh_timer))
    {
        console_printf("LVINFO: refresh layout\n");
        lvinfo_refresh_layout();
        layout_dirty = 0;
    }
    
    if (top)
    {
        lvinfo_align_and_display(top_items, top_count, 0, get_ml_topbar_pos(), TOTAL_WIDTH, 32);
    }
    
    if (bottom)
    {
        lvinfo_align_and_display(bot_items, bot_count, 0, get_ml_bottombar_pos(), TOTAL_WIDTH, 32);
    }
    
    give_semaphore(lvinfo_sem);
}

static void lvinfo_init()
{
    lvinfo_sem = create_named_semaphore("lvinfo_sem", 1);
}

INIT_FUNC("lvinfo", lvinfo_init);

