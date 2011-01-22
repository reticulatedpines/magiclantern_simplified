/* This is AJ's 5D2 code ported on the 550D
 * 
 *  Copyright (C) 2010 AJ_NEWMAN         -   AJ's routine library for the 5D2                     */

/*  aj_FalseColour_HistoCalc()   -                 HISTOGRAM = YES   FALSE_COLOUR=YES             */

extern unsigned int aj_FalseColour_HistoCalc(
          unsigned int g_arr_falsColhist, // R0 = g_arr_falsColhist . Bit0=1 to update histo
          unsigned int V_start,           // R1 = first word of Vram to process  
          unsigned int Vram_pixels,       // R2 = process this number of vram pixels
          unsigned int Bmp )              // R3 = first word of Overlay to write to
{
    // g_arr_falsColhist[256].   Top byte has false Overlay colour for that Luma level

    /******
    IRE
    101 >      [254] RED       
    95 - 100         ORANGE
    85 - 94          BRIGHT YELLOW
    77 - 84          GREENY YELLOW
    59 - 76          LIGHT GREY
    54 - 58          PINK
    48 - 53          SLIGHTLY DARK GREY
    42 - 47          GREEN
    22 - 41          DARK GREY
    12 - 21          LIGHT BLUE
    00 - 11          DARK BLUE
    <0         [02]  FUCHSIA
    ******/

   //--------------------------------------------------------------------
   //------  R E G I S T E R    L I S T  --------------------------------
   //--------------------------------------------------------------------
   // R0 = g_arr_falsColhist       	         R7  = Pix1   Vram line1 word1
   // R1 = V_start       	                 R8  = Pix2   Vram line1 word2
   // R2 = Vram_pixels                           R9  = <NOT USED>
   // R3 = Bmp_adr         	                 R10 = <NOT USED>
   // R4 = 1 If Histogram update required        R11 = <NOT USED>
   // R5 = Histo_luma_value   	                 R12 = <NOT USED>
   // R6 = Colour word to write to Overlay       R13 = STACK POINTER           
   //                                            R14 = TEMP WORKSPACE
   //--------------------------------------------------------------------
                                    
   asm volatile(

   //=========================================
   // SAVE REGISTERS BEFORE WE 'CLOBBER' THEM
   //=========================================
     
"     STMFD r13!,{r6,r7,r8,r14}\n" // Save registers that we are going to trash

"     BICS r1,#0x3\n"                 // V_start Must be word aligned, and Not Zero
"     BICNES r3,#0x3\n"               // Bmp Must be word aligned and Not Zero
"     BICNES r2,#0x3\n"               // Vram_pixels Must be a multiple of 4, and not Zero

"     STMFD r13!,{r4,r5}\n"           // Save registers that we are going to trash
"     AND  r4,r0,#0x1\n"              // r4=1 if histogram update is required
"     BICS r0,#0x1\n"                 // Remove this bit from g_arr_falsColhist

"     BEQ FLS_RoutineFailure\n"       // Return if problem 
 
//+=============================================+
//| MAIN LOOP  -      Read_4_Vram_Pixels	|
//+=============================================+

"FLS_AJ_MAIN_LOOP:\n"
"     MOV r6,#0x0\n"             //  Set Overlay pixel colour = completely see through
"     LDMIA r1!,{r7, r8}\n"      //  LDMIA V_start,{Pix2, Pix3}         
          
   //============================================================================================
   // Pix1a 
   //============================================================================================

"     AND   r14,r7,#0x0000FF00\n"       //  Luma of Vpixel 1  (shifted)
"     MOV   r14,r14,LSR#6\n"            //  Luma x 4 (ie create word position to add)

"     LDR   r5,[r0, r14]\n"             //  r5 = False Col + Hist value
"     CMP   r4,#1\n"                    //  r5 = False Col + Hist value
"     ADDEQ r5,r5,#1\n"                 //  r5 = Hist value
"     STREQ r5,[r0, r14]\n"             //  Write back new    Hist value
"     ORR   r6,r6,r5,LSR#24\n"          //  add to bottom (1st) byte of Overlay colour	

   //============================================================================================
   // Pix1b 
   //============================================================================================

"     AND   r14,r7,#0xFF000000\n"    //	
"     MOV   r14,r14,LSR#24-2\n"         //  Luma x 4  (ie create word position to add)

"     LDR   r5,[r0, r14]\n"             //  r5 = False Col + Hist value
"     CMP   r4,#1\n"                    //  r5 = False Col + Hist value
"     ADDEQ r5,r5,#1\n"                 //  r5 = Hist value
"     STREQ r5,[r0, r14]\n"             //  Write back new    Hist value

"     AND   r5,r5,#0xFF<<24\n"          //  r5 = top byte = False colour -> get it
"     ORR   r6,r6,r5,LSR#16\n"          //  add to 2nd byte of Overlay colour	

		
   //============================================================================================
   // Pix2a 
   //============================================================================================
	
"     AND   r14,r8,#0x0000FF00\n"    //	
"     MOV   r14,r14,LSR#6\n"            //  Luma x 4 (ie create word position to add)

"     LDR   r5,[r0, r14]\n"             //  r5 = False Col + Hist value
"     CMP   r4,#1\n"                    //  r5 = False Col + Hist value
"     ADDEQ r5,r5,#1\n"                 //  r5 = Hist value
"     STREQ r5,[r0, r14]\n"             //  Write back new    Hist value

"     AND   r5,r5,#0xFF<<24\n"          //  r5 = top byte = False colour -> get it
"     ORR   r6,r6,r5,LSR#8\n"           //  add to bottom (3rd) byte of Overlay colour	
	

   //============================================================================================
   // Pix2b 	
   //============================================================================================
	
"     AND r14,r8,#0xFF000000\n"    //	
"     MOV   r14,r14,LSR#24-2\n"         //  Luma x 4  (ie create word position to add)

"     LDR   r5,[r0, r14]\n"             //  r5 = False Col + Hist value
"     CMP   r4,#1\n"                    //  r5 = False Col + Hist value
"     ADDEQ r5,r5,#1\n"                 //  r5 = Hist value
"     STREQ r5,[r0, r14]\n"             //  Write back new    Hist value

"     AND   r5,r5,#0xFF<<24\n"          //  r5 = top byte = False colour -> get it
"     ORR   r6,r6,r5\n"                 //  add to 4th byte of Overlay colour	


//===============================================
// WriteWordToOverlay
//===============================================
"FLS_WriteWordToOverlay:\n" 	
"     STMIA r3!,{r6}\n"            //	

//===============================================
// ReadyForNextBlock	
//===============================================

"     SUBS r2,r2,#4\n"              //		
"     BNE FLS_AJ_MAIN_LOOP\n"      //		
	
//===============================================
// RoutineCompleted SUCCESSFULLY	
//===============================================

"FLS_RoutineComplete:\n"       	
"     LDMFD r13!,{r4,r5}\n" 
"     MOV   r0,#1\n"              //  SUCCESS	 
"     LDMFD r13!,{r6,r7,r8,r15}\n"     

//===============================================
// RoutineCompleted FAILED	
//===============================================

"FLS_RoutineFailure:\n"       	
"     LDMFD r13!,{r4,r5}\n" 
"     MOV   r0,#2\n"             //  FAILURE  	
"     LDMFD r13!,{r6,r7,r8,r15}\n"     

//===============================================
//===============================================
//=======   ^^ RETURN POINT OF ASM ^^  ==========
//===============================================
//===============================================	
	
      :             // Output operands   eg.  [output1]"+r"(g_r13_temp_store)
      :             // Input operands    eg.  [input]"m"(parm1)
      : "r0","r1","r2","r3"      // eg "memory","cc"    =   Clobber list       
   );  // end of asm volatile()

   return( 666 );

} /* end of aj_FalseColour_HistoCalc() */
