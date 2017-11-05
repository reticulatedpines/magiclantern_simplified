#ifndef EOS_UTILS_H
#define EOS_UTILS_H

inline static void reverse_bytes_order(uint8_t* buf, int count)
{
    short* buf16 = (short*) buf;
    int i;
    for (i = 0; i < count/2; i++)
    {
        short x = buf16[i];
        buf[2*i+1] = x;
        buf[2*i] = x >> 8;
    }
}


#endif

