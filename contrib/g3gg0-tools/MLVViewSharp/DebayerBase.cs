using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using mlv_view_sharp;

namespace MLVViewSharp
{
    public abstract class DebayerBase
    {
        protected int _BlackLevel = 0;
        protected int _WhiteLevel = 0;
        protected float _Brightness = 0;
        protected int _Overexposed = 0;
        protected int _Underexposed = 0;
        protected float _Saturation = 0.0f;

        protected float[] _CamMatrix = new float[] { 1f, 0f, 0f, 0f, 0.5f, 0f, 0f, 0f, 1f };
        protected Matrix CamToRgbMatrix = new Matrix(3, 3);
        protected Matrix WhiteBalance = new Matrix(3, 1);
        

        protected float[] PixelLookupTable = new float[16384];
        protected bool LookupTablesDirty = true;


        protected void CreateConversionMatrix()
        {
            Matrix rgbToXYZ = new Matrix(3, 3);
            Matrix xyzToCam = new Matrix(3, 3);

            WhiteBalance[0, 0] = 2;
            WhiteBalance[1, 0] = 1;
            WhiteBalance[2, 0] = 2;

            /* this is the RGB --> XYZ conversion matrix */
            rgbToXYZ[0, 0] = 0.4124564m;
            rgbToXYZ[0, 1] = 0.3575761m;
            rgbToXYZ[0, 2] = 0.1804375m;
            rgbToXYZ[1, 0] = 0.2126729m;
            rgbToXYZ[1, 1] = 0.7151522m;
            rgbToXYZ[1, 2] = 0.0721750m;
            rgbToXYZ[2, 0] = 0.0193339m;
            rgbToXYZ[2, 1] = 0.1191920m;
            rgbToXYZ[2, 2] = 0.9503041m;

            /* fill supplied camera matrix, which is XYZ --> camera */
            for (int row = 0; row < xyzToCam.RowCount; row++)
            {
                for (int col = 0; col < xyzToCam.ColumnCount; col++)
                {
                    xyzToCam[row, col] = (decimal)_CamMatrix[col * 3 + row];
                }
            }

            Matrix rgbToCam = rgbToXYZ + xyzToCam;

            /* normalize rows as described in http://users.soe.ucsc.edu/~rcsumner/papers/RAWguide.pdf 
             * the values in a row have to sum up to 1.0 to ensure better color balance.
             */
            for (int row = 0; row < rgbToCam.RowCount; row++)
            {
                decimal sum = 0;
                for (int col = 0; col < rgbToCam.ColumnCount; col++)
                {
                    sum += rgbToCam[row, col];
                }
                for (int col = 0; col < rgbToCam.ColumnCount; col++)
                {
                    rgbToCam[row, col] /= sum;
                }
            }

            CamToRgbMatrix = rgbToCam.Inverse();
        }

        protected void CreateLookupTable()
        {
            LookupTablesDirty = false;

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


        #region Debayer Member

        public int BlackLevel
        {
            get
            {
                return _BlackLevel;
            }
            set
            {
                _BlackLevel = value;
                LookupTablesDirty = true;
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
                _WhiteLevel = value;
                LookupTablesDirty = true;
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
                _Brightness = value;
                LookupTablesDirty = true;
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
                _CamMatrix = value;
                LookupTablesDirty = true;
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
                LookupTablesDirty = true;
            }
        }


        #endregion
    }
}
