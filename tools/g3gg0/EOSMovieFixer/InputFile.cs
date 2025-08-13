using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace EOSMovieFixer
{
    public class InputFile
    {
        private BinaryReader reader = null;
        public UInt64 Length
        {
            get
            {
                return (UInt64)reader.BaseStream.Length;
            }
        }

        public InputFile(string fileName)
        {
            reader = new BinaryReader(File.Open(fileName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite));
        }

        internal void ReadBytes(UInt64 position, byte[] buffer, int bytes)
        {
            Seek(position);
            byte[] buf = reader.ReadBytes(bytes);
            if (buf.Length == bytes)
            {
                Array.Copy(buf, buffer, bytes);
            }
        }

        internal void Seek(UInt64 position)
        {
            reader.BaseStream.Seek((long)position, SeekOrigin.Begin);
        }

        internal UInt32 ReadUInt32(UInt64 position)
        {
            Seek(position);

            byte[] a32 = new byte[4];
            a32 = reader.ReadBytes(4);
            Array.Reverse(a32);
            return BitConverter.ToUInt32(a32, 0);
        }

        internal UInt64 ReadUInt64(UInt64 position)
        {
            Seek(position);

            byte[] a64 = new byte[8];
            a64 = reader.ReadBytes(8);
            Array.Reverse(a64);
            return BitConverter.ToUInt64(a64, 0);
        }

        internal void Close()
        {
            reader.Close();
            reader = null;
        }
    }
}
