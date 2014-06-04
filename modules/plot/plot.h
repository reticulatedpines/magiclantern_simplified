
#ifndef __plot_h__
#define __plot_h__

#ifdef __plot_c__
    #define EXT_WEAK_FUNC(f) 
#else
    #define EXT_WEAK_FUNC(f) WEAK_FUNC(f)
#endif

/*
 * collection:
 *   { entry, entry, entry, entry, entry, ...}
 *
 * entry:
 *   { field, field, ...}
 *
 *
 *
 *
 */
 
#define PLOT_LINEAR        0
#define PLOT_XY            1

#define PLOT_COLOR_NONE    0x100

/* the data type we use */
typedef float plot_data_t;
#define PLOT_NONZERO(x) ((x) > 0.00001 || (x) < -0.00001)
#define PLOT_MAX (FLT_MAX)
#define PLOT_MIN (-FLT_MAX)


typedef struct
{
    /* this is the number of values stored per data entry. e.g. for X/Y data its two. could be even more and you pick some */
    uint32_t fields;
    /* pointer to the data buffer containing all values */
    plot_data_t *entries;
    /* number of plot data entries to be allocated at once */
    uint32_t extent;
    /* number of allocated entries */
    uint32_t allocated;
    /* number of entries already used in that buffer */
    uint32_t used;
    /* size of one added data set (e.g. 8 for a X/Y uint32_t pair) */
    uint32_t entry_size;
} plot_coll_t;

typedef struct
{
    plot_data_t min;
    plot_data_t max;
} plot_win_t;

typedef struct
{
    /* destination area for the plot */
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
    
    /* plot kind, X/Y or linear */
    uint32_t type;
    
    /* which field in source data is for which axis, only relevant for X/Y yet */
    uint32_t x_field;
    uint32_t y_field;
    
    /* sets the range which data to plot */
    plot_win_t x_win;
    plot_win_t y_win;
    
    /* if this is a LINEAR plot, enabling this will redraw the graph everytime a new data entry
       was added. if disabled, the plot will grow by points_min entries at once.
    */
    uint32_t dynamic_x;
    
    /* size of the dots to paint at every data spot */
    uint32_t dot_size;
    
    /* colors for all the plot parts */
    uint32_t color_border;
    uint32_t color_dots;
    uint32_t color_lines;
    uint32_t color_bg;
    uint32_t color_axis;
    uint32_t color_range;
    
    /* number of data points to be shown at minimum, also used as extent value when growing */
    uint32_t points_min;
    
    /* internal state variables for updating */
    uint32_t points_last;
    uint32_t points_drawn;
    uint32_t last_dot_x;
    uint32_t last_dot_y;
} plot_graph_t;


#if defined(MODULE)

plot_coll_t *EXT_WEAK_FUNC(ret_0) plot_alloc_data(uint32_t fields);
plot_graph_t *EXT_WEAK_FUNC(ret_0) plot_alloc_graph(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void EXT_WEAK_FUNC(ret_0) plot_free_data(plot_coll_t *);
void EXT_WEAK_FUNC(ret_0) plot_free_graph(plot_graph_t *);

uint32_t EXT_WEAK_FUNC(ret_0) plot_add(plot_coll_t *coll, ...);
void EXT_WEAK_FUNC(ret_0) plot_clear(plot_coll_t *coll);
void EXT_WEAK_FUNC(ret_0) plot_set_range(plot_graph_t *plot, plot_data_t x_min, plot_data_t x_max, plot_data_t y_min, plot_data_t y_max);
void EXT_WEAK_FUNC(ret_0) plot_graph_draw(plot_coll_t *coll, plot_graph_t *desc);
void EXT_WEAK_FUNC(ret_0) plot_graph_update(plot_coll_t *coll, plot_graph_t *plot);
void EXT_WEAK_FUNC(ret_0) plot_graph_reset(plot_graph_t *plot);
void EXT_WEAK_FUNC(ret_0) plot_autorange(plot_coll_t *coll, plot_graph_t *plot);
plot_data_t EXT_WEAK_FUNC(ret_0) plot_get_average(plot_coll_t *coll, uint32_t field);
void EXT_WEAK_FUNC(ret_0) plot_get_extremes(plot_coll_t *coll, uint32_t field, plot_data_t win_lo, plot_data_t win_hi, plot_data_t *ret_low, plot_data_t *ret_high);

#else

static plot_coll_t *(*plot_alloc_data) (uint32_t fields) = MODULE_FUNCTION(plot_alloc_data);
static plot_graph_t *(*plot_alloc_graph) (uint32_t x, uint32_t y, uint32_t w, uint32_t h) = MODULE_FUNCTION(plot_alloc_graph);
void (*plot_free_data) (plot_coll_t *) = MODULE_FUNCTION(plot_free_data);
void (*plot_free_graph) (plot_graph_t *) = MODULE_FUNCTION(plot_free_graph);

static uint32_t (*plot_add) (plot_coll_t *coll, ...) = MODULE_FUNCTION(plot_add);
static void (*plot_clear) (plot_coll_t *coll) = MODULE_FUNCTION(plot_clear);
static void (*plot_set_range) (plot_graph_t *plot, plot_data_t x_min, plot_data_t x_max, plot_data_t y_min, plot_data_t y_max) = MODULE_FUNCTION(plot_set_range);
static void (*plot_graph_draw) (plot_coll_t *coll, plot_graph_t *desc) = MODULE_FUNCTION(plot_graph_draw);
static void (*plot_graph_update) (plot_coll_t *coll, plot_graph_t *plot) = MODULE_FUNCTION(plot_graph_update);
static void (*plot_graph_reset) (plot_graph_t *plot) = MODULE_FUNCTION(plot_graph_reset);
static void (*plot_autorange) (plot_coll_t *coll, plot_graph_t *plot) = MODULE_FUNCTION(plot_autorange);
static plot_data_t (*plot_get_average) (plot_coll_t *coll, uint32_t field) = MODULE_FUNCTION(plot_get_average);
static void (*plot_get_extremes) (plot_coll_t *coll, uint32_t field, plot_data_t win_lo, plot_data_t win_hi, plot_data_t *ret_low, plot_data_t *ret_high) = MODULE_FUNCTION(plot_get_extremes);

#endif
#endif
