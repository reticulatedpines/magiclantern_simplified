// 50D AFMA constants

#define PROP_AFMA 0x80010006

static int8_t afma_buf[0xE];
#define AFMA_MODE       afma_buf[0x7]
#define AFMA_PER_LENS   afma_buf[0xB]
#define AFMA_ALL_LENSES afma_buf[0xD]

/* from Max Chen:

here is what i found:

80010006  |  000e  |  8  |  mode  |  value of adjust by lens  |  value of adjust all by same amount  |  0  |  0

mode                                          value
----------------------------------------------------------------
0:disable                                        0
1:adjust all by same amount                1000000
2:adjust by lens                           2000000

amount  value of adjust by lens       value ofadjust all by same amount
----------------------------------------------------------------------------------------------------------------------------
-20                     ec000706       ec00
-19                     ed000706       ed00
....                      .....
               .....
-1                       ff000706      ff00
0                        706             00
1                        1000706        100
....                       ....
               ....
19                      13000706        1300
20                       14000706       1400

and ,if you change the value of adjust by lens,the value of adjust all
by same amount will add 0x02,no matter what the value of adjust by
lens is.
*/
