using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using pixelType = System.Byte;

namespace mlv_view_sharp
{
    public class DebayerHalfRes : Debayer
    {
        public int _BlackLevel = 0;
        public int _WhiteLevel = 0;
        public float _Brightness = 0;
        public float _Saturation = 0.0f;
        public float[] _CamMatrix = new float[] { 1f, 0f, 0f, 0f, 0.5f, 0f, 0f, 0f, 1f };

        private float[] PixelLookupTable = new float[16384];
        private bool PixelLookupTableDirty = true;

        private int Downscale = 1;

        public DebayerHalfRes(int downscale)
        {
            Downscale = Math.Max(1, downscale);
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
                float scal = (float)(pos - black) / range;

                scal *= _Brightness;

                PixelLookupTable[pos] = Math.Max(0, scal);
            }
        }

        public void Process(ushort[,] pixelData, pixelType[, ,] rgbData)
        {
            if (PixelLookupTableDirty)
            {
                CreateLookupTable();
            }

            float[] matrix = _CamMatrix;
            float[] rgbIn = new float[] { 0, 0, 0 };
            float[] rgbOut = new float[] { 0, 0, 0 };

            for (int y = 0; y < pixelData.GetLength(0); y += Downscale * 2)
            {
                for (int x = 0; x < pixelData.GetLength(1); x += Downscale * 2)
                {
                    /* assume RGRGRG and GBGBGB lines */
                    rgbIn[0] = PixelLookupTable[pixelData[y, x]];
                    rgbIn[1] = PixelLookupTable[pixelData[y, x + 1]];
                    rgbIn[2] = PixelLookupTable[pixelData[y + 1, x + 1]];
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
                        for (int pixelX = 0; pixelX < Downscale; pixelX++)
                        {
                            for (int pixelY = 0; pixelY < Downscale; pixelY++)
                            {
                                pixelType data = (pixelType)Math.Max(0, Math.Min(255, rgbOut[channel]));
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

        #region Debayer Member

        public int BlackLevel
        {
            get
            {
                PixelLookupTableDirty = true;
                return _BlackLevel;
            }
            set
            {
                _BlackLevel = value;
            }
        }

        public int WhiteLevel
        {
            get
            {
                PixelLookupTableDirty = true;
                return _WhiteLevel;
            }
            set
            {
                _WhiteLevel = value;
            }
        }

        public float Brightness
        {
            get
            {
                PixelLookupTableDirty = true;
                return _Brightness;
            }
            set
            {
                _Brightness = value;
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
