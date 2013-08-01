#ifndef __PLATFORM_MVR_H__
#define __PLATFORM_MVR_H__

// Movie recording.

extern struct mvr_struct * mvr_struct;
extern struct state_object * mvr_state;

// tab size: 4

/***** Added by from AJ 2.0.4 IDC *************************************************************
*                                                                                             *
*  mvr_config  (also called aAJ_Movie_CompressionRate_struct_0x86AC_0x00_to_0xA8 in AJ 2.0.4  *
*                                                                                             *
***********************************************************************************************
*  Callable DRYOS functions that relate to configuring the H264 compression.
*
*  "mvrSetQscale"
*  "mvrSetQscaleYC"
*  "mvrSetDeblockingFilter"
*  "mvrSetLimitQScale"
*  "mvrSetDefQScale"
*  "mvrSetTimeConst"
*  "mvrSetFullHDOptSize"
*  "mvrSetVGAOptSize"
*  "mvrSetGopOptSizeFULLHD"
*  "mvrSetGopOptSizeVGA"
*  "mvrSetD_FULLHD"
*  "mvrSetD_VGA"
*  "mvrFixQScale"
*  "mvrSetDefDBFilter"
*  "mvrSetPrintMovieLog"
*
**********************************************************************************************/

struct mvr_config   // used in aj_bitrate.c to change the CBR and VBR     mvr_config=0x86AC
{
   uint16_t   debug_flag;              // 0x00, 1 = write debugmsg's
   uint16_t   qscale_mode;             // 0x02, {CBR=0, VBR(QScale)=1} AJ_mvrFixQScale()

   uint16_t   db_filter_a;             // 0x04, AJ_mvrSetDefDBFilter()  Alex: No effect?
   uint16_t   db_filter_b;             // 0x06, AJ_mvrSetDefDBFilter()  Alex: No effect?

   int16_t    def_q_scale;             // 0x08, AJ_mvrSetDefQScale() VBR works when qscale_mode=1
   int16_t    def_q_scale2;            // 0x0A

   int16_t    qscale_limit_L;          // 0x0C, AJ_mvrSetLimitQScale()
   int16_t    qscale_limit_H;          // 0x0E, AJ_mvrSetLimitQScale()

   uint16_t   time_const;              // 0x10, AJ_mvrSetTimeConst()  [0..255]?
                                       //       AJ_mvrFixQScale() <-- prints out in this
   uint16_t   x12;                     // 0x12

   /***************************************
   *   1920 pix @ 30 fps  -------  CBR    *
   ***************************************/

   unsigned int   v1920_30fps_opt_size_I;    // 0x14   AJ_mvrSetFullHDOptSize() [I]
   unsigned int   v1920_30fps_opt_size_P;    // 0x18   AJ_mvrSetFullHDOptSize() [P]
   unsigned int   v1920_30fps_D_H;           // 0x1C   AJ_mvrSetD_FULLHD() copy to -> 0x30
   unsigned int   v1920_30fps_D_L;           // 0x20   AJ_mvrSetD_FULLHD() copy to -> 0x34,0x44,0x48
   unsigned int   v1920_30fps_kinda_counter; // 0x24


   /***************************************
   *   1920 pix @ 25 fps  -------  CBR    *
   ***************************************/

   unsigned int   v1920_25fps_opt_size_I;    // 0x28  AJ_mvrSetFullHDOptSize() [I]
   unsigned int   v1920_25fps_opt_size_P;    // 0x2C  AJ_mvrSetFullHDOptSize() [P]
   unsigned int   v1920_25fps_D_H;           // 0x30  AJ_mvrSetD_FULLHD() <-- 0x1C
   unsigned int   v1920_25fps_D_L;           // 0x34  AJ_mvrSetD_FULLHD() <-- 0x20
   unsigned int   v1920_25fps_kinda_counter; // 0x38


   /***************************************
   *   1920 pix @ 24 fps  -------  CBR    *
   ***************************************/

   unsigned int   v1920_24fps_opt_size_I;    // 0x3C  AJ_mvrSetFullHDOptSize()   IOptSize
   unsigned int   v1920_24fps_opt_size_P;    // 0x40  AJ_mvrSetFullHDOptSize()   POptSize
   unsigned int   v1920_24fps_D_H;           // 0x44  AJ_mvrSetD_FULLHD() <-- 0x20
   unsigned int   v1920_24fps_D_L;           // 0x48  AJ_mvrSetD_FULLHD() <-- 0x20
   unsigned int   v1920_24fps_kinda_counter; // 0x4C

   /***************************************
   *   640 pix @ 30 fps   -------  CBR    *
   ***************************************/

   unsigned int   v640_30fps_opt_size_I;     // 0x50   AJ_mvrSetVGAOptSize() [I]
   unsigned int   v640_30fps_opt_size_P;     // 0x54   AJ_mvrSetVGAOptSize() [P]
   unsigned int   v640_30fps_D_H;            // 0x58   AJ_mvrSetD_VGA() --> 0x6C
   unsigned int   v640_30fps_D_L;            // 0x5C   AJ_mvrSetD_VGA() --> 0x70
   unsigned int   v640_30fps_kinda_counter;  // 0x60

   /***************************************
   *   640 pix @ 25 fps   -------  CBR    *
   ***************************************/

   unsigned int   v640_25fps_opt_size_I;     // 0x64   AJ_mvrSetVGAOptSize() [I]
   unsigned int   v640_25fps_opt_size_P;     // 0x68   AJ_mvrSetVGAOptSize() [P]
   unsigned int   v640_25fps_D_H;            // 0x6C   AJ_mvrSetD_VGA() <-- 0x58
   unsigned int   v640_25fps_D_L;            // 0x70   AJ_mvrSetD_VGA() <-- 0x70
   unsigned int   v640_25fps_kinda_counter;  // 0x74


   /***************************************/
   /***************************************/
   /***************************************/


   unsigned int   DefQScale;            // 0x78  AJ_Movie_CompressionRateAdjuster.c
   unsigned int   IniQScale;            // 0x7C  AJ_Movie_CompressionRateAdjuster.c
   int            actual_qscale_maybe;  // 0x80  inited #0x8000_0000
    				        //       in AJ_Movie_CompressionRateAdjuster.c

   unsigned int   _IOptSize;            // 0x84   XXX OPT XXX
   unsigned int   _POptSize;            // 0x88   XXX OPT XXX

   unsigned int   IOptSize2;            // 0x8C  AJ_MovieCompression_setup_Gop_size_Qscale()
   unsigned int   POptSize2;            // 0x90  AJ_MovieCompression_setup_Gop_size_Qscale()
   unsigned int   GopSize;              // 0x94  AJ_MovieCompression_setup_Gop_size_Qscale()
   unsigned int   ABC_zone_NowIndex;    // 0x98  A, B or C Zone _NowIndex
   unsigned int   GopOpt_struct_ptr;    // 0x9C   &Struct [2,3,4,5,6] -> Copied into here

   unsigned int   D1_DH;                // 0xA0  XXX GOPT XXX
   unsigned int   D2_DL;     		// 0xA4  XXX GOPT XXX
   unsigned int   kinda_counter;

	uint32_t		fullhd_30fps_gop_opt_0; // 0xac
	uint32_t		fullhd_30fps_gop_opt_1; // 0xb0
	uint32_t		fullhd_30fps_gop_opt_2; // 0xb4
	uint32_t		fullhd_30fps_gop_opt_3; // 0xb8
	uint32_t		fullhd_30fps_gop_opt_4; // 0xbc
	uint32_t		fullhd_25fps_gop_opt_0; // 0xc0
	uint32_t		fullhd_25fps_gop_opt_1; // 0xc4
	uint32_t		fullhd_25fps_gop_opt_2; // 0xc8
	uint32_t		fullhd_25fps_gop_opt_3; // 0xcc
	uint32_t		fullhd_25fps_gop_opt_4; // 0xd0
	uint32_t		fullhd_24fps_gop_opt_0; // 0xd4
	uint32_t		fullhd_24fps_gop_opt_1; // 0xd8
	uint32_t		fullhd_24fps_gop_opt_2; // 0xdc
	uint32_t		fullhd_24fps_gop_opt_3; // 0xe0
	uint32_t		fullhd_24fps_gop_opt_4; // 0xe4
	uint32_t		vga_30fps_gop_opt_0;    // 0xe8
	uint32_t		vga_30fps_gop_opt_1;    // 0xec
	uint32_t		vga_30fps_gop_opt_2;    // 0xf0
	uint32_t		vga_30fps_gop_opt_3;    // 0xf4
	uint32_t		vga_30fps_gop_opt_4;    // 0xf8
	uint32_t		vga_25fps_gop_opt_0;    // 0xfc
	uint32_t		vga_25fps_gop_opt_1;    // 0x100
	uint32_t		vga_25fps_gop_opt_2;    // 0x104
	uint32_t		vga_25fps_gop_opt_3;    // 0x108
	uint32_t		vga_25fps_gop_opt_4;    // 0x10c

} __attribute__((aligned,packed));

//~ SIZE_CHECK_STRUCT( mvr_config, 0x30 );

extern struct mvr_config mvr_config;


// This is from 5D2, not used here.
/*
 *
 * State information is in this structure.  A pointer to the global
 * object is at 0x1ee0.  It is of size 0x1b4.
 *
 * The state object is in 0x68a4.
 */
/*struct mvr_struct
{
	const char *		type;	 // "MovieRecorder" off 0
	uint32_t		off_0x04;
	uint32_t		task;	// off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint32_t		off_0x2c;
	uint32_t		off_0x30;
	uint32_t		off_0x34;
	uint32_t		off_0x38;
	uint32_t		off_0x3c;
	uint32_t		off_0x40;
	uint32_t		off_0x44;
	uint32_t		off_0x48;
	uint32_t		off_0x4c;
	uint32_t		off_0x50;
	uint32_t		off_0x54;
	uint32_t		off_0x58;
	uint32_t		off_0x5c;
	uint32_t		off_0x60;
	uint32_t		off_0x64;
	uint32_t		off_0x68;
	uint32_t		off_0x6c;
	uint32_t		off_0x70;
	uint32_t		off_0x74;
	uint32_t		off_0x78;
	uint32_t		off_0x7c;
	uint32_t		off_0x80;
	uint32_t		off_0x84;
	uint32_t		off_0x88;
	uint32_t		off_0x8c;
	uint32_t		off_0x90;
	uint32_t		off_0x94;
	uint32_t		off_0x98;
	uint32_t		off_0x9c;
	uint32_t		off_0xa0;
	uint32_t		off_0xa4;
	uint32_t		off_0xa8;
	uint32_t		off_0xac;
	uint32_t		off_0xb0;
	uint32_t		off_0xb4;
	uint32_t		off_0xb8;
	uint32_t		off_0xbc;
	uint32_t		off_0xc0;
	uint32_t		off_0xc4;
	uint32_t		off_0xc8;
	uint32_t		off_0xcc;
	uint32_t		off_0xd0;
	uint32_t		off_0xd4;
	uint32_t		off_0xd8;
	uint32_t		off_0xdc;
	uint32_t		off_0xe0;
	uint32_t		off_0xe4;
	uint32_t		off_0xe8;
	uint32_t		off_0xec;
	uint32_t		off_0xf0;
	uint32_t		off_0xf4;
	uint32_t		off_0xf8;
	uint32_t		off_0xfc;
	uint32_t		off_0x100;
	uint32_t		off_0x104;
	uint32_t		off_0x108;
	uint32_t		off_0x10c;
	uint32_t		off_0x110;
	uint32_t		off_0x114;
	uint32_t		off_0x118;
	uint32_t		off_0x11c;
	uint32_t		off_0x120;
	uint32_t		off_0x124;
	uint32_t		off_0x128;
	uint32_t		off_0x12c;
	uint32_t		off_0x130;
	uint32_t		off_0x134;
	uint32_t		off_0x138;
	uint32_t		off_0x13c;
	uint32_t		is_vga;	// 0==1920, 1==640 off_0x140;
	uint32_t		off_0x144;
	uint32_t		off_0x148;
	uint32_t		fps;		// 30, off_0x14c;
	uint32_t		width;		// off_0x150;
	uint32_t		height;		// off_0x154;
	uint32_t		audio_rec;	// off_0x158;
	uint32_t		auido_channels;	// 2 or 0, off_0x15c;
	uint32_t		audio_rate;	// 44100 or 0, off_0x160;
	uint32_t		off_0x164;
	uint32_t		off_0x168;
	uint32_t		off_0x16c;
	uint32_t		off_0x170;
	uint32_t		off_0x174;
	uint32_t		off_0x178;
	uint32_t		off_0x17c;
	uint32_t		off_0x180;
	uint32_t		off_0x184;
	uint32_t		off_0x188;
	uint32_t		off_0x18c;
	uint32_t		bit_rate; // off_0x190;
	uint32_t		off_0x194;
	uint32_t		off_0x198;
	uint32_t		off_0x19c;
	uint32_t		off_0x1a0;
	uint32_t		off_0x1a4;
	uint32_t		off_0x1a8;
	uint32_t		off_0x1ac;
	uint32_t		off_0x1b0;
	uint32_t		off_0x1b4;
	uint32_t		off_0x1b8;
	uint32_t		off_0x1bc;
	uint32_t		off_0x1c0;
	uint32_t		off_0x1c4;
	uint32_t		off_0x1c8;
	uint32_t		off_0x1cc;
	uint32_t		off_0x1d0;
	uint32_t		off_0x1d4;
	uint32_t		off_0x1d8;
	uint32_t		off_0x1dc;
	uint32_t		off_0x1e0;
	uint32_t		off_0x1e4;
	uint32_t		off_0x1e8;
	uint32_t		off_0x1ec;
	uint32_t		off_0x1f0;
	uint32_t		off_0x1f4;
	uint32_t		off_0x1f8;
	uint32_t		off_0x1fc;
};

SIZE_CHECK_STRUCT( mvr_struct, 512 );*/

#endif /* __PLATFORM_MVR_H__ */
