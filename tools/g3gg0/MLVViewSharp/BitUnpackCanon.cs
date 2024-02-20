using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace mlv_view_sharp
{
    public class BitUnpackCanon : BitUnpack
    {
        private int _BitsPerPixel = 14;

        private unsafe static ushort BitExtract(byte* src, int offset, int position, int depth)
        {
            int value = 0;
            int src_pos = position * depth / 16;
            int bits_to_left = ((depth * position) - (16 * src_pos)) % 16;
            int shift_right = 16 - depth - bits_to_left;

            int byteNum = offset + 2 * src_pos;

            value = (int)src[byteNum] | (((int)src[byteNum + 1]) << 8);

            if (shift_right >= 0)
            {
                value >>= shift_right;
            }
            else
            {
                int val2 = (int)src[byteNum + 2] | (((int)src[byteNum + 3]) << 8);

                value <<= -shift_right;
                value |= val2 >> (16 + shift_right);
            }

            value &= (1 << depth) - 1;

            return (ushort)value;
        }

        public unsafe void Process(byte[] buffer, int startOffset, int length, ushort[,] dest)
        {
            int yRes = dest.GetLength(0);
            int xRes = dest.GetLength(1);

            int bpp = _BitsPerPixel;
            int pitch = (xRes * bpp) / 8;
            int pos = startOffset;

            if ((xRes * yRes * bpp / 8) > length)
            {
                throw new ArgumentException();
            }

            fixed (byte* pSrc = buffer)
            {
                for (int y = 0; y < yRes; y++)
                {
                    for (int x = 0; x < xRes; x++)
                    {
                        dest[y, x] = BitExtract(pSrc, pos, x, bpp);
                    }
                    pos += pitch;
                }
            }
        }

        public int BitsPerPixel
        {
            get
            {
                return _BitsPerPixel;
            }
            set
            {
                _BitsPerPixel = value;
            }
        }
    }
}
