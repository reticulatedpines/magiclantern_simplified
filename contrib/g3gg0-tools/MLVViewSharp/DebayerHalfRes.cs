using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using pixelType = System.Byte;
using MLVViewSharp;

namespace mlv_view_sharp
{
    public class DebayerHalfRes : DebayerBase, Debayer
    {
        private int Downscale = 1;
  
        public DebayerHalfRes(int downscale)
        {
            Downscale = Math.Max(1, downscale);
        }

        public void Process(ushort[,] pixelData, pixelType[, ,] rgbData)
        {
            if (LookupTablesDirty)
            {
                CreateLookupTable();
                CreateConversionMatrix();
            }

            Matrix rgbInMatrix = new Matrix(3, 1);

            for (int y = 0; y < pixelData.GetLength(0); y += Downscale * 2)
            {
                for (int x = 0; x < pixelData.GetLength(1); x += Downscale * 2)
                {
                    /* assume RGRGRG and GBGBGB lines */
                    rgbInMatrix[0, 0] = (decimal)PixelLookupTable[pixelData[y, x]];
                    rgbInMatrix[1, 0] = (decimal)PixelLookupTable[pixelData[y, x + 1]];
                    rgbInMatrix[2, 0] = (decimal)PixelLookupTable[pixelData[y + 1, x + 1]];

                    /* apply transformation matrix */
                    Matrix rgbOutMatrix = CamToRgbMatrix * rgbInMatrix;

                    for (int channel = 0; channel < 3; channel++)
                    {
                        for (int pixelX = 0; pixelX < Downscale; pixelX++)
                        {
                            for (int pixelY = 0; pixelY < Downscale; pixelY++)
                            {
                                decimal value = rgbOutMatrix[channel, 0] * WhiteBalance[channel, 0];

                                pixelType data = (pixelType)Math.Max(0, Math.Min(255, value));

                                rgbData[y + pixelY * 2 + 0, x + pixelX * 2 + 0, channel] = data;
                                rgbData[y + pixelY * 2 + 0, x + pixelX * 2 + 1, channel] = data;
                                rgbData[y + pixelY * 2 + 1, x + pixelX * 2 + 0, channel] = data;
                                rgbData[y + pixelY * 2 + 1, x + pixelX * 2 + 1, channel] = data;
                            }
                        }
                    }
                }
            }
        }
    }
}
