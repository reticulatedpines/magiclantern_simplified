using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MLVViewSharp
{
    public class Lut3D
    {
        /* index is R, G, B */
        protected int[] Dimensions = null;

        /* data is R, G, B */
        protected float[,,][] LookupTable = null;


        public Lut3D()
        {
            Dimensions = new int[3] {0, 0, 0};
            LookupTable = new float[0, 0, 0][];
        }

        public virtual void AllocateLookupTable()
        {
            if (Dimensions[0] < 1 || Dimensions[1] < 1 || Dimensions[2] < 1)
            {
                return;
            }

            LookupTable = new float[Dimensions[0], Dimensions[1], Dimensions[2]][];

            for (int x = 0; x < Dimensions[0]; x++)
            {
                for (int y = 0; y < Dimensions[1]; y++)
                {
                    for (int z = 0; z < Dimensions[2]; z++)
                    {
                        LookupTable[x, y, z] = new float[3] { 0, 0, 0 };
                    }
                }
            }
        }

        virtual public void Lookup(float r, float g, float b, out float r_out, out float g_out, out float b_out)
        {
            int[] idxHigh = new int[3];
            int[] idxLow = new int[3];
            float[] delta = new float[3];

            for (int ch = 0; ch < 3; ch++)
            {
                float val = 0;
                float dim = Dimensions[ch] - 1;

                switch (ch)
                {
                    case 0:
                        val = Math.Min(1, Math.Max(0, r));
                        break;
                    case 1:
                        val = Math.Min(1, Math.Max(0, g));
                        break;
                    case 2:
                        val = Math.Min(1, Math.Max(0, b));
                        break;
                }

                idxHigh[ch] = (int)Math.Ceiling(dim * val);
                idxLow[ch] = (int)Math.Floor(dim * val);
                delta[ch] = (dim * val) - idxLow[ch];
            }

            /* from https://github.com/imageworks/OpenColorIO/blob/master/src/core/Lut3DOp.cpp */
            float fx = delta[0];
            float fy = delta[1];
            float fz = delta[2];
            float[] rgbOut = new float[3];

            float[] n000 = LookupTable[idxLow[0], idxLow[1], idxLow[2]];
            float[] n100 = LookupTable[idxHigh[0], idxLow[1], idxLow[2]];
            float[] n010 = LookupTable[idxLow[0], idxHigh[1], idxLow[2]];
            float[] n001 = LookupTable[idxLow[0], idxLow[1], idxHigh[2]];
            float[] n110 = LookupTable[idxHigh[0], idxHigh[1], idxLow[2]];
            float[] n101 = LookupTable[idxHigh[0], idxLow[1], idxHigh[2]];
            float[] n011 = LookupTable[idxLow[0], idxHigh[1], idxHigh[2]];
            float[] n111 = LookupTable[idxHigh[0], idxHigh[1], idxHigh[2]];

            if (fx > fy)
            {
                if (fy > fz)
                {
                    rgbOut[0] = (1 - fx) * n000[0] + (fx - fy) * n100[0] + (fy - fz) * n110[0] + (fz) * n111[0];
                    rgbOut[1] = (1 - fx) * n000[1] + (fx - fy) * n100[1] + (fy - fz) * n110[1] + (fz) * n111[1];
                    rgbOut[2] = (1 - fx) * n000[2] + (fx - fy) * n100[2] + (fy - fz) * n110[2] + (fz) * n111[2];
                }
                else if (fx > fz)
                {
                    rgbOut[0] = (1 - fx) * n000[0] + (fx - fz) * n100[0] + (fz - fy) * n101[0] + (fy) * n111[0];
                    rgbOut[1] = (1 - fx) * n000[1] + (fx - fz) * n100[1] + (fz - fy) * n101[1] + (fy) * n111[1];
                    rgbOut[2] = (1 - fx) * n000[2] + (fx - fz) * n100[2] + (fz - fy) * n101[2] + (fy) * n111[2];
                }
                else
                {
                    rgbOut[0] = (1 - fz) * n000[0] + (fz - fx) * n001[0] + (fx - fy) * n101[0] + (fy) * n111[0];
                    rgbOut[1] = (1 - fz) * n000[1] + (fz - fx) * n001[1] + (fx - fy) * n101[1] + (fy) * n111[1];
                    rgbOut[2] = (1 - fz) * n000[2] + (fz - fx) * n001[2] + (fx - fy) * n101[2] + (fy) * n111[2];
                }
            }
            else
            {
                if (fz > fy)
                {
                    rgbOut[0] = (1 - fz) * n000[0] + (fz - fy) * n001[0] + (fy - fx) * n011[0] + (fx) * n111[0];
                    rgbOut[1] = (1 - fz) * n000[1] + (fz - fy) * n001[1] + (fy - fx) * n011[1] + (fx) * n111[1];
                    rgbOut[2] = (1 - fz) * n000[2] + (fz - fy) * n001[2] + (fy - fx) * n011[2] + (fx) * n111[2];
                }
                else if (fz > fx)
                {
                    rgbOut[0] = (1 - fy) * n000[0] + (fy - fz) * n010[0] + (fz - fx) * n011[0] + (fx) * n111[0];
                    rgbOut[1] = (1 - fy) * n000[1] + (fy - fz) * n010[1] + (fz - fx) * n011[1] + (fx) * n111[1];
                    rgbOut[2] = (1 - fy) * n000[2] + (fy - fz) * n010[2] + (fz - fx) * n011[2] + (fx) * n111[2];
                }
                else
                {
                    rgbOut[0] = (1 - fy) * n000[0] + (fy - fx) * n010[0] + (fx - fz) * n110[0] + (fz) * n111[0];
                    rgbOut[1] = (1 - fy) * n000[1] + (fy - fx) * n010[1] + (fx - fz) * n110[1] + (fz) * n111[1];
                    rgbOut[2] = (1 - fy) * n000[2] + (fy - fx) * n010[2] + (fx - fz) * n110[2] + (fz) * n111[2];
                }
            }

            r_out = rgbOut[0];
            g_out = rgbOut[1];
            b_out = rgbOut[2];
        }
    }
}
