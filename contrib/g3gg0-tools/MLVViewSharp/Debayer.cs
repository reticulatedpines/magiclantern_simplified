using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using pixelType = System.Byte;

namespace mlv_view_sharp
{
    public interface Debayer
    {
        int BlackLevel { get; set; }
        int WhiteLevel { get; set; }

        float Brightness { get; set; }
        float Saturation { get; set; }

        float[] CamMatrix { get; set; }
        float[] WhiteBalance { get; set; }

        void Process(ushort[,] pixelData, pixelType[, ,] rgbData);
    }
}
