
#define __plot_c__

#include <module.h>
#include <dryos.h>
#include <bmp.h>
#include <math.h>
#include <float.h>
#include <string.h>

#include "plot.h"

/* reset the number of entries in a data collection. does not free any memory */
void plot_clear(plot_coll_t *coll)
{
    if(!coll)
    {
        return;
    }
    coll->used = 0;
}

/* create a new graph at given position */
plot_graph_t *plot_alloc_graph(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    plot_graph_t *graph = malloc(sizeof(plot_graph_t));
    
    if(!graph)
    {
        return NULL;
    }

    memset(graph, 0x00, sizeof(plot_graph_t));

    graph->x = x;
    graph->y = y;
    graph->w = w;
    graph->h = h;

    /* default config */
    graph->type = PLOT_LINEAR;
    graph->x_field = 0;
    graph->y_field = 1;
    graph->dynamic_x = 0;

    graph->dot_size = 2;
    graph->points_min = 100;
    graph->points_drawn = 0;
    graph->color_dots = COLOR_RED;
    graph->color_lines = COLOR_YELLOW;
    graph->color_border = COLOR_WHITE;
    graph->color_axis = COLOR_WHITE;
    graph->color_range = COLOR_GRAY(60);
    graph->color_bg = COLOR_GRAY(20);
    return graph;
}

/* allocate an empty structure for data to plot. */
plot_coll_t *plot_alloc_data(uint32_t fields)
{
    plot_coll_t *coll = malloc(sizeof(plot_coll_t));

    if(!coll)
    {
        return NULL;
    }
    
    memset(coll, 0x00, sizeof(plot_coll_t));

    if(fields)
    {
        coll->fields = fields;
    }
    else
    {
        coll->fields = 1;
    }

    coll->entry_size = sizeof(plot_data_t) * coll->fields;

    coll->extent = 1024;
    coll->entries = malloc(coll->extent * coll->entry_size);
    coll->allocated = coll->extent;
    coll->used = 0;

    return coll;
}

/* add one data "entry" consisting of "fields" whose amount were specified when calling plot_alloc_data() */
uint32_t plot_add(plot_coll_t *coll, ...)
{
    va_list argp;
    va_start(argp, coll);
    
    /* can we be defensive here when (coll == NULL)? 
       returning without going through all va_args will it render stack corrupted?
       to be checked!
    */
    if(!coll)
    {
        va_end(argp);
        return 0;
    }
    

    if(coll->used + coll->entry_size > coll->allocated)
    {
        uint32_t new_allocated = coll->allocated + coll->extent;
        uint32_t new_size = new_allocated * coll->entry_size;

        void *new_data = realloc(coll->entries, new_size);

        if(!new_data)
        {
            return -1;
        }

        coll->entries = new_data;
        coll->allocated = new_allocated;
    }

    for(uint32_t field = 0; field < coll->fields; field++)
    {
        /* when passing floats via vararg, they get promoted to double */
        plot_data_t value = va_arg(argp, double);
        coll->entries[coll->used * coll->fields + field] = value;
    }

    coll->used++;

    va_end(argp);

    return coll->used;
}

/* add many data "entry" consisting of "fields" whose amount were specified when calling plot_alloc_data() */
uint32_t plot_add_array(plot_coll_t *coll, uint32_t entry_count, plot_data_t *entries)
{
    if(!coll || !entries)
    {
        return 0;
    }
    
    if(coll->used + entry_count > coll->allocated)
    {
        uint32_t extent = (entry_count / coll->extent + 1) * coll->extent;
        uint32_t new_allocated = coll->allocated + extent;
        uint32_t new_size = new_allocated * coll->entry_size;

        void *new_data = realloc(coll->entries, new_size);

        if(!new_data)
        {
            return -1;
        }

        coll->entries = new_data;
        coll->allocated = new_allocated;
    }

    for(uint32_t entry = 0; entry < entry_count; entry++)
    {
        for(uint32_t field = 0; field < coll->fields; field++)
        {
            plot_data_t value = entries[entry * coll->entry_size + field];
            coll->entries[coll->used * coll->fields + field] = value;
        }
        coll->used++;
    }

    return coll->used;
}


/* internal: return the given field and pre-scale it */
static plot_data_t plot_get_scaled(plot_coll_t *coll, plot_graph_t *plot, uint32_t entry, uint32_t get_y_field)
{
    /* get first field's address in current entry */
    uint32_t src_field = 0;
    uint32_t max_value = 0;
    plot_win_t win;

    if(!coll || !plot)
    {
        return 0;
    }
    
    if(get_y_field)
    {
        if(plot->type == PLOT_XY)
        {
            src_field = plot->y_field;
        }
        else
        {
            src_field = 0;
        }
        max_value = plot->h;
        win = plot->y_win;
    }
    else
    {
        src_field = plot->x_field;
        max_value = plot->w;
        win = plot->x_win;
    }

    for(uint32_t field = 0; field < coll->fields; field++)
    {
        if(field == src_field)
        {
            /* the selected range in source data */
            uint32_t scale = 0;

            plot_data_t range = win.max - win.min;
            scale = (range > 0.00001) || (range < -0.00001);

            plot_data_t value = coll->entries[entry * coll->fields + field];
            if(scale)
            {
                value = ((value - win.min) * max_value) / range;
            }

            plot_data_t scaled = COERCE(value, 0, max_value);
            return scaled;
        }
    }

    return 0;
}

/* mark the graph for whole repainting */
void plot_graph_reset(plot_graph_t *plot)
{
    if(!plot)
    {
        return;
    }
    plot->points_drawn = 0;
    plot->points_last = 0;
}

void plot_fmt_float(char *buf, uint32_t buf_len, float value)
{
    int32_t left = (int32_t)fabs(value);
    int32_t right = ((int32_t)(fabs(value) * 1000.0f)) % 1000;
    char *sign = " ";
    
    if(!buf)
    {
        return;
    }

    if(value < 0)
    {
        sign = "-";
    }

    snprintf(buf, buf_len, "%s%d.%03d", sign, left, right);
}

void plot_graph_paint_range(plot_coll_t *coll, plot_graph_t *plot)
{
    if(!coll || !plot)
    {
        return;
    }
    
    if(plot->color_range != PLOT_COLOR_NONE)
    {
        char line[64];
        char buf[32];

        strcpy(line, "x: ");
        plot_fmt_float(buf, sizeof(buf), plot->x_win.min);
        strcat(line, buf);
        strcat(line, " - ");
        plot_fmt_float(buf, sizeof(buf), plot->x_win.max);
        strcat(line, buf);
        bmp_printf(FONT(FONT_SMALL,plot->color_range,plot->color_bg), plot->x + 2, plot->y + 2, line);

        strcpy(line, "y: ");
        plot_fmt_float(buf, sizeof(buf), plot->y_win.min);
        strcat(line, buf);
        strcat(line, " - ");
        plot_fmt_float(buf, sizeof(buf), plot->y_win.max);
        strcat(line, buf);
        bmp_printf(FONT(FONT_SMALL,plot->color_range,plot->color_bg), plot->x + 2, plot->y + 2 + fontspec_height(FONT_SMALL), line);
    }
}

void plot_graph_paint_grid(plot_coll_t *coll, plot_graph_t *plot)
{
    if(!coll || !plot)
    {
        return;
    }
    
    /* should we paint the axis? */
    if(plot->color_axis != PLOT_COLOR_NONE)
    {
        plot_data_t x_range = plot->x_win.max - plot->x_win.min;
        plot_data_t y_range = plot->y_win.max - plot->y_win.min;

        /* only draw if the line would be in visible range */
        if(PLOT_NONZERO(x_range) && (plot->x_win.min <= 0 && plot->x_win.max > 0))
        {
            /* translate coordinates and add to display X start */
            plot_data_t base = plot->x + (-plot->x_win.min * plot->w) / x_range;
            draw_line(base, plot->y, base, plot->y + plot->h, plot->color_axis);
        }

        if(PLOT_NONZERO(y_range) && (plot->y_win.min <= 0 && plot->y_win.max > 0))
        {
            /* translate coordinates and add to display Y start (but count upwards, so subtract) */
            plot_data_t base = (plot->y + plot->h) - (-plot->y_win.min * plot->h) / y_range;
            draw_line(plot->x, base, plot->x + plot->w, base, plot->color_axis);
        }
    }
}

/* redraw parts of the graph if some data was added */
void plot_graph_update(plot_coll_t *coll, plot_graph_t *plot)
{
    uint32_t redraw = 0;
    uint32_t points = 0;
    
    if(!coll || !plot)
    {
        return;
    }

    if(plot->points_drawn > coll->used)
    {
        plot->points_drawn = 0;
    }

    if(plot->type == PLOT_LINEAR)
    {
        if(plot->dynamic_x)
        {
            points = MAX(plot->points_min, coll->used);
        }
        else
        {
            points = plot->points_min * (coll->used / plot->points_min + 1);
        }

        /* when the number of points to be drawn exceed the current scaling, change scaling and redraw */
        if(plot->points_last != points)
        {
            plot->points_drawn = 0;
            plot->points_last = points;
        }
    }

    /* when redrawing the whole plot, also clear bg and paint borders */
    if(!plot->points_drawn)
    {
        redraw = 1;
        
        if(plot->color_bg != PLOT_COLOR_NONE)
        {
            bmp_fill(plot->color_bg, plot->x, plot->y, plot->w, plot->h);
        }
        if(plot->color_border != PLOT_COLOR_NONE)
        {
            bmp_draw_rect_chamfer(plot->color_border, plot->x, plot->y, plot->w, plot->h, 0, 0);
        }
        
        plot_graph_paint_grid(coll, plot);
    }

    /* these are the base coordinates for that plot */
    uint32_t start_x = plot->x;
    uint32_t start_y = plot->y + plot->h;


    /* for every data entry in the collection that has not been drawn yet */
    for(uint32_t entry = plot->points_drawn; entry < coll->used; entry++)
    {
        uint32_t plot_x = 0;
        uint32_t plot_y = 0;

        /* a linear plot is incrementing X for every entry and taking Y from value collection */
        if(plot->type == PLOT_LINEAR)
        {
            plot_x = (entry * plot->w) / points;
        }
        else if(plot->type == PLOT_XY)
        {
            plot_x = plot_get_scaled(coll, plot, entry, 0);
        }

        /* now get y value */
        plot_y = plot_get_scaled(coll, plot, entry, 1);

        /* now position correctly */
        uint32_t x = start_x + plot_x;
        uint32_t y = start_y - plot_y;

        if(entry != 0 && plot->color_lines != PLOT_COLOR_NONE)
        {
            draw_line(plot->last_dot_x, plot->last_dot_y, x, y, plot->color_lines);
        }

        if(plot->dot_size > 1 && plot->color_dots != PLOT_COLOR_NONE)
        {
            uint32_t size = plot->dot_size;
            
            if( (x - plot->dot_size > plot->x) && (x + plot->dot_size < plot->x + plot->w) && 
                (y - plot->dot_size > plot->y) && (y + plot->dot_size < plot->y + plot->h))
            {
                fill_circle(x, y, size, plot->color_dots);
            }
        }

        plot->last_dot_x = x;
        plot->last_dot_y = y;
        plot->points_drawn++;
    }

    if(redraw)
    {
        plot_graph_paint_range(coll, plot);
    }
}

/* repaint the whole graph */
void plot_graph_draw(plot_coll_t *coll, plot_graph_t *plot)
{
    if(!coll || !plot)
    {
        return;
    }
    
    plot_graph_reset(plot);
    plot_graph_update(coll, plot);
}

/* set the ranges to be drawn */
void plot_set_range(plot_graph_t *plot, plot_data_t x_min, plot_data_t x_max, plot_data_t y_min, plot_data_t y_max)
{
    if(!plot)
    {
        return;
    }
    
    if( (plot->x_win.min != x_min) ||
        (plot->x_win.max != x_max) ||
        (plot->y_win.min != y_min) ||
        (plot->y_win.max != y_max))
    {
        plot->points_drawn = 0;
    }

    plot->x_win.min = x_min;
    plot->x_win.max = x_max;
    plot->y_win.min = y_min;
    plot->y_win.max = y_max;
}

/* autorange a plot based on the data collection passed */
void plot_autorange(plot_coll_t *coll, plot_graph_t *plot)
{
    plot_data_t x_min = PLOT_MAX;
    plot_data_t x_max = PLOT_MIN;
    plot_data_t y_min = PLOT_MAX;
    plot_data_t y_max = PLOT_MIN;
    uint32_t y_field = 0;
    
    if(!coll || !plot)
    {
        return;
    }

    if(plot->type == PLOT_LINEAR)
    {
        x_min = 0;
        x_max = plot->points_min * (coll->used / plot->points_min + 1);
        y_field = 0;
    }
    else
    {
        for(uint32_t entry = 0; entry < coll->used; entry++)
        {
            plot_data_t value = coll->entries[entry * coll->fields + plot->x_field];

            x_min = MIN(x_min, value);
            x_max = MAX(x_max, value);
        }
        y_field = plot->y_field;
    }

    /* both plot types share the same code for Y range */
    for(uint32_t entry = 0; entry < coll->used; entry++)
    {
        plot_data_t value = coll->entries[entry * coll->fields + y_field];

        y_min = MIN(y_min, value);
        y_max = MAX(y_max, value);
    }

    plot_set_range(plot, x_min, x_max, y_min, y_max);
}


static unsigned int plot_init()
{
#if DEMO_CODE
    uint32_t width = 700;
    uint32_t height = 220;

    /* allocate plot datasets */
    plot_coll_t *coll_1 = plot_alloc_data(1);
    plot_coll_t *coll_2 = plot_alloc_data(2);
    plot_coll_t *coll_3 = plot_alloc_data(2);
    
    /* allocate plot graphics */
    plot_graph_t *plot_1 = plot_alloc_graph(5, 5, width, height);
    plot_graph_t *plot_2 = plot_alloc_graph(5, 5 + height, width/2, height);
    plot_graph_t *plot_3 = plot_alloc_graph(5 + width/2, 5 + height, width/2, height);

    /* X/Y plot */
    plot_2->type = PLOT_XY;
    plot_3->type = PLOT_XY;

    /* selecet displayed range */
    plot_set_range(plot_1, 0, 0, -1000, 1000);
    plot_set_range(plot_2, -1, 1, -1, 1);
    plot_set_range(plot_3, -1, 1, -1, 1);

    for(uint32_t value = 0; value < 10000; value++)
    {
        float x = sinf((float)value / 16.9944444f) * value / 1000;
        float y = cosf((float)value / 16.1111111f) * value / 1000;
        float z = y * cosf((float)value / 13.33f) / ((1 + (float)value / 1000.0f));

        /* add some data for every plot */
        plot_add(coll_1, (y * 1000));
        plot_add(coll_2, x, y);
        plot_add(coll_3, z, x);

        /* override displayed range and autoscale */
        plot_autorange(coll_1, plot_1);
        plot_autorange(coll_2, plot_2);
        plot_autorange(coll_3, plot_3);

        /* now paint or redraw the plots */
        plot_graph_update(coll_1, plot_1);
        plot_graph_update(coll_2, plot_2);
        plot_graph_update(coll_3, plot_3);

        bmp_printf(FONT_MED, 15, 55, "Points: %d", value * 3);
        msleep(10);
    }
    msleep(10000);

#endif

    return 0;
}

static unsigned int plot_deinit()
{
    return 0;
}


MODULE_INFO_START()
    MODULE_INIT(plot_init)
    MODULE_DEINIT(plot_deinit)
MODULE_INFO_END()

