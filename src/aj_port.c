/* This is AJ's 5D2 code ported on the 550D
 * 
 *  Copyright (C) 2011 AJ_NEWMAN         -   AJ's routine library for the 5D2                     */

#include <dryos.h>
#include <bmp.h>
#include <propvalues.h>


// Notations: AJ <---> Unified ML
#define g_VOLATILE_lv_action (!lv)
#define g_bmp_width BMP_WIDTH
#define g_bmp_height BMP_HEIGHT


/********************************************************************
*                                                                   *
*  aj_green_screen() -                                              *
*                                                                   *
********************************************************************/

void aj_green_screen()
{
   static unsigned int last_green_screen_state = 0;

   /****************************************************
   *  .. if Canon menu is ACTIVE - return immediately  *
   ****************************************************/

   if (g_VOLATILE_lv_action != 0)
   {
      last_green_screen_state = 0;
      return;
   }

   /********************************
   *   Masses of auto variables.   *
   ********************************/

   unsigned int Xs,Xe;           // X_POSITION of  Cropmark_start/_end
   //~ Xs = g_cropmark_x_start;      // First Pix drawn on (and a multiple of 4)
   //~ Xe = g_cropmark_x_end;        // First Pix not drawn on (multiple of 4)
 
   unsigned int Vram_pixels = Xe - Xs;
 

   unsigned int vpix, lum1, lum2; 

   unsigned int total_luma   = 0;
   unsigned int highest_luma = 0;
   unsigned int lowest_luma  = 256;
   unsigned int total_pixels = 0;

   /****************************************************************
   *   Set address pointers up to first line in Vram               *   
   ****************************************************************/

   uint32_t* lv = get_yuv422_vram()->vram;
   uint8_t* bm = bmp_vram();
   uint16_t* bm16 = bmp_vram();
   uint8_t* bm_mirror = get_bvram_mirror();

   /******************************************************************
   *   Go through Crop area.  Note highest and lowest luma, average  *   
   ******************************************************************/

	for(int y = os.y0 + os.off_169; y < os.y_max - os.off_169; y+=2 )
	{
		for (int x = os.x0; x < os.x_max; x += 2)
		{
         /********************************************
         *  Get 4 bytes of vram (ie two vram Pixels) *
         ********************************************/

         // vpix   = [LSB=pix on left] u y1 v y2  [MSB=pix on right] 
         vpix = lv[BM2LV(x,y)/4];

         total_pixels += 2;
 
         lum1 = ( vpix & 0x0000FF00 ) >>  8;  // y1
         lum2 = ( vpix & 0xFF000000 ) >> 24;  // y2

         /*************************
         *  Update total luma     *
         *************************/

         total_luma += lum1 + lum2;

         /*************************
         *  new Maximum ?         *
         *************************/

         if (lum1 > highest_luma)
            highest_luma = lum1;

         if (lum2 > highest_luma)
            highest_luma = lum2;
     

         /*************************
         *  new Miniumum ?         *
         *************************/

         if (lum1 < lowest_luma)
            lowest_luma = lum1;

         if (lum2 < lowest_luma)
            lowest_luma = lum2;
      }
   } // end of (y loop)


   /************************
   *   Quick sanity Check  *   
   ************************/

   if (total_pixels == 0)
   {
      bmp_printf( FONT(FONT_MED,1,COLOR_BG), 
               150,g_bmp_height -24*2, 
               "Err: Green screen: total_pixels = 0! ");
      return;  
   }

   /**********************************
   *   Display average, min and max  *   
   **********************************/

   unsigned int average_luma = total_luma / total_pixels;

   bmp_printf( FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 
               os.x0 + os.x_ex/2 - font_med.width*7, os.y_max - os.off_169 - 47, 
               "Average = %03d",average_luma);

   bmp_printf( FONT(FONT_MED,COLOR_CYAN, COLOR_BLACK), 
               os.x0, os.y_max - os.off_169 - 47, 
               "MIN = %03d",lowest_luma);

   bmp_printf( FONT(FONT_MED,COLOR_YELLOW, COLOR_BLACK), 
               os.x_max - font_med.width*9,
               os.y_max - os.off_169 - 47, 
               "%03d = MAX",highest_luma);


   bmp_printf( FONT(FONT_MED,COLOR_WHITE, COLOR_BLACK), 
               os.x0 + os.x_ex/2 - font_med.width*7, os.y_max - os.off_169 - 27, 
               "Accurracy=%02d%%",((255-(highest_luma-lowest_luma))*99 )/255
             );

   bmp_printf( FONT(FONT_MED,COLOR_CYAN, COLOR_BLACK), 
               os.x0, os.y_max - os.off_169 - 27, 
               "delta %03d",average_luma - lowest_luma);

   bmp_printf( FONT(FONT_MED, COLOR_YELLOW, COLOR_BLACK), 
               os.x_max - font_med.width*9,
               os.y_max - os.off_169 - 27, 
               "%03d delta",highest_luma - average_luma);

   /*************  FIRST PASS DONE ... ON THE NEXT ONE WE'LL UPDATE THE OVERLAY *********/

   /****************************************************************
   *   Set address pointers up to first line in Vram / and Overlay *   
   ****************************************************************/

   unsigned int high_delta = highest_luma - average_luma;  // used to work out colour scale
   unsigned int low_delta  = average_luma - lowest_luma;   // colour scale for darker pixels

   /******************************************************************
   *   Go through Crop area.  Note highest and lowest luma, average  *   
   ******************************************************************/

    int high_delta_factor = 1024 / high_delta; // replace division with multiplication
    int low_delta_factor = 1024 / low_delta;

	for(int y = os.y0 + os.off_169; y < os.y_max - os.off_169; y += 2 )
	{
		uint32_t * const v_row = (uint32_t*)( lv        + BM2LV_R(y)    );  // 2 pixels
		uint16_t * const b_row = (uint16_t*)( bm        + BM_R(y)       );  // 2 pixels
		uint16_t * const m_row = (uint16_t*)( bm_mirror + BM_R(y)       );  // 2 pixels
		
		uint8_t* lvp; // that's a moving pointer through lv vram
		uint16_t* bp;  // through bmp vram
		uint16_t* mp;  // through mirror
		
		for (int x = os.x0; x < os.x_max; x += 2)
		{
			lvp = v_row + BM2LV_X(x)/2; lvp++;
			bp = b_row + x/2;
			mp = m_row + x/2;

         /********************************************
         *  Get 4 bytes of vram (ie two vram Pixels) *
         ********************************************/

         // vpix   = [LSB=pix on left] u y1 v y2  [MSB=pix on right] 
         vpix = lv[BM2LV(x,y)/4];

         total_pixels += 2;
 
         lum1 = ( vpix & 0x0000FF00 ) >>  8;  // y1
         lum2 = ( vpix & 0xFF000000 ) >> 24;  // y2

         int lum = (lum1 + lum2) / 2;
         /*********************************************************
         *  Initialise writeback colour of overlay to 0  for LUM1 *
         *********************************************************/

         int col = 0;

         /**************************************
         *  LUM1  Higher than average luma     *
         **************************************/
       
         if (lum > average_luma)
         {
            col = ((lum-average_luma)*12) * high_delta_factor / 1024;
            
            if (col > 12)
               col=12; 

            col = 128 + (col+2) * 8;
        
         }
         else if (lum < average_luma)
         {
            /**************************************
            *  LUM1   Lower than average luma     *
            **************************************/

            col = ((average_luma-lum)*12) * low_delta / 1024;

            if (col > 12)
               col=12; 

            col = 128 - (col+2) * 8;
         }

            if (col) col = ((col * 41) >> 8) + 38;
            int c = col | (col << 8);
            
			#define BP (*bp)
			#define MP (*mp)
			#define BN (*(bp + BMPPITCH/2))
			#define MN (*(mp + BMPPITCH/2))
			
			if (BP != 0 && BP != MP) { continue; }
			if (BN != 0 && BN != MN) { continue; }

			MP = BP = c;
			MN = BN = c;

			#undef BP
			#undef MP
			#undef BN
			#undef MN
        }

   } // end of (y loop)
  
   msleep(10); // don't kill the battery :)

} /* end of aj_green_screen() */
