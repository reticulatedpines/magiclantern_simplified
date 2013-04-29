using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Xml.Serialization;

namespace LensDumper
{
    class Program
    {
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

        static int Main(string[] args)
        {
            string binSourceName = null;

            if (args.Length >= 1)
            {
                binSourceName = args[0];
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

                LensDataAccessor.LensDataStructure blocks = LensDataAccessor.ParseLensData(data);

                string xmlOutName = binSourceName + ".lensxml";
                XmlSerializer writer = new XmlSerializer(blocks.GetType());
                StreamWriter file = new StreamWriter(xmlOutName);
                writer.Serialize(file, blocks);
                file.Close();
                return 0;
            }

            Console.Out.WriteLine(" Lens data dumper by g3gg0.de");
            Console.Out.WriteLine("=============================");
            Console.Out.WriteLine("");
            Console.Out.WriteLine("  Usage:");
            Console.Out.WriteLine("    Dump lens data to .lensxml:");
            Console.Out.WriteLine("      LensDumper.exe [LENS File]");
            Console.Out.WriteLine("");


            return 1;
        }
    }
}
