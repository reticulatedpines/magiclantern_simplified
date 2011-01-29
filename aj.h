// ||  (~3)    #
/*************************
*     #DEFINES           *
*************************/

#define   OVERLAY_RADIUS1  (100 &(~3))         // Make sure its a multiple of 4!
#define   OVERLAY_RADIUS2  (160 &(~3))         // Make sure its a multiple of 4!
#define   OVERLAY_RADIUS3  (210 &(~3))         // Make sure its a multiple of 4!

/*************************
*     STUCTURES          *
*************************/


/*************************
*     GLOBAL VARIABLES   *
*************************/

extern unsigned int g_debug;    // NoDebug=0   Debug=1

unsigned int g_aj_force_bmp_start_line=0;  // set to  0 when not in use
unsigned int g_aj_force_bmp_end_line  =99; // set to 99 when not in use

/************   YUV -> LCD pallette transform   ******************/ 
extern uint8_t * g_Yuv2LcdPal;    // malloced in code

unsigned int *g_arr_falsColhist=0;                 // get malloced in cachable memory


/*****************   VRAM   ******************/ 

extern unsigned int g_update_vram_n_bmp;    // must be set to one when Record is pressed 
                                                // or if this the first time around

// The Video RAM (Vram) is 16 bits per pixel YUV fomat.4:2:0 PO16 ??
// We know there is a 'Y' (luma) plane of pixels that is 1920x1080 (or 720x480)
// Haven't found the UV plane yet ... it may be there is color info in the Y plane?  
// Each horizontal line has space for 'pitch' vram pixels, of which 'width' are used.

extern unsigned int g_vram_bank;            // 0 or 1
extern unsigned int g_vram_base_addr;
extern unsigned int g_vram_pitch;           // [HD] 1920      [STD]=960
extern unsigned int g_vram_width;           // [HD] 1920      [STD]=720
extern unsigned int g_vram_width;           // [HD] 1920      [STD]=720
extern unsigned int g_vram_height;          // [HD] 1080      [STD]=480 [Only 0..424 used]
extern unsigned int g_vram_bytes_per_line;  // = 2 x pitch   

/*****************   BMP (LCD & HDMI memory for Overlay display)   ******************/ 

// The BMP (bitmap) RAM is used by the DryOs to display menus & controls.
// this Overlays on the image created from the Vram, and displayed on the LCD or HDMI.

// When to a High Def (HD) HDMI display, the amount of Bmp memory is 960x 540
// When connected to a std-def HDMI display, the amount of Bmp memory is 720 x 480.
// Each horizontal line has space for 'pitch' bmp pixels, of which 'width' are used.

extern unsigned int g_bmp_base_addr;
extern unsigned int g_bmp_pitch;    //     [HD] 960     [STD] 960    <= ALWAYS 960!
extern unsigned int g_bmp_width;    //     [HD] 960     [STD] 720
extern unsigned int g_bmp_height;   //     [HD] 540     [STD] 480

// When Record is pressed and in HD_HDMI mode, the memory map stays the same, the
// display drivers output is changed from 1920x1080 ->  720x480

/*****************   Menu event   ******************/ 

//extern struct event g_event;                     // defined in gui.c
//extern unsigned int g_event_READY_TO_PROCESS;    // 0 or 1     ( defined in gui.c )

/*****************   GLOBAL KEY ACTIONS   ********************************/ 
int g_aj_overlay_button = 0;   // -1 = Overlay->OFF       0 = DoNothing   1 = Overlay->magnify
int g_aj_rotate_dial    = 0;   // -1 = Anticlockwise.     0 = DoNothing   1 = Clockwise
int g_aj_joy_y          = 0;   // -1 = ZoomOrigin->Down   0 = DoNothing   1 = ZoomOrigin->Up
int g_aj_joy_x          = 0;   // -1 = ZoomOrigin->Left   0 = DoNothing   1 = ZoomOrigin->Right

/*****************   GLOBAL CONTROL OF SCREEN FUNCTIONS   ****************/ 
unsigned int g_display_false_colour = 0; // 0=no,1=display. When active,no Zebra / Overlay displayed
unsigned int g_display_histogram    = 1; // Calculate and display the Histogram
unsigned int g_display_overlay      = 0; // 0 =no,     1=yes  (starting condition)
unsigned int g_display_zebras       = 1; // 0 =no,     1=yes  (should always be on I think)
unsigned int g_fast_zebra           = 1; // 0=Dipslay every line.  1=Skip lines (may look better)
unsigned int g_fast_histo           = 1; // 0=Do Every Pixel. 1=Average two at the time.
unsigned int g_logarithmic_histo    = 1; // 0=Dipslay in Base 2 logarithmic scale (may look better)
unsigned int g_draw_cropmarks       = 1; // 0=no  1=Yes
unsigned int g_display_fps          = 0; // 0=no  1=Yes   (Only for debugging really!)
unsigned int g_aj_keys_active       = 1; // 0 =no,     1=yes  (respond to hotkeys)
unsigned int g_display_audio_meters = 1; // 0 =no,     1=yes  
unsigned int g_display_audio_lvl_num= 0; // 0 =no,     1=yes  
unsigned int g_display_text_info    = 1; // 0 =no,     1=yes  
unsigned int g_display_text_info_typ= 0; // 0 =ISO APERTURE 

unsigned int g_focus_changed        = 0; // 0 =no,     1=yes  
unsigned int g_focus_trigger        = 0; // 0 =no,     1=yes  
unsigned int g_focus_deTrigger_time = 5; // 0 =Dont switch off.  N = Seconds to deactivate
unsigned int g_overlay_was_switched_on_by_FOCUS_TRIGGER = 0;

unsigned int g_countdown_to_screendump = 0; // 0 = no count. 1=take pic. >1 .. frames to go
unsigned int g_battery_icon = 1;         // 0 =no,     1=yes 
unsigned int g_cmos_temp_info = 1;       // 0 =no,     1=yes 

unsigned int g_overheating = 0;          // 0 =no,     1=yes  2=Camera meltdown imminent!


/*******************  What needs to be drawn when we start   *************/ 
unsigned int g_clear_screen            =1; // Signal a clear screen
unsigned int g_redraw_vram_pos_icon    =1; // Draw as if a clear screen has just occurred
unsigned int g_redraw_overlay_pos_icon =1; // Draw as if a clear screen has just occurred
unsigned int g_redraw_record_icon      =1; // Draw as if a clear screen has just occurred
unsigned int g_redraw_cropmarks        =1; // Draw as if a clear screen has just occurred
unsigned int g_redraw_histogram_icon   =1; // Draw as if a clear screen has just occurred
unsigned int g_redraw_audio_icon       =1; // Draw as if a clear screen has just occurred
unsigned int g_redraw_battery_icon     =1; // Draw as if a clear screen has just occurred
unsigned int g_redraw_cmos_temp_icon   =1; // Draw as if a clear screen has just occurred
unsigned int g_redraw_text_info_icon   =1; // Draw as if a clear screen has just occurred

/*************************************************************************/ 
//PROPERTY WORKSPACE (and debug ish) space === UPDATED IN PROPERTY ROUTINES

#define PROP_BATTERY_REPORT 	0x8003001D     // this is the one I'm using
#define PROP_BATTERY_HISTORY    0x0204000F
#define PROP_BATTERY_CHECK 	0x80030013

unsigned int g_battery_power = 0;     // 0 = no power.  Aprox 607 = Full
#define BATTERY_ICON_WIDTH  (24)
unsigned int g_battery_icon_width = BATTERY_ICON_WIDTH;  // size of battery icon

extern unsigned long g_efic_temp;   // defined in PROP_INT macro at bottom of aj.c


/*************************************************************************/ 

unsigned int g_overlay_size   =2;     // 1 = small, 2=large, 3=fullscreen
                                      // if was off -> go back to previous size
                                      // else cycle between 1-2-3-1-2-3 ....
unsigned int g_overlay_position = 0;  // 0=TOP_LEFT, 1=TOP_RIGHT, 2=BOT_RIGHT, 3=BOT_LEFT

unsigned int g_overlay_radius = OVERLAY_RADIUS2;   // size of current overlay 
                                       // This must be equate to g_overlay_size
                                       // Used to ensure we don't hit any borders.  


unsigned int g_overlay_centre_x  =525; // Between 0..719   (more like 100-619) 
                                       // Position on LCD where Overlay displayed
                                       // stretch if Overlay X Resolution greater then 720 

unsigned int g_overlay_centre_y  =240; // Between 0..480   (more like 100-300) 
                                       // Position on LCD where Overlay displayed
                                       // stretch if Overlay Y Resolution greater then 48
unsigned int g_Overlay_icon_x_from_right = 74; // positon of 'find icon' from bottom right corner

unsigned int g_hd_vram_mag_x = 525;  // Magnify from this point in vram
                             // stretch if Vram X Resolution greater then 720 

unsigned int g_hd_vram_mag_y = 240;  // Magnify from this point in vram
                             // stretch if Vram Y Resolution greater then 480 
unsigned int g_vram_step_size  = 3; // Vram origin will jump by g_overlay_radius / g_vram_step_size

unsigned int g_update_overlay_array = 1;  //=1 when the overlay 'cut_out' struct needs updating


//---------- HD VRAM related ---------------------

unsigned int g_hd_vram_base_addr      = 0x44000080;   // HACK HACK HACK = Segment 1 on 5D2
unsigned int g_hd_vram_bytes_per_line = 2048; // Changes to 3744 when recording  


unsigned int g_hd_vram_width     =  2048 / 2;
                 // starting value =  g_hd_vram_bytes_per_line / 2;

unsigned int g_hd_vram_height    = 480 * 2048 / 1440;
    // starting value = (g_vram_height * g_hd_vram_bytes_per_line)/g_vram_bytes_per_line;
                    

//---------- CROPMARKS related ---------------------

unsigned int g_cropmarks_override = 0;    // 0=no  1=Yes. Allow users to kabosh current range
unsigned int g_cropmarks_override_top_left = 1;    // 1=top_left  0=bot_right
unsigned int g_cropmarks_type = 0;        // FULL SCREEN

int g_overide_joy_x = 0;                // Camera button input
int g_overide_joy_y = 0;                // Camera button input
unsigned int g_overide_switch_pos = 0;  // Camera button input

      // Master is the 720p line. Full screen X.  Y is 0..480
unsigned int g_cropmark_Master_x_start = 0   &(~3);  // 0..716  Cropmark has 4 pixel width
unsigned int g_cropmark_Master_x_end   = 720 &(~3);  // 0..716  (Not including this pixel)
unsigned int g_cropmark_Master_y_start = 0;      // 0..480  ([33] line is drawn. [32] overlay drawn
unsigned int g_cropmark_Master_y_end   = 425;    // 0..480  ([424] drawn.  [425] Overlay dran at

extern unsigned int g_HDMI_connected;     // 0 = not connected.  1= connected

      // Dependent on screen mode - this changes (ie using HDMI / 960 pixel width)
unsigned int g_cropmark_x_start = 75  &(~3);  // 0..716  
unsigned int g_cropmark_x_end   = 600 &(~3);  // 0..716  (Not including this pixel)
unsigned int g_cropmark_y_start = 50;         // 0..480  ([33] line is drawn. [32] overlay drawn
unsigned int g_cropmark_y_end   = 400;        // 0..480  ([423] drawn.  [424] Overlay dran at


struct 
{
   const char *name;
   unsigned int min_x;
   unsigned int max_x;
   unsigned int min_y;
   unsigned int max_y;
} g_crop_mark_type[] =
{
    {"Full Screen of Fun ",   0,  720,0,  425},
    {"Audio              ",   0,  720,33, 425},
    {"Audio trimmed      ",   30, 700,33, 425},
    {"Small Window stuff ",   100,600,100,400},
    {"FrEaKy - 'Far Side'",   133,333,133,233},
};
       
unsigned int g_crop_types = sizeof(g_crop_mark_type) / sizeof(g_crop_mark_type[0]);


// ----------- RECORDING related ---------------------

unsigned int g_recording_state=0;    // = 0 (not recording)     =1 (recording)
unsigned int g_record_second_flash;  // = 0 or 1
unsigned int g_rec_time_left=0;      // = time remaining in seconds

/*************************
*     PROTOTYPES         *
*************************/

extern void aj_zoom_rectangle_info( int *, 
                             int *, 
                             unsigned int *,
                             unsigned int *,
                             unsigned int *,
                             unsigned int *,
                             unsigned int *,
                             unsigned int *,
                             unsigned int *,
                             unsigned int *,
                             unsigned int );

extern unsigned int aj_update_vram_n_bmp_global_varables(void);

extern unsigned int aj_counter( unsigned int  , unsigned int);



extern void aj_draw_pixel(
	unsigned int    ,       /* x = 0 -> width - 1             */
	unsigned int	,       /* y = 0 -> height -1             */
	unsigned int	,       /* 0 ... 255                      */
        unsigned int            /* 0 = do nothing,    1 = xor     */
);

extern void aj_draw_pixel_word(
	unsigned int    ,       /* x = 0 -> width - 1             */
	unsigned int	,       /* y = 0 -> height -1             */
	unsigned int	,       /* 0 ... 0xFFFFFFFF               */
        unsigned int            /* 0 = do nothing,    1 = xor     */
);



extern unsigned int aj_abs( int );

extern unsigned int aj_create_log_file( char *);
extern void aj_close_log_file( void );
extern void aj_write_time_to_log_file( void );



extern void aj_framerate( unsigned int, unsigned int );


extern void aj_testcard( void );
            // Creates a 16 x 16 grid with each Colour in Hex in each box
extern void aj_testcard_yuv( void );
            // Creates a 16 x 16 grid with each YUV colour -> dumps to CF
extern void aj_testcard_rgb( void );
            // Creates a 16 x 16 grid with each RGB colour -> dumps to CF
extern void aj_LCD_palette_yuv_rgb_to_file( void);
            // prints out a log of all colour information for each colour to CF


extern unsigned int aj_create_YUV_trsfrm_array( void ); 
                    // create Lookup table for transform of YUV -> LCD palette

extern unsigned int aj_YUV_to_LCD_palette( unsigned int , unsigned int , unsigned int );
                // uses tranform lookup table to find closest two colours

extern void AJ_Write_BmpVram( void );  
            // I think this writes VRAM to a bmp file


extern unsigned int aj_check_if_vram_gobal_vars_need_refreshing(void);
            // should be replaced by a routine that works out if variables have changed.

// LIB ABOVE ************************************************************************************
// LIB ABOVE ************************************************************************************
// LIB ABOVE ************************************************************************************
// LIB ABOVE ************************************************************************************



extern void aj_main(void);
            // DUMMY routine to test AJ's funcs.  Should be replaced by a Task

extern void aj_dump_LiveViewMgr_struct_0x2930( void );
            // bit meaning of LiveViewMgr struct [0x2930]

extern void aj_bmp_n_vram_locations(void);
            // printout of bmp vram struct

extern void aj_force_HD_HDMI(void);
            // attempt to get Full HD during recording
                               

                               

extern unsigned int aj_Zebra(unsigned int ,    // R0 top 2 bits = ZigZag
                              unsigned int ,    // R1 = V_start =first word of Vram to process  
                              unsigned int ,    // R2 = Vram_pixels (Must be multiple of 4, and >0) 
                              unsigned int  );  // R3 = Bmp = first word of Overlay to write to

extern unsigned int aj_VramToOverlay3(unsigned int ,       // R0
                                      unsigned int ,     // R1 first Vram line  
                                      unsigned int ,  // R2 
                                      unsigned int  );        // R3 Overlay line 1


extern unsigned fred_sqrt(unsigned long );


extern unsigned int aj_HistoCalc(
          unsigned int ,      // R0 = g_histogram
          unsigned int ,      // V_start  R1 = first word of Vram to process  
          unsigned int );     // Vram_pixels R2 = process this number of vram pixels     


extern unsigned int aj_Zebra_HistoCalc(
       unsigned int, // ZigZag_Histogram, // R0 = [top 2 bits = ZigZag][Bottom 30 bits = Histogram]
       unsigned int, // V_start,          // R1 first word of Vram to process  
       unsigned int, // Vram_pixels,      // R2 = process this number of vram pixels
       unsigned int); // Bmp )             // R3 = first word of Overlay to write to


extern unsigned int aj_FalseColour_HistoCalc(
          unsigned int,  // Histogram,        // R0 = g_arr_falsColhist
          unsigned int,  // V_start,          // R1 = first word of Vram to process  
          unsigned int,  // Vram_pixels,      // R2 = process this number of vram pixels
          unsigned int); // Bmp )             // R3 = first word of Overlay to write to


extern unsigned int aj_HistoCalc_fast(
          unsigned int ,      // R0 = g_histogram
          unsigned int ,      // V_start  R1 = first word of Vram to process  
          unsigned int );     // Vram_pixels R2 = process this number of vram pixels     


extern unsigned int aj_Zebra_HistoCalc_fast(
       unsigned int, // ZigZag_Histogram, // R0 = [top 2 bits = ZigZag][Bottom 30 bits = Histogram]
       unsigned int, // V_start,          // R1 first word of Vram to process  
       unsigned int, // Vram_pixels,      // R2 = process this number of vram pixels
       unsigned int); // Bmp )             // R3 = first word of Overlay to write to



extern unsigned int aj_CLZ(  unsigned int );    // Do an ASM CLZ - for Base 2 Log calc

extern void aj_draw_box(
	unsigned int, //    x_origin,       /* x = 0 -> width - 1             */
	unsigned int, //	y_origin,       /* y = 0 -> height -1             */
        unsigned int, //    width,
        unsigned int, //    height,
        unsigned int, //    line_thickness,
	unsigned int, //	colour,  /* 0 ... 255                      */
        unsigned int  //  xor      /* 0 = do nothing,    1 = xor     */
);



extern void aj_draw_ellipse(
	unsigned int, //   x_origin,       /* x = 0 -> width - 1             */
	unsigned int,//	y_origin,       /* y = 0 -> height -1             */
        unsigned int, //   width,
        unsigned int, //   height,
        unsigned int, //   line_thickness,
	unsigned int,//	colour,  /* 0 ... 255                      */
        unsigned int //   xor      /* 0 = do nothing,    1 = xor     */
);

extern void aj_draw_ellipse2(
	unsigned int, //   x_origin,       /* x = 0 -> width - 1             */
	unsigned int,//	y_origin,       /* y = 0 -> height -1             */
        unsigned int, //   width,
        unsigned int, //   height,
        unsigned int, //   line_thickness,
	unsigned int,//	colour,  /* 0 ... 255                      */
        unsigned int //   xor      /* 0 = do nothing,    1 = xor     */
);


extern void aj_draw_line(
	unsigned int ,  // start_x,       /* x = 0 -> width - 1             */
	unsigned int,	// start_y,       /* y = 0 -> height -1             */
        int ,  // width,
        int ,  // height,
        unsigned int  , // line_thickness,
	unsigned int,	// colour,  /* 0 ... 255                      */
        unsigned int   //  xor      /* 0 = do nothing,    1 = xor     */
);
