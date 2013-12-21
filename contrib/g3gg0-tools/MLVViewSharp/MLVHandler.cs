using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.Drawing.Drawing2D;
using mlv_view_sharp;
using System.Threading;
using System.Windows.Forms;
using System.Drawing.Imaging;

using MLVViewSharp;
using NAudio.Wave;

namespace mlv_view_sharp
{
    public class MLVHandler
    {
        public Bitmap CurrentFrame;
        private LockBitmap LockBitmap = null;
        public bool FrameUpdated = false; 
        internal float _ExposureCorrection = 0.0f;
        internal Lut3D ColorLut = null;

        public int RawFixOffset = 0;
        public bool VideoEnabled = true;
        public bool AudioEnabled = true;

        private WaveOut DriverOut;
        private BufferedWaveProvider WaveProvider;


        public MLVTypes.mlv_rawi_hdr_t RawiHeader;
        public MLVTypes.mlv_file_hdr_t FileHeader;
        public MLVTypes.mlv_expo_hdr_t ExpoHeader;
        public MLVTypes.mlv_lens_hdr_t LensHeader;
        public MLVTypes.mlv_idnt_hdr_t IdntHeader;
        public MLVTypes.mlv_rtci_hdr_t RtciHeader;
        public MLVTypes.mlv_styl_hdr_t StylHeader;
        public MLVTypes.mlv_wbal_hdr_t WbalHeader;
        public MLVTypes.mlv_vidf_hdr_t VidfHeader;

        public string InfoString = "";

        private Debayer Debayer = new DebayerHalfRes(0);
        private BitUnpack Bitunpack = new BitUnpackCanon();

        private ushort[,] PixelData = new ushort[0, 0];
        private float[, ,] RGBData = new float[0, 0, 0];


        float[] camMatrix = new float[] { 0.6722f, -0.0635f, -0.0963f, -0.4287f, 1.2460f, 0.2028f, -0.0908f, 0.2162f, 0.5668f };

        public MLVHandler()
        {
        }

        public void HandleBlock(string type, MLVTypes.mlv_file_hdr_t header, byte[] raw_data, int raw_pos, int raw_length)
        {
            FileHeader = header;
        }

        public void HandleBlock(string type, MLVTypes.mlv_expo_hdr_t header, byte[] raw_data, int raw_pos, int raw_length)
        {
            ExpoHeader = header;
        }

        public void HandleBlock(string type, MLVTypes.mlv_lens_hdr_t header, byte[] raw_data, int raw_pos, int raw_length)
        {
            LensHeader = header;
        }

        public void HandleBlock(string type, MLVTypes.mlv_idnt_hdr_t header, byte[] raw_data, int raw_pos, int raw_length)
        {
            IdntHeader = header;
        }

        public void HandleBlock(string type, MLVTypes.mlv_rtci_hdr_t header, byte[] raw_data, int raw_pos, int raw_length)
        {
            RtciHeader = header;
        }

        public void HandleBlock(string type, MLVTypes.mlv_styl_hdr_t header, byte[] raw_data, int raw_pos, int raw_length)
        {
            StylHeader = header;
        }

        public void HandleBlock(string type, MLVTypes.mlv_wbal_hdr_t header, byte[] raw_data, int raw_pos, int raw_length)
        {
            WbalHeader = header;
        }

        public void HandleBlock(string type, MLVTypes.mlv_info_hdr_t header, byte[] raw_data, int raw_pos, int raw_length)
        {
            InfoString = Encoding.ASCII.GetString(raw_data, raw_pos, raw_length);
        }

        public void HandleBlock(string type, MLVTypes.mlv_wavi_hdr_t header, byte[] raw_data, int raw_pos, int raw_length)
        {
            if (DriverOut != null)
            {
                DriverOut.Stop();
            }
            DriverOut = new WaveOut();
            DriverOut.DesiredLatency = 100;

            WaveFormat fmt = new WaveFormat((int)header.samplingRate, header.bitsPerSample, header.channels);
            WaveProvider = new BufferedWaveProvider(fmt);
            WaveProvider.BufferLength = 256 * 1024;

            DriverOut.Init(WaveProvider);
            DriverOut.Play();
        }

        public void HandleBlock(string type, MLVTypes.mlv_rawi_hdr_t header, byte[] raw_data, int raw_pos, int raw_length)
        {
            RawiHeader = header;

            if (FileHeader.videoClass != 0x01)
            {
                return;
            }

            for (int pos = 0; pos < camMatrix.Length; pos++)
            {
                camMatrix[pos] = (float)header.raw_info.color_matrix1[2 * pos] / (float)header.raw_info.color_matrix1[2 * pos + 1];
            }

            Bitunpack.BitsPerPixel = header.raw_info.bits_per_pixel;

            Debayer.Saturation = 0.12f;
            Debayer.Brightness = 1;
            Debayer.BlackLevel = header.raw_info.black_level;
            Debayer.WhiteLevel = header.raw_info.white_level;
            Debayer.CamMatrix = camMatrix;

            /* simple fix to overcome a mlv_dump misbehavior. it simply doesnt scale white and black level when changing bit depth */
            while (Debayer.WhiteLevel > (1 << header.raw_info.bits_per_pixel))
            {
                Debayer.BlackLevel >>= 1;
                Debayer.WhiteLevel >>= 1;
            }

            PixelData = new ushort[header.yRes, header.xRes];
            RGBData = new float[header.yRes, header.xRes, 3];

            CurrentFrame = new System.Drawing.Bitmap(RawiHeader.xRes, RawiHeader.yRes, PixelFormat.Format24bppRgb);
            LockBitmap = new LockBitmap(CurrentFrame);
        }

        private void HandleBlockAsync(object thread_parm)
        {
            object[] parm = (object[])thread_parm;
            MLVTypes.mlv_vidf_hdr_t header = (MLVTypes.mlv_vidf_hdr_t)parm[0];
            byte[] raw_data = (byte[])parm[1];
            int raw_pos = (int)parm[2];
        }

        public void HandleBlock(string type, MLVTypes.mlv_audf_hdr_t header, byte[] rawData, int rawPos, int rawLength)
        {
            if (WaveProvider != null)
            {
                while (WaveProvider.BufferedBytes + (int)(rawLength - header.frameSpace) > WaveProvider.BufferLength)
                {
                    Thread.Sleep(100);
                }
                WaveProvider.AddSamples(rawData, (int)(rawPos + header.frameSpace), (int)(rawLength - header.frameSpace));
            }
        }

        public void HandleBlock(string type, MLVTypes.mlv_vidf_hdr_t header, byte[] rawData, int rawPos, int rawLength)
        {
            VidfHeader = header;

            if (FileHeader.videoClass != 0x01 || LockBitmap == null)
            {
                return;
            }

            if (!VideoEnabled)
            {
                return;
            }

            lock (this)
            {
                int startPos = rawPos + (int)Math.Max(header.frameSpace - RawFixOffset, 0);

                /* first extract the raw channel values */
                Bitunpack.Process(rawData, startPos, rawLength, PixelData);

                /* then debayer the pixel data */
                Debayer.Process(PixelData, RGBData);

                /* and transform into a bitmap for displaying */
                LockBitmap.LockBits();

                int pos = 0;
                uint[] average = new uint[] { 0, 0, 0 };

                for (int y = 0; y < RawiHeader.yRes; y++)
                {
                    for (int x = 0; x < RawiHeader.xRes; x++)
                    {
                        float r = RGBData[y, x, 0];
                        float g = RGBData[y, x, 1];
                        float b = RGBData[y, x, 2];

                        if (ColorLut != null)
                        {
                            ColorLut.Lookup(r, g, b, out r, out g, out b);
                        }

                        /* now scale to TV black/white levels */
                        ScaleLevels(ref r);
                        ScaleLevels(ref g);
                        ScaleLevels(ref b);

                        average[0] += (uint)g;
                        average[1] += (uint)b;
                        average[2] += (uint)r;

                        /* limit RGB values */
                        LockBitmap.Pixels[pos++] = (byte)b;
                        LockBitmap.Pixels[pos++] = (byte)g;
                        LockBitmap.Pixels[pos++] = (byte)r;
                    }
                }
                LockBitmap.UnlockBits();

                int pixels = RawiHeader.yRes * RawiHeader.xRes;

                /* make sure the average brightness is somewhere in the mid range */
                if (Math.Abs(_ExposureCorrection) == 0.0f)
                {
                    double averageBrightness = (average[0] + average[1] + average[2]) / (3 * pixels);
                    if (averageBrightness < 100)
                    {
                        Debayer.Brightness *= 1.0f + (float)(100.0f - averageBrightness) / 100.0f;
                    }
                    if (averageBrightness > 200)
                    {
                        Debayer.Brightness /= 1.0f + (float)(averageBrightness - 200.0f) / 55.0f;
                    }
                }
                else
                {
                    Debayer.Brightness = (float)Math.Pow(2,_ExposureCorrection);
                }

                FrameUpdated = true;
            }
        }

        private void ScaleLevels(ref float value)
        {
            value *= (235 - 16);
            value += 16;
            value = Math.Max(0, Math.Min(255, value));
        }

        public void BlockHandler(string type, object header, byte[] raw_data, int raw_pos, int raw_length)
        {
            FrameUpdated = false;

            switch (type)
            {
                case "MLVI":
                    HandleBlock(type, (MLVTypes.mlv_file_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "VIDF":
                    HandleBlock(type, (MLVTypes.mlv_vidf_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "AUDF":
                    HandleBlock(type, (MLVTypes.mlv_audf_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "RAWI":
                    HandleBlock(type, (MLVTypes.mlv_rawi_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "WAVI":
                    HandleBlock(type, (MLVTypes.mlv_wavi_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "EXPO":
                    HandleBlock(type, (MLVTypes.mlv_expo_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "LENS":
                    HandleBlock(type, (MLVTypes.mlv_lens_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "RTCI":
                    HandleBlock(type, (MLVTypes.mlv_rtci_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "IDNT":
                    HandleBlock(type, (MLVTypes.mlv_idnt_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "XREF":
                    break;
                case "INFO":
                    HandleBlock(type, (MLVTypes.mlv_info_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "DISO":
                    break;
                case "MARK":
                    break;
                case "STYL":
                    HandleBlock(type, (MLVTypes.mlv_styl_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
                case "ELVL":
                    break;
                case "WBAL":
                    HandleBlock(type, (MLVTypes.mlv_wbal_hdr_t)header, raw_data, raw_pos, raw_length);
                    break;
            }
        }

        public void SelectDebayer(int downscale)
        {
            lock (this)
            {
                Debayer newDebayer = null;

                switch (downscale)
                {
                    case 0:
                        newDebayer = new DebayerBilinear();
                        break;
                    default:
                        newDebayer = new DebayerHalfRes(1 << (downscale - 1));
                        break;
                }

                newDebayer.BlackLevel = Debayer.BlackLevel;
                newDebayer.WhiteLevel = Debayer.WhiteLevel;
                newDebayer.Brightness = Debayer.Brightness;
                newDebayer.Saturation = Debayer.Saturation;
                newDebayer.CamMatrix = Debayer.CamMatrix;
                newDebayer.WhiteBalance = Debayer.WhiteBalance;
                newDebayer.UseCorrectionMatrices = Debayer.UseCorrectionMatrices;

                Debayer = newDebayer;
            }
        }

        public float ExposureCorrection
        {
            get
            {
                return _ExposureCorrection;
            }
            set
            {
                _ExposureCorrection = value;
            }
        }

        public void SetWhite(float r, float g, float b)
        {
            if (Math.Abs(r) == 0 || Math.Abs(g) == 0 || Math.Abs(b) == 0)
            {
                return;
            }

            float[] wb = Debayer.WhiteBalance;
            wb[0] /= (r / g);
            wb[2] /= (b / g);
            Debayer.WhiteBalance = wb;
        }

        public void ResetWhite()
        {
            Debayer.WhiteBalance = new float[] { 1, 1, 1 };
        }

        public float ColorTemperature
        {
            get
            {
                return Debayer.ColorTemperature;
            }
            set
            {
                Debayer.ColorTemperature = value;
            }
        }

        public bool UseCorrectionMatrices
        {
            get
            {
                return Debayer.UseCorrectionMatrices;
            }
            set
            {
                Debayer.UseCorrectionMatrices = value;
            }
        }

        public bool HighlightRecovery
        {
            get
            {
                return Debayer.HighlightRecovery;
            }
            set
            {
                Debayer.HighlightRecovery = value;
            }
        }

        internal void SetLut(Lut3D lut)
        {
            ColorLut = lut;
        }
    }
}
