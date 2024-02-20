using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Globalization;

namespace MLVViewSharp
{
    public class CubeLut : Lut3D
    {
        public string Title = "";

        public CubeLut(string filename) : base()
        {
            StreamReader sr = new StreamReader(filename);

            ParseCubeFile(sr.ReadToEnd());
        }

        private void ParseCubeFile(string content)
        {
            float[] domainMin = new float[3] { 0, 0, 0};
            float[] domainMax = new float[3] { 1, 1, 1 };
            int lineNumber = 0;

            foreach (string line in content.Replace("\r", "").Split('\n'))
            {
                string trimmed = line.Trim();
                if (trimmed.Length == 0 || trimmed.StartsWith("#"))
                {
                    continue;
                }

                if (trimmed.StartsWith("TITLE"))
                {
                    Title = trimmed.Split(' ')[1].Trim();
                    continue;
                }

                if (trimmed.StartsWith("DOMAIN_MIN"))
                {
                    string[] elem = trimmed.Split(' ');

                    for (int ch = 0; ch < 3; ch++)
                    {
                        if (!float.TryParse(elem[1 + ch], out domainMin[ch]))
                        {
                            throw new Exception("Invalid DOMAIN_MIN entry");
                        }
                    }
                    continue;
                }

                if (trimmed.StartsWith("DOMAIN_MAX"))
                {
                    string[] elem = trimmed.Split(' ');

                    for (int ch = 0; ch < 3; ch++)
                    {
                        if (!float.TryParse(elem[1 + ch], out domainMax[ch]))
                        {
                            throw new Exception("Invalid DOMAIN_MAX entry");
                        }
                    }
                    continue;
                }

                if (trimmed.StartsWith("LUT_3D_SIZE"))
                {
                    string dimension = trimmed.Split(' ')[1].Trim();
                    int dim = 0;

                    if (!int.TryParse(dimension, out dim))
                    {
                        throw new Exception("Invalid LUT size entry");
                    }
                    Dimensions[0] = dim;
                    Dimensions[1] = dim;
                    Dimensions[2] = dim;

                    AllocateLookupTable();
                    continue;
                }

                /* is that a number? */
                if (trimmed[0] >= '0' && trimmed[0] <= '9')
                {
                    string[] elem = trimmed.Split(' ');

                    int x = lineNumber % Dimensions[0];
                    int y = (lineNumber / Dimensions[0]) % Dimensions[1];
                    int z = (lineNumber / (Dimensions[0] * Dimensions[1])) % Dimensions[2];

                    for (int ch = 0; ch < 3; ch++)
                    {
                        if (!float.TryParse(elem[ch], NumberStyles.Any, CultureInfo.InvariantCulture, out LookupTable[x, y, z][ch]))
                        {
                            throw new Exception("Invalid LUT table entry");
                        }
                    }
                    lineNumber++;
                    continue;
                }

                Console.Out.WriteLine("Unhandled line: '" + line + "'");
            }
        }
    }
}
