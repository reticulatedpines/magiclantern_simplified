using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using MLVViewSharp;

namespace mlv_view_sharp
{
    public class DebayerHalfRes : DebayerBase, Debayer
    {
        private int Downscale = 1;
  
        public DebayerHalfRes(int downscale)
        {
            Downscale = Math.Max(1, downscale);

            InitMatrices();
        }

        public void Process(ushort[,] pixelData, float[, ,] rgbData)
        {
            if (LookupTablesDirty)
            {
                CreateLookupTable();
                UpdateMatrix();
            }

            Matrix rgbInMatrix = new Matrix(3, 1);
            Matrix rgbOutMatrix = new Matrix(3, 1);

            for (int y = 0; y < pixelData.GetLength(0) - Downscale * 2; y += Downscale * 2)
            {
                for (int x = 0; x < pixelData.GetLength(1) - Downscale * 2; x += Downscale * 2)
                {
                    /* assume RGRGRG and GBGBGB lines */
                    rgbInMatrix[0] = (float)PixelLookupTable[pixelData[y, x]];
                    rgbInMatrix[1] = (float)PixelLookupTable[pixelData[y, x + 1]];
                    rgbInMatrix[2] = (float)PixelLookupTable[pixelData[y + 1, x + 1]];

                    /* apply transformation matrix */
                    rgbOutMatrix = CorrectionMatrices(rgbInMatrix);

                    for (int pixelY = 0; pixelY < Downscale; pixelY++)
                    {
                        for (int pixelX = 0; pixelX < Downscale; pixelX++)
                        {
                            int posX = x + pixelX * 2;
                            int posY = y + pixelY * 2;

                            for (int channel = 0; channel < 3; channel++)
                            {
                                float value = rgbOutMatrix[channel];

                                if (!UseCorrectionMatrices && (channel == 1))
                                {
                                    value /= 2;
                                }

                                float data = value;

                                rgbData[posY + 0, posX + 0, channel] = data;
                                rgbData[posY + 0, posX + 1, channel] = data;
                                rgbData[posY + 1, posX + 0, channel] = data;
                                rgbData[posY + 1, posX + 1, channel] = data;
                            }
                        }
                    }
                }
            }
        }
    }
}
