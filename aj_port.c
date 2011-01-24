/* This is AJ's 5D2 code ported on the 550D
 * 
 *  Copyright (C) 2010 AJ_NEWMAN         -   AJ's routine library for the 5D2                     */

/*  aj_FalseColour_HistoCalc()   -                 HISTOGRAM = YES   FALSE_COLOUR=YES             */

#include "dryos.h"
#include "bmp.h"
#include "version.h"
#include "config.h"
#include "menu.h"
#include "property.h"
#include "gui.h"
#include "lens.h"

int g_cropmark_y_start = 100;
int g_cropmark_x_start = 100;
int g_cropmark_x_end = 720-100;
int g_cropmark_y_end = 480-100;
int g_display_false_colour = 1;
unsigned int * g_arr_falsColhist = 0;
void* g_bmp_base_addr = 0;
int g_bmp_pitch = 0;
int g_bmp_height = 0;
void* g_vram_base_addr = YUV422_LV_BUFFER;
int g_vram_bytes_per_line = YUV422_LV_PITCH;
int g_display_histogram = 0;


/***********************************************************************************************
*                                                                                              *
*  aj_DisplayFalseColour_n_CalcHistogram()                                                     *     *                                                                                              *
*  Uses Marshall like colour scheme to display colour                                          *                                                  *                                                                                              *                                                                                              
***********************************************************************************************/	
void aj_DisplayFalseColour_n_CalcHistogram(void)
{   
   /*************************       **********************
   *  Display False_Colour  *   +   *   Calc Histogram   *   Only Do what is required
   **************************       *********************/

   if (   g_display_false_colour==0   ||  g_arr_falsColhist ==0)
      return;

   /********************************
   *   Masses of auto variables.   *
   ********************************/

   unsigned int Xs,Xe;           // X_POSITION of  Cropmark_start/_end
   Xs = g_cropmark_x_start;      // First Pix drawn on (and a multiple of 4)
   Xe = g_cropmark_x_end;        // First Pix not drawn on (multiple of 4)

 
   unsigned int Bmp_line_base;     // Fist Byte of Overlay memory on each line in Cropped area
   Bmp_line_base = g_bmp_base_addr + 
                   g_cropmark_y_start * g_bmp_pitch; 

   unsigned int V_start_line_base; // Fist Byte of std_vram memory on each line in Cropped area
   V_start_line_base = g_vram_base_addr + 
                       g_cropmark_y_start * g_vram_bytes_per_line; 
 
   unsigned int Vram_pixels = Xe - Xs;
 
   unsigned int ss;             // status returned from asm routine
   unsigned int y;              // Bmp screen y position counter in for loop

   /*************************************************************
   *   Go through Visible lines in Overlay Matrix and display   *   
   *************************************************************/

   for(  y=g_cropmark_y_start;   y<g_cropmark_y_end;   
                y++, 
                Bmp_line_base += g_bmp_pitch,
                V_start_line_base += g_vram_bytes_per_line
      )  
   {        
      
      ss =aj_FalseColour_HistoCalc(
          ((unsigned int) g_arr_falsColhist) | g_display_histogram, // R0 = False Colour/ Histogram
                         V_start_line_base + Xs*2,  // R1 first word of Vram to process  
                         Vram_pixels,               // R2 = process this number of vram pixels
                         Bmp_line_base + Xs);       // R3 = first word of Overlay to write to

      if (ss != 1)
      {
         bmp_printf( FONT_MED, 30, 32,  "ErrFalseColHisto z=%d ", ss);
         return;
      }       


   } // end of (y loop)


} /* end of aj_DisplayFalseColour_n_CalcHistogram() */


#define MAX_FALSE_COLOUR_LEVELS    (256)   
//unsigned int *g_arr_falsColhist=0;                 // get malloced in cachable memory

void aj_malloc_false_colour_histogram_struct() // why not use AllocateMemory? 
{
   unsigned int tmp_addr = 
                  (unsigned int) malloc( sizeof(unsigned int) * MAX_FALSE_COLOUR_LEVELS +64);

   if ( tmp_addr == 0 )
   {
      bmp_printf( FONT(FONT_SMALL,COLOR_RED,COLOR_BG), 000, g_bmp_height-16, 
                      "AJ: FalseCol Malloc failed. ");      
      return; 
   }

   tmp_addr = (tmp_addr + 32)  & (~31);

   g_arr_falsColhist = (unsigned int *) tmp_addr;

   /*********************************************************************************
   * Initialise it.                                                                 *
   * Actual structure:  Top byte       = Overlay 8bit colour for this False Colour  *
   *                    Bottom 3 bytes = Histogram count of number of occurances    *   
   *********************************************************************************/

    unsigned char false_colour[MAX_FALSE_COLOUR_LEVELS] = {
0x0E, 0x0E, 0x0E, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0xAE, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6E, 0x6F 
	    };

   unsigned int x;

   for(x=0; x<MAX_FALSE_COLOUR_LEVELS; x++)
   {
//NOP AJAJ
asm volatile ( "nop" );
asm volatile ( "nop" );
asm volatile ( "nop" );
asm volatile ( "nop" );

      g_arr_falsColhist[x] = false_colour[x] << 24;
   }

} /*  end of aj_malloc_false_colour_histogram_struct */


static void init_aj_stuff(void)
{
	g_bmp_base_addr = bmp_vram();
	g_bmp_pitch = BMPPITCH;
	g_bmp_height = 480;
    aj_malloc_false_colour_histogram_struct();
}

INIT_FUNC("aj", init_aj_stuff);
