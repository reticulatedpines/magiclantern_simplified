using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using mlv_view_sharp;

using pixelType = System.Byte;

namespace MLVViewSharp
{
    public abstract class DebayerBase
    {
        protected int _BlackLevel = 0;
        protected int _WhiteLevel = 0;
        protected float _Brightness = 0;
        protected float _Saturation = 0.0f;
        protected float _ColorTemperature = 6500.0f;

        protected Matrix CamToXYZMatrix = new Matrix(3, 3);
        protected Matrix CamToRgbMatrix = new Matrix(3, 3);
        protected Matrix WhiteBalanceMatrix = new Matrix(3, 3);
        protected Matrix WhiteBalanceMatrixRaw = new Matrix(3, 3);

        protected float[] PixelLookupTable = new float[16384];
        protected bool LookupTablesDirty = true;
        
        public bool _HighlightRecovery = true;
        public bool _UseCorrectionMatrices = true;

        protected Matrix RGBToYCbCrMatrix = null;
        protected Matrix XYZD50toD65 = null;
        protected Matrix XYZBradford = null;
        protected Matrix RGBToXYZMatrix = null;
        

        protected void InitMatrices()
        {
            /* this is the RGB --> XYZ conversion matrix */
            RGBToXYZMatrix = new Matrix(new float[][]{ 
                new[] { 0.4124564f,  0.3575761f,  0.1804375f},
                new[] { 0.2126729f,  0.7151522f,  0.0721750f},
                new[] { 0.0193339f,  0.1191920f,  0.9503041f} });

            XYZD50toD65 = new Matrix(new float[][]{ 
                new[] { 0.9555766f, -0.0230393f,  0.0631636f}, 
                new[] {-0.0282895f,  1.0099416f,  0.0210077f}, 
                new[] { 0.0122982f, -0.0204830f,  1.3299098f} });

            XYZBradford = new Matrix(new float[][]{ 
                new[] { 0.8951000f,  0.2664000f, -0.1614000f}, 
                new[] {-0.7502000f,  1.7135000f,  0.0367000f}, 
                new[] { 0.0389000f, -0.0685000f,  1.0296000f} });

            RGBToYCbCrMatrix = new Matrix(new float[][]{ 
                new[] { 0.2990000f,  0.5870000f,  0.1140000f}, 
                new[] {-0.1687360f, -0.3312640f,  0.5000000f}, 
                new[] { 0.5000000f, -0.4186880f, -0.0813120f} });

            /* in raw space, balance out the green cast */
            WhiteBalanceMatrixRaw[0, 0] = 1;
            WhiteBalanceMatrixRaw[1, 1] = 1;
            WhiteBalanceMatrixRaw[2, 2] = 1;

            /* fine WB is set to 1 */
            WhiteBalanceMatrix[0, 0] = 1;
            WhiteBalanceMatrix[1, 1] = 1;
            WhiteBalanceMatrix[2, 2] = 1;
        }

        /* from http://www.brucelindbloom.com/index.html?ChromAdaptEval.html */
        protected Matrix KelvinToXYZ(float temp)
        {
            Matrix xyz = new Matrix(3, 1);
            double x = 0;
            double y = 0;
            if (temp < 4000 || temp > 7000)
            {
                throw new NotSupportedException();
            }

            x = -4.6070 * 1000000000.0 / Math.Pow(temp, 3) + 2.9678 * 1000000.0 / Math.Pow(temp, 2) + 0.09911 * 1000.0 / temp + 0.244063;
            y = -3 * Math.Pow(x, 2) + 2.870 * x - 0.275;

            xyz[0] = (float)(x / y);
            xyz[1] = 1.0f;
            xyz[2] = (float)((1 - x - y) / y);

            return xyz;
        }

        protected pixelType ToPixelValue(float value)
        {
            /* quick check, hopefully faster than Math functions (?) */
            if (value >= 0 && value <= 255)
            {
                return (pixelType)value;
            }

            return (pixelType)Math.Max(0, Math.Min(255, value));
        }

        /* found on the web: http://www.johndcook.com/csharp_erf.html (public domain) */
        protected static float Erf(float x)
        {
            // constants
            float a1 = 0.254829592f;
            float a2 = -0.284496736f;
            float a3 = 1.421413741f;
            float a4 = -1.453152027f;
            float a5 = 1.061405429f;
            float p = 0.3275911f;

            // Save the sign of x
            int sign = 1;
            if (x < 0)
                sign = -1;
            x = Math.Abs(x);

            // A&S formula 7.1.26
            float t = 1.0f / (1.0f + p * x);
            float y = 1.0f - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * (float)Math.Exp(-x * x);

            return sign * y;
        }

        protected void UpdateMatrix()
        {
            /* convert XYZ into a cone response domain, scale and back again (see http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html) */

            /* get the XYZ --> cone reference whites for requested temperatures */
            Matrix dst = XYZBradford * KelvinToXYZ(ColorTemperature);
            Matrix src = XYZBradford * KelvinToXYZ(5000);

            /* scale coordinates in cone color space */
            Matrix xyzScale = new Matrix(3, 3);
            xyzScale[0, 0] = dst[0] / src[0];
            xyzScale[1, 1] = dst[1] / src[1];
            xyzScale[2, 2] = dst[2] / src[2];

            /* finally scale colors */
            Matrix xyzKelvinWb = XYZBradford.Inverse() * xyzScale * XYZBradford;

            /* now combine the whole thing to get RAW --> RAW-WB --> XYZ --> Kelvin-WB --> XYZ --> (s)RGB --> RGB-WB */
            CamToRgbMatrix = WhiteBalanceMatrix * RGBToXYZMatrix.Inverse() * xyzKelvinWb * CamToXYZMatrix.Inverse() * WhiteBalanceMatrixRaw;
        }

        internal Matrix CorrectionMatrices(Matrix inMatrix)
        {
            Matrix outMatrix = inMatrix;

            if (UseCorrectionMatrices)
            {
                outMatrix = CamToRgbMatrix * inMatrix;

                if (HighlightRecovery)
                {
                    Matrix yuv = RGBToYCbCrMatrix * outMatrix;

                    /* stitch error function on top to compress highlights */
                    float startBrightness = 200;
                    if (yuv[0] > startBrightness)
                    {
                        float offset = yuv[0] - startBrightness;
                        float room = 255 - startBrightness;
                        float newLuma = startBrightness + (Erf(offset / (room * 2)) * room);
                        float scal = newLuma / yuv[0];

                        /* scale RGB values down to decrease luminance and ensure correct color */
                        outMatrix = scal * outMatrix;
                    }
                }
            }
            return outMatrix;
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

                /* now pre-scale to 0-255 to save operations later */
                PixelLookupTable[pos] = scal * 255;
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

        public float ColorTemperature
        {
            get
            {
                return _ColorTemperature;
            }
            set
            {
                _ColorTemperature = value;
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
                float[] value = new float[9];

                for (int row = 0; row < 3; row++)
                {
                    for (int col = 0; col < 3; col++)
                    {
                        value[col + row * 3] = CamToXYZMatrix[row, col];
                    }
                }
                return value;
            }
            set
            {
                for (int row = 0; row < 3; row++)
                {
                    for (int col = 0; col < 3; col++)
                    {
                        CamToXYZMatrix[row, col] = value[col + row * 3];
                    }
                }
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
                //CamToXYZMatrix = new float[] { 1f, -_Saturation, -_Saturation, -_Saturation, 0.5f, -_Saturation, -_Saturation, -_Saturation, 1f };
                LookupTablesDirty = true;
            }
        }

        public float[] WhiteBalance
        {
            get
            {
                return new float[3] { WhiteBalanceMatrix[0, 0], WhiteBalanceMatrix[1, 1], WhiteBalanceMatrix[2, 2] };
            }
            set
            {
                WhiteBalanceMatrix[0, 0] = value[0];
                WhiteBalanceMatrix[1, 1] = value[1];
                WhiteBalanceMatrix[2, 2] = value[2];
                LookupTablesDirty = true;
            }
        }
        
        public bool UseCorrectionMatrices
        {
            get
            {
                return _UseCorrectionMatrices;
            }
            set
            {
                _UseCorrectionMatrices = value;
            }
        }

        public bool HighlightRecovery
        {
            get
            {
                return _HighlightRecovery;
            }
            set
            {
                _HighlightRecovery = value;
            }
        }


        #endregion
    }
}
