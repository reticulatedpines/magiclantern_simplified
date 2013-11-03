using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using pixelType = System.Byte;
using MLVViewSharp;

namespace mlv_view_sharp
{
    public class DebayerBilinear : DebayerBase, Debayer
    {
        public float[,] MatrixCenter = new float[,] 
             { 
                 { 0.00f, 0.00f, 0.00f }, 
                 { 0.00f, 1.00f, 0.00f }, 
                 { 0.00f, 0.00f, 0.00f } 
             };
        public float[,] MatrixCross = new float[,] 
             { 
                 { 0.25f, 0.00f, 0.25f }, 
                 { 0.00f, 0.00f, 0.00f }, 
                 { 0.25f, 0.00f, 0.25f } 
             };
        public float[,] MatrixCrossMid = new float[,] 
             { 
                 { 0.20f, 0.00f, 0.20f }, 
                 { 0.00f, 0.20f, 0.00f }, 
                 { 0.20f, 0.00f, 0.20f } 
             };
        public float[,] MatrixPlus = new float[,] 
             { 
                 { 0.00f, 0.25f, 0.00f }, 
                 { 0.25f, 0.00f, 0.25f }, 
                 { 0.00f, 0.25f, 0.00f } 
             };
        public float[,] MatrixSide = new float[,] 
             { 
                 { 0.00f, 0.00f, 0.00f }, 
                 { 0.50f, 0.00f, 0.50f }, 
                 { 0.00f, 0.00f, 0.00f } 
             };
        public float[,] MatrixTop = new float[,] 
             { 
                 { 0.00f, 0.50f, 0.00f }, 
                 { 0.00f, 0.00f, 0.00f }, 
                 { 0.00f, 0.50f, 0.00f } 
             };

        public float[, ,][,] BilinearMatrix = null;

        public DebayerBilinear()
        {
            BilinearMatrix = new[, ,]
            {
                /* R */
                {{MatrixCenter, MatrixSide}, {MatrixTop, MatrixCross}},
                /* G */
                {{MatrixPlus, MatrixCenter}, {MatrixCenter, MatrixPlus}},
                /* B */
                {{MatrixCross, MatrixTop}, {MatrixSide, MatrixCenter}},
            };

            WhiteBalanceMatrix[0, 0] = 2;
            WhiteBalanceMatrix[1, 0] = 1;
            WhiteBalanceMatrix[2, 0] = 2;
        }

        private float GetPixel(ushort[,] pixelData, int x, int y, int color)
        {
            float value = 0;

            float[,] matrix = BilinearMatrix[color, y % 2, x % 2];

            int hy = matrix.GetLength(0);
            int hx = matrix.GetLength(1);
            for (int dy = 0; dy < hy; dy++)
            {
                for (int dx = 0; dx < hx; dx++)
                {
                    int pixX = x + dx - (hx - 1) / 2;
                    int pixY = y + dy - (hy - 1) / 2;

                    value += matrix[dy, dx] * pixelData[pixY, pixX];
                }
            }

            return value;
        }

        public void Process(ushort[,] pixelData, pixelType[, ,] rgbData)
        {
            if (LookupTablesDirty)
            {
                CreateLookupTable();
                CreateConversionMatrix();
            }

            Matrix rgbInMatrix = new Matrix(3, 1);

            for (int y = 2; y < pixelData.GetLength(0) - 2; y++)
            {
                for (int x = 2; x < pixelData.GetLength(1) - 2; x++)
                {
                    /* assume RGRGRG and GBGBGB lines */
                    rgbInMatrix[0, 0] = PixelLookupTable[(int)GetPixel(pixelData, x, y, 0)];
                    rgbInMatrix[1, 0] = PixelLookupTable[(int)GetPixel(pixelData, x, y, 1)];
                    rgbInMatrix[2, 0] = PixelLookupTable[(int)GetPixel(pixelData, x, y, 2)];

                    if (UseCorrectionMatrices)
                    {
                        /* apply transformation matrix */
                        Matrix rgbOutMatrix = CamToRgbMatrix * rgbInMatrix;

                        for (int channel = 0; channel < 3; channel++)
                        {
                            float value = rgbInMatrix[channel, 0] * WhiteBalanceMatrix[channel, 0];
                            rgbData[y, x, channel] = (pixelType)Math.Max(0, Math.Min(255, value));
                        }
                    }
                    else
                    {
                        for (int channel = 0; channel < 3; channel++)
                        {
                            float value = rgbInMatrix[channel, 0];
                            rgbData[y, x, channel] = (pixelType)Math.Max(0, Math.Min(255, value));
                        }
                    }
                }
            }
        }
    }
}
