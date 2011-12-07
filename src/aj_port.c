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

   // results from previous loop, used for display
   static int total_luma   = 0;
   static int highest_luma = 0;
   static int lowest_luma  = 256;
   static int total_pixels = 0;

   // results for current loop, being updated (will be used at next loop)
   int total_luma_tmp   = 0;
   int highest_luma_tmp = 0;
   int lowest_luma_tmp  = 256;
   int total_pixels_tmp = 0;

   /****************************************************************
   *   Set address pointers up to first line in Vram               *   
   ****************************************************************/

   uint32_t* lv = get_yuv422_vram()->vram;
   uint8_t* bm = bmp_vram();
   uint16_t* bm16 = bmp_vram();
   uint8_t* bm_mirror = get_bvram_mirror();

   unsigned int average_luma = total_luma / total_pixels;
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

         total_pixels_tmp += 2;
 
         lum1 = ( vpix & 0x0000FF00 ) >>  8;  // y1
         lum2 = ( vpix & 0xFF000000 ) >> 24;  // y2

         /*************************
         *  Update total luma     *
         *************************/

         total_luma_tmp += lum1 + lum2;

         /*************************
         *  new Maximum ?         *
         *************************/

         if (lum1 > highest_luma_tmp)
            highest_luma_tmp = lum1;

         if (lum2 > highest_luma_tmp)
            highest_luma_tmp = lum2;
     

         /*************************
         *  new Miniumum ?         *
         *************************/

         if (lum1 < lowest_luma_tmp)
            lowest_luma_tmp = lum1;

         if (lum2 < lowest_luma_tmp)
            lowest_luma_tmp = lum2;

         /*********************************************************
         *  Initialise writeback colour of overlay to 0  for LUM1 *
         *********************************************************/

         int lum = (lum1 + lum2) / 2;
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
  

   /**********************************
   * commit statistics for next loop *
   **********************************/

   total_luma = total_luma_tmp;
   highest_luma = highest_luma_tmp;
   lowest_luma = lowest_luma_tmp;
   total_pixels = total_pixels_tmp;

   /**********************************
   *   Display average, min and max  *   
   **********************************/

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
               "delta %03d", COERCE(average_luma - lowest_luma, 0, 255));

   bmp_printf( FONT(FONT_MED, COLOR_YELLOW, COLOR_BLACK), 
               os.x_max - font_med.width*9,
               os.y_max - os.off_169 - 27, 
               "%03d delta", COERCE(highest_luma - average_luma, 0, 255));

   msleep(10); // don't kill the battery :)

} /* end of aj_green_screen() */



#if 0 // doens'tworkstation
/*************************************************************************************************
*                                                                                                *
*  aj_CLZ()  -  Do an asm( CLZ  )                                                                *
*                                                                                                *
*************************************************************************************************/

unsigned int aj_CLZ( unsigned int input_num)
{
   asm volatile(

"     CLZ r0,r0\n" 
"     MOV r15,r14\n"     
//===============================================
//===============================================
//=======   ^^ RETURN POINT OF ASM ^^  ==========
//===============================================
//===============================================	
	
      :             // Output operands   eg.  [output1]"+r"(g_r13_temp_store)
      :             // Input operands    eg.  [input]"m"(parm1)
      : "r0"        // eg "memory","cc"    =   Clobber list       
   );  // end of asm volatile()

   return( 666 );

} /* end of aj_clz() */



/********************************************************************
*                                                                   *
*  aj_log_length() -                                                *
*                                                                   *
********************************************************************/

unsigned int aj_log_length( unsigned int val )
{
    /****************************************************************************
    *            8 765 4321 = yclz (highest bit with 1 in it)                   *
    *                                                                           *
    *           val    =  1 010 0000                                            *
    *         yclz       [1]          = Digit of first '1'bit from right = 8    *    
    * Wipe top bit     =  x 010 0000                                            *
    * 2 next sig bits  =         x10  = remainder                               *
    *                                                                           *
    *  Screen Length   = yclz x 4  + remainder = 8x4 +  1  = 33                 *
    *****************************************************************************

                          0 00 0 ->  0 x 4  + 0   = 0     (0)
                          0 00 1 ->  1 x 4  + 0   = 4     (1)
                          0 01 0 ->  2 x 4  + 0   = 8     (2)
                          0 01 1 ->  2 x 4  + 2   = 10    (3)       (+2 because 1<<1)
                          0 1 00 ->  3 x 4  + 0   = 12    (4 = 12-8) 
                          0 1 01 ->  3 x 4  + 1   = 13    (5)
                          0 1 10 ->  3 x 4  + 2   = 14    (6)
                          0 1 11 ->  3 x 4  + 3   = 15    (7)
                          1 0 00 ->  4 x 4  + 0   = 16    (8)
     */


   /*******************************************************************************************
   *   Code looks cleaner hard coding the low values .. (it'll give you less of a headache!)  *
   *******************************************************************************************/
        
   if ( val < 4 )
   {
      /* this is my first attempt

      if (val == 0)
         return( 0 );
      if (val == 1)
         return( 4 ); 
      if (val == 2)
         return( 8 );
      return( 10 );
      */

      return( val ); // ie 0..3
   }


   /*******************************************************
   *   OK .. use CLZ from ASM to count the leading Zeros  *
   *******************************************************/
   
   unsigned int yclz = 32 - aj_CLZ( val );      // if val =0,        yclz= 0
                                                // if val =2^32-1,   yclz= 32
  
   /********************************************        
   *   Get 2nd and 3rd most significant bits   *              
   ********************************************/            
                             
   val = (val >> (yclz - 3 ) ) & 3;     //   1 01 0000  yclz=7.  Need to shift right 4 bits.     
 
   // return(  yclz*4  + val);       first attempt

  
   return(  yclz*4  + val - 8);    // first value = 12 - 8  = 4


}  /* end of aj_log_length() */     
 
#else
int log_length(int v)
{
    if (!v) return 0;
    return (int)(log2f(v) * 100);
}
#endif
