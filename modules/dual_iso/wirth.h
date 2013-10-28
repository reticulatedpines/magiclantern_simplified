
/*
 * Algorithm from N. Wirth's book, implementation by N. Devillard.
 * This code in public domain.
 * 
 * Source: http://ndevilla.free.fr/median/median/
 * Modified to have both int and unsigned short versions.
 */


#define ELEM_SWAP_INT(a,b) { register int t=(a);(a)=(b);(b)=t; }
#define ELEM_SWAP_USHORT(a,b) { register unsigned short t=(a);(a)=(b);(b)=t; }


/*---------------------------------------------------------------------------
   Function :   kth_smallest()
   In       :   array of elements, # of elements in the array, rank k
   Out      :   one element
   Job      :   find the kth smallest element in the array
   Notice   :   use the median() macro defined below to get the median. 

                Reference:

                  Author: Wirth, Niklaus 
                   Title: Algorithms + data structures = programs 
               Publisher: Englewood Cliffs: Prentice-Hall, 1976 
    Physical description: 366 p. 
                  Series: Prentice-Hall Series in Automatic Computation 

 ---------------------------------------------------------------------------*/


static inline int kth_smallest_int(int a[], int n, int k)
{
    register int i,j,l,m ;
    register int x ;

    l=0 ; m=n-1 ;
    while (l<m) {
        x=a[k] ;
        i=l ;
        j=m ;
        do {
            while (a[i]<x) i++ ;
            while (x<a[j]) j-- ;
            if (i<=j) {
                ELEM_SWAP_INT(a[i],a[j]) ;
                i++ ; j-- ;
            }
        } while (i<=j) ;
        if (j<k) l=i ;
        if (k<i) m=j ;
    }
    return a[k] ;
}

static inline unsigned short kth_smallest_ushort(unsigned short a[], int n, int k)
{
    register int i,j,l,m ;
    register unsigned short x ;

    l=0 ; m=n-1 ;
    while (l<m) {
        x=a[k] ;
        i=l ;
        j=m ;
        do {
            while (a[i]<x) i++ ;
            while (x<a[j]) j-- ;
            if (i<=j) {
                ELEM_SWAP_USHORT(a[i],a[j]) ;
                i++ ; j-- ;
            }
        } while (i<=j) ;
        if (j<k) l=i ;
        if (k<i) m=j ;
    }
    return a[k] ;
}

#define median_int_wirth(a,n) kth_smallest_int(a,n,(((n)&1)?((n)/2):(((n)/2)-1)))
#define median_ushort_wirth(a,n) kth_smallest_ushort(a,n,(((n)&1)?((n)/2):(((n)/2)-1)))
