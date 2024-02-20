using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using System.IO;
using System.Xml.Serialization;

namespace PropertyEditor
{
    static class Program
    {
        public static byte[] propPatternRing = new byte[] { 0x00, 0x00, 0x00, 0x02, 0x0c, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02 };
        public static byte[] propPatternRasen = new byte[] { 0x00, 0x00, 0x00, 0x02, 0x0c, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02 };

        public static byte[] ReadAllBytes(BinaryReader reader)
        {
            const int bufferSize = 4096;
            using (var ms = new MemoryStream())
            {
                byte[] buffer = new byte[bufferSize];
                int count;
                while ((count = reader.Read(buffer, 0, buffer.Length)) != 0)
                    ms.Write(buffer, 0, count);
                return ms.ToArray();
            }
        }

        public static ulong FindPattern(byte[] buffer, byte[] pattern, ulong align)
        {
            for (ulong pos = 0; pos < (ulong)buffer.Length; pos += align)
            {
                if (PatternMatch(buffer, pos, pattern))
                {
                    return pos;
                }
            }

            return 0;
        }

        private static bool PatternMatch(byte[] buffer, ulong offset, byte[] pattern)
        {
            if (offset + (ulong)pattern.Length > (ulong)buffer.Length)
            {
                return false;
            }

            for (ulong pos = 0; pos < (ulong)pattern.Length; pos++)
            {
                if (buffer[offset + pos] != pattern[pos])
                {
                    return false;
                }
            }

            return true;
        }

        public static int Main(string[] args)
        {
            string binSourceName = null;
            ulong address = 0;
            ulong blockSize = 0;
            ulong blockCount = 0;

            if (args.Length >= 1)
            {
                binSourceName = args[0];
            }

            if (args.Length >= 2)
            {
                address = UInt32.Parse(args[1].Replace("0x", ""), System.Globalization.NumberStyles.HexNumber);
                Log.WriteLine("[I] Supplied property offset: " + address.ToString("X8"));
            }

            if (args.Length >= 3)
            {
                blockSize = UInt32.Parse(args[2].Replace("0x", ""), System.Globalization.NumberStyles.HexNumber);
                Log.WriteLine("[I] Supplied block size: " + blockSize.ToString("X8"));
            }

            if (args.Length >= 4)
            {
                blockCount = UInt32.Parse(args[3].Replace("0x", ""), System.Globalization.NumberStyles.HexNumber);
                Log.WriteLine("[I] Supplied block count: " + blockCount.ToString("X8"));
            }

            if (binSourceName != null)
            {
                byte[] data = null;

                try
                {
                    BinaryReader reader = new BinaryReader(File.Open(binSourceName, FileMode.Open, FileAccess.Read));
                    data = ReadAllBytes(reader);
                    reader.Close();
                }
                catch (Exception ex)
                {
                    Log.WriteLine("Failed to load <" + binSourceName + ">. Reason: " + ex.GetType().ToString());
                    return -1;
                }

                if (address == 0)
                {
                    address = FindPattern(data, propPatternRing, 0x04) & 0xFFFFF00;
                    if (address != 0)
                    {
                        Log.WriteLine("[I] Autodetected property offset: " + address.ToString("X8"));
                    }
                    else
                    {
                        Log.WriteLine("[E] Autodetect property offset failed");
                        return -1;
                    }
                }

                PropertyAccessor.PropertyBlock[] blocks = PropertyAccessor.ParsePropertyBlocks(data, address, blockSize, blockCount);

                string xmlOutName = binSourceName + "_" + address.ToString("X8") + ".propxml";
                XmlSerializer writer = new XmlSerializer(blocks.GetType());
                StreamWriter file = new StreamWriter(xmlOutName);
                writer.Serialize(file, blocks);
                file.Close();
                return 0;
            }

            Console.Out.WriteLine(" Property Editor by g3gg0.de");
            Console.Out.WriteLine("=============================");
            Console.Out.WriteLine("");
            Console.Out.WriteLine("  Usage:");
            Console.Out.WriteLine("    Dump properties to .propxml:");
            Console.Out.WriteLine("      PropertyEditor.exe [ROM File] <address> <blockSize> <blockCount>");
            Console.Out.WriteLine("");

            return 1;
        }
    }
}
