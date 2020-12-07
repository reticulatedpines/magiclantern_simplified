
typedef unsigned char u8;
typedef unsigned short u16;

extern u16 const crc16_table[256];

extern u16 crc16(u16 crc, const u8 *buffer, long len);

static inline u16 crc16_byte(u16 crc, const u8 data)
{
    return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}

