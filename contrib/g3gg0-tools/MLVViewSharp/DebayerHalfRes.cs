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

            WhiteBalanceMatrix[0, 0] = 2;
            WhiteBalanceMatrix[1, 0] = 1;
            WhiteBalanceMatrix[2, 0] = 2;
        }

        public void Process(ushort[,] pixelData, pixelType[, ,] rgbData)
        {
            if (LookupTablesDirty)
            {
                CreateLookupTable();
                CreateConversionMatrix();
            }

            Matrix rgbInMatrix = new Matrix(3, 1);

            for (int y = 0; y < pixelData.GetLength(0) - Downscale; y += Downscale * 2)
            {
                for (int x = 0; x < pixelData.GetLength(1) - Downscale; x += Downscale * 2)
                {
                    /* assume RGRGRG and GBGBGB lines */
                    rgbInMatrix[0, 0] = (float)PixelLookupTable[pixelData[y, x]];
                    rgbInMatrix[1, 0] = (float)PixelLookupTable[pixelData[y, x + 1]];
                    rgbInMatrix[2, 0] = (float)PixelLookupTable[pixelData[y + 1, x + 1]];

                    Matrix rgbOutMatrix = rgbInMatrix;

                    /* apply transformation matrix */
                    if (UseCorrectionMatrices)
                    {
                        rgbOutMatrix = CamToRgbMatrix * rgbInMatrix;
                    }

                    for (int pixelY = 0; pixelY < Downscale; pixelY++)
                    {
                        for (int pixelX = 0; pixelX < Downscale; pixelX++)
                        {
                            for (int channel = 0; channel < 3; channel++)
                            {
                                float value = rgbOutMatrix[channel, 0];

                                if (UseCorrectionMatrices)
                                {
                                    value *= WhiteBalanceMatrix[channel, 0];
                                }

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
