using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace mlv_view_sharp
{
    public interface BitUnpack
    {
        int BitsPerPixel { get; set; }
        void Process(byte[] buffer, int startOffset, int length, ushort[,] dest);
    }
}
