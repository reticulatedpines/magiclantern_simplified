using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace EOSMovieFixer
{
    public class OutputFile
    {
        private BinaryWriter writer = null;

        public OutputFile(string fileName)
        {
            writer = new BinaryWriter(File.Open(fileName, FileMode.OpenOrCreate, FileAccess.ReadWrite));
        }

        internal void WriteUInt32(UInt32 p)
        {
            byte[] buf = BitConverter.GetBytes(p);
            Array.Reverse(buf);
            writer.Write(buf);
        }

        internal void WriteChars(char[] p)
        {
            writer.Write(p);
        }

        internal void WriteUInt64(UInt64 p)
        {
            byte[] buf = BitConverter.GetBytes(p);
            Array.Reverse(buf);
            writer.Write(buf);
        }

        internal void WriteBytes(byte[] p)
        {
            writer.Write(p);
        }

        internal void WriteFromInput(InputFile inFile, ulong position, ulong length)
        {
            ulong blockSize = 8 * 1024 * 1024;
            ulong remaining = length;

            while(remaining>0)
            {
                byte[] buf = new byte[Math.Min(remaining, blockSize)];
                inFile.ReadBytes(position, buf, buf.Length);
                writer.Write(buf);

                remaining -= (ulong)buf.Length;
                position += (ulong)buf.Length;
            }
        }

        internal void Close()
        {
            writer.Close();
            writer = null;
        }
    }
}
