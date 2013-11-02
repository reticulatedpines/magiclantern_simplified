using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using pixelType = System.Byte;

namespace mlv_view_sharp
{
    public class DebayerBilinear : Debayer
    {
        public int _BlackLevel = 0;
        public int _WhiteLevel = 0;
        public float _Brightness = 0;
        public int _Overexposed = 0;
        public int _Underexposed = 0;
        public float _Saturation = 0.0f;
        public float[] _CamMatrix = new float[] { 1f, 0f, 0f, 0f, 0.5f, 0f, 0f, 0f, 1f };

        private float[] PixelLookupTable = new float[16384];
        private bool PixelLookupTableDirty = true;

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
        }

        private void CreateLookupTable()
        {
            PixelLookupTableDirty = false;

            int black = _BlackLevel;
            int white = _WhiteLevel;
            float brightness = _Brightness;

            for (int pos = 0; pos < PixelLookupTable.Length; pos++)
            {
                /* subtract black level and scale to 0.0 to 1.0 */
                int range = white - black;
                float scal = (float)Math.Max(0, pos - black) / range;

                scal *= _Brightness;

                /* now scale to 0-255 */
                PixelLookupTable[pos] = Math.Max(0, scal) * 255;
            }
        }

        public void Process(ushort[,] pixelData, pixelType[, ,] rgbData)
        {
            Overexposed = 0;
            Underexposed = 0;

            if (PixelLookupTableDirty)
            {
                CreateLookupTable();
            }

            float[] matrix = _CamMatrix;
            float[] rgbIn = new float[] { 0, 0, 0 };
            float[] rgbOut = new float[] { 0, 0, 0 };

            for (int y = 2; y < pixelData.GetLength(0) - 2; y++)
            {
                for (int x = 2; x < pixelData.GetLength(1) - 2; x++)
                {
                    /* assume RGRGRG and GBGBGB lines */
                    rgbIn[0] = PixelLookupTable[(int)GetPixel(pixelData, x, y, 0)];
                    rgbIn[1] = PixelLookupTable[(int)GetPixel(pixelData, x, y, 1)];
                    rgbIn[2] = PixelLookupTable[(int)GetPixel(pixelData, x, y, 2)];
                    rgbOut[0] = 0;
                    rgbOut[1] = 0;
                    rgbOut[2] = 0;

                    /* apply transformation matrix */
                    for (int col = 0; col < 3; col++)
                    {
                        for (int row = 0; row < 3; row++)
                        {
                            rgbOut[row] += rgbIn[col] * matrix[col * 3 + row];
                        }
                    }

                    for (int channel = 0; channel < 3; channel++)
                    {
                        rgbData[y, x, channel] = (pixelType)Math.Max(0, Math.Min(255, rgbOut[channel]));
                    }
                }
            }
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

        #region Debayer Member

        public int BlackLevel
        {
            get
            {
                return _BlackLevel;
            }
            set
            {
                PixelLookupTableDirty = true;
                _BlackLevel = value;
            }
        }

        public int WhiteLevel
        {
            get
            {
                return _WhiteLevel;
            }
            set
            {
                PixelLookupTableDirty = true;
                _WhiteLevel = value;
            }
        }

        public float Brightness
        {
            get
            {
                return _Brightness;
            }
            set
            {
                PixelLookupTableDirty = true;
                _Brightness = value;
            }
        }

        public int Overexposed
        {
            get
            {
                return _Overexposed;
            }
            set
            {
                _Overexposed = value;
            }
        }

        public int Underexposed
        {
            get
            {
                return _Underexposed;
            }
            set
            {
                _Underexposed = value;
            }
        }

        public float[] CamMatrix
        {
            get
            {
                return _CamMatrix;
            }
            set
            {
                //_CamMatrix = value;
            }
        }

        public float Saturation
        {
            get
            {
                return _Saturation;
            }
            set
            {
                _Saturation = value;
                _CamMatrix = new float[] { 1f, -_Saturation, -_Saturation, -_Saturation, 0.5f, -_Saturation, -_Saturation, -_Saturation, 1f };
            }
        }


        #endregion
    }
}
