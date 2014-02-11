#ifndef AF_PATTERNS_H_
#define AF_PATTERNS_H_

// some definitions from main.h 
#define IS_EOL(item) (item->pattern == AF_PATTERN_END_OF_LIST)
#define END_OF_LIST  {AF_PATTERN_END_OF_LIST, 0, 0, 0, 0, 0 }
#define TRUE  1
// done 

// AF patterns, from 400plus's main.h (codes are slightly different)
// These might be camera-specific, but codes are the same on 550D and 60D
// [5] Values for "af_point" (can be ORed together to form patterns)

#if defined(CONFIG_50D) || defined(CONFIG_5D2) || defined(CONFIG_40D)
#define AF_POINT_C  0x01 // Center
#define AF_POINT_T  0x02 // Top
#define AF_POINT_B  0x04 // Bottom
#define AF_POINT_TL 0x08 // Top-left
#define AF_POINT_TR 0x10 // Top-right
#define AF_POINT_BL 0x20 // Bottom-left
#define AF_POINT_BR 0x40 // Bottom-right
#define AF_POINT_L  0x80 // Left
#define AF_POINT_R  0x100 // Right

#elif defined(CONFIG_7D)        // 7D focus points
/*
          TT
   TLL TL T  TR TRR
LLL LL L  C  R  RR  RRR
   BLL BL B  BR  BRR
          BB
*/
#define AF_POINT_C    0x44a00 // Center
#define AF_POINT_T    0x42a00 // Top
#define AF_POINT_TT   0x40a00 // Top
#define AF_POINT_B    0x46a00 // Bottom
#define AF_POINT_BB   0x48a00 // Bottom
#define AF_POINT_TL   0x42800 // Top-left
#define AF_POINT_TLL  0x42600 // Top-left-left
#define AF_POINT_TR   0x42c00 // Top-right
#define AF_POINT_TRR  0x42e00 // Top-right-right
#define AF_POINT_BL   0x46800 // Bottom-left
#define AF_POINT_BLL  0x46600 // Bottom-left-left
#define AF_POINT_BR   0x46c00 // Bottom-right
#define AF_POINT_BRR  0x46e00 // Bottom-right-right
#define AF_POINT_L    0x44800 // Left
#define AF_POINT_LL   0x44600 // Left-left
#define AF_POINT_LLL  0x44400 // Left-left-left
#define AF_POINT_R    0x44c00 // Right
#define AF_POINT_RR   0x44e00 // Right-right
#define AF_POINT_RRR  0x45000 // Right-right-right

#elif defined(CONFIG_6D)
+/*
+          T
+    TL         TR
+L   CL    C    CR    R
+    BL         BR
+          B
+*/
#define AF_POINT_C  0x4A00 // Center
#define AF_POINT_T  0x0A00 // Top
#define AF_POINT_B  0x8A00 // Bottom
#define AF_POINT_TL 0x2800 // Top-left
#define AF_POINT_TR 0x2C00 // Top-right
#define AF_POINT_BL 0x6800 // Bottom-left
#define AF_POINT_BR 0x6C00 // Bottom-right
#define AF_POINT_L  0x4600 // Left
#define AF_POINT_R  0x4E00 // Right
#define AF_POINT_CL 0x4800 // Center-left
#define AF_POINT_CR 0x4C00 // Center-right


#else
#define AF_POINT_C  0x0100 // Center
#define AF_POINT_T  0x0200 // Top
#define AF_POINT_B  0x0400 // Bottom
#define AF_POINT_TL 0x0800 // Top-left
#define AF_POINT_TR 0x1000 // Top-right
#define AF_POINT_BL 0x2000 // Bottom-left
#define AF_POINT_BR 0x4000 // Bottom-right
#define AF_POINT_L  0x8000 // Left
#define AF_POINT_R  0x0001 // Right
#endif

#define AF_PATTERN_NONE            0

#if defined(CONFIG_6D)
#define AF_PATTERN_CENTERLEFT      AF_POINT_CL
#define AF_PATTERN_CENTERRIGHT      AF_POINT_CR
#endif

#define AF_PATTERN_CENTER          AF_POINT_C
#if defined(CONFIG_6D)
#define AF_PATTERN_SQUARE          AF_POINT_C | AF_POINT_TL | AF_POINT_TR | AF_POINT_BL | AF_POINT_BR | AF_POINT_CL | AF_POINT_CR
#else
#define AF_PATTERN_SQUARE          AF_POINT_C | AF_POINT_TL | AF_POINT_TR | AF_POINT_BL | AF_POINT_BR
#endif

#define AF_PATTERN_TOP             AF_POINT_T
#define AF_PATTERN_TOPTRIANGLE     AF_POINT_T | AF_POINT_TL | AF_POINT_TR
#define AF_PATTERN_TOPDIAMOND      AF_POINT_T | AF_POINT_TL | AF_POINT_TR | AF_POINT_C
#if defined(CONFIG_6D)
#define AF_PATTERN_TOPHALF         AF_POINT_T | AF_POINT_TL | AF_POINT_TR | AF_POINT_C | AF_POINT_L | AF_POINT_R | AF_POINT_CL | AF_POINT_CR
#else
#define AF_PATTERN_TOPHALF         AF_POINT_T | AF_POINT_TL | AF_POINT_TR | AF_POINT_C | AF_POINT_L | AF_POINT_R
#endif

#define AF_PATTERN_BOTTOM          AF_POINT_B
#define AF_PATTERN_BOTTOMTRIANGLE  AF_POINT_B | AF_POINT_BL | AF_POINT_BR
#define AF_PATTERN_BOTTOMDIAMOND   AF_POINT_B | AF_POINT_BL | AF_POINT_BR | AF_POINT_C
#if defined(CONFIG_6D)
#define AF_PATTERN_BOTTOMHALF      AF_POINT_B | AF_POINT_BL | AF_POINT_BR | AF_POINT_C | AF_POINT_L | AF_POINT_R | AF_POINT_CL | AF_POINT_CR
#else
#define AF_PATTERN_BOTTOMHALF      AF_POINT_B | AF_POINT_BL | AF_POINT_BR | AF_POINT_C | AF_POINT_L | AF_POINT_R 
#endif

#define AF_PATTERN_TOPLEFT         AF_POINT_TL
#define AF_PATTERN_TOPRIGHT        AF_POINT_TR
#define AF_PATTERN_BOTTOMLEFT      AF_POINT_BL
#define AF_PATTERN_BOTTOMRIGHT     AF_POINT_BR

#define AF_PATTERN_LEFT            AF_POINT_L
#if defined(CONFIG_6D)
#define AF_PATTERN_LEFTTRIANGLE    AF_POINT_L | AF_POINT_TL | AF_POINT_BL | AF_POINT_CL | AF_POINT_CR
#define AF_PATTERN_LEFTDIAMOND     AF_POINT_L | AF_POINT_TL | AF_POINT_BL | AF_POINT_C | AF_POINT_CL | AF_POINT_CR
#define AF_PATTERN_LEFTHALF        AF_POINT_L | AF_POINT_TL | AF_POINT_BL | AF_POINT_C | AF_POINT_T | AF_POINT_B | AF_POINT_CL | AF_POINT_CR
#else
#define AF_PATTERN_LEFTTRIANGLE    AF_POINT_L | AF_POINT_TL | AF_POINT_BL
#define AF_PATTERN_LEFTDIAMOND     AF_POINT_L | AF_POINT_TL | AF_POINT_BL | AF_POINT_C
#define AF_PATTERN_LEFTHALF        AF_POINT_L | AF_POINT_TL | AF_POINT_BL | AF_POINT_C | AF_POINT_T | AF_POINT_B
#endif

#define AF_PATTERN_RIGHT           AF_POINT_R
#if defined(CONFIG_6D)
#define AF_PATTERN_RIGHTTRIANGLE   AF_POINT_R | AF_POINT_TR | AF_POINT_BR | AF_POINT_CL | AF_POINT_CR
#define AF_PATTERN_RIGHTDIAMOND    AF_POINT_R | AF_POINT_TR | AF_POINT_BR | AF_POINT_C | AF_POINT_CL | AF_POINT_CR
#define AF_PATTERN_RIGHTHALF       AF_POINT_R | AF_POINT_TR | AF_POINT_BR | AF_POINT_C | AF_POINT_T | AF_POINT_B | AF_POINT_CL | AF_POINT_CR
#else
#define AF_PATTERN_RIGHTTRIANGLE   AF_POINT_R | AF_POINT_TR | AF_POINT_BR
#define AF_PATTERN_RIGHTDIAMOND    AF_POINT_R | AF_POINT_TR | AF_POINT_BR | AF_POINT_C
#define AF_PATTERN_RIGHTHALF       AF_POINT_R | AF_POINT_TR | AF_POINT_BR | AF_POINT_C | AF_POINT_T | AF_POINT_B
#endif

#if defined(CONFIG_6D)
#define AF_PATTERN_HLINE           AF_POINT_C | AF_POINT_L | AF_POINT_R | AF_POINT_CL | AF_POINT_CR
#else
#define AF_PATTERN_HLINE           AF_POINT_C | AF_POINT_L | AF_POINT_R
#endif
#define AF_PATTERN_VLINE           AF_POINT_C | AF_POINT_T | AF_POINT_B

#if defined(CONFIG_6D)
#define AF_PATTERN_ALL             AF_POINT_C | AF_POINT_T | AF_POINT_B | AF_POINT_TL | AF_POINT_TR | AF_POINT_BL | AF_POINT_BR | AF_POINT_L | AF_POINT_R | AF_POINT_CL | AF_POINT_CR
#else
#define AF_PATTERN_ALL             AF_POINT_C | AF_POINT_T | AF_POINT_B | AF_POINT_TL | AF_POINT_TR | AF_POINT_BL | AF_POINT_BR | AF_POINT_L | AF_POINT_R
#endif

#define AF_PATTERN_END_OF_LIST      -1

typedef struct {
    int pattern;
    int next_center;
    int next_top;
    int next_bottom;
    int next_left;
    int next_right;
} type_PATTERN_MAP_ITEM;

typedef enum {
    DIRECTION_CENTER,
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
} type_DIRECTION;

static void afp_enter ();

static void afp_center ();
static void afp_top ();
static void afp_bottom ();
static void afp_left ();
static void afp_right ();

#endif /* AF_PATTERNS_H_ */
