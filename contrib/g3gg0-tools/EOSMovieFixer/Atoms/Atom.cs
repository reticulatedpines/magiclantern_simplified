using System;
using System.Text;

namespace EOSMovieFixer.Atoms
{
    public class Atom
    {
        public UInt64 TotalLength = 0;
        public UInt64 HeaderLength = 0;
        public UInt64 OriginalPayloadLength = 0;
        public UInt64 OriginalPayloadFileOffset = 0;
        
        public UInt64 PayloadLength
        {
            get
            {
                return TotalLength - HeaderLength;
            }
        }

        public string Type = "";
        public UInt64 HeaderFileOffset = 0;
        public UInt64 PayloadFileOffset
        {
            get
            {
                return HeaderFileOffset + HeaderLength;
            }
        }
        public byte[] PayloadData = null;

        public Atom()
        {
        }

        public Atom(string type)
        {
            Type = type;
        }

        public override string ToString()
        {
            return Type + " L:" + TotalLength.ToString("X16") + " O:" + HeaderFileOffset.ToString("X16");
        }

        public virtual void ParsePayload(InputFile file, ulong position)
        {
        }
    }
}
