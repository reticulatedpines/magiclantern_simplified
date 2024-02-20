using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using EOSMovieFixer.Atoms;
using System.Collections;

namespace EOSMovieFixer
{
    public class AtomParser
    {
        public string[] ContainerTypes = { "moov", "trak", "mdia", "minf", "stbl", "skip" };
        public string[] AtomTypes = { "cmov", "cmvd", "co64", "ctts", "dcom", "edts", "elst", "esds", "fiel", "free", "ftyp", "gmhd", "hdlr", "iods", "junk", "mdat", "mdhd", "mdia", "minf", "moov", "mvhd", "pict", "pnot", "rdrf", "rmcd", "rmcs", "rmda", "rmdr", "rmqu", "rmra", "rmvc", "skip", "smhd", "stbl", "stco", "stsc", "stsd", "stss", "stsz", "stts", "tkhd", "trak", "uuid", "vmhd", "wide", "wfex" };

        public ArrayList ParseFile(InputFile file)
        {
            ArrayList atoms = new ArrayList();
            UInt64 position = 0;

            while (position < file.Length)
            {
                string type = GetAtomType(file, position);
                Atom atom = null;

                if (IsContainer(type))
                {
                    atom = new ContainerAtom(type);
                    ParseContainer(file, position, (ContainerAtom)atom);
                }
                else if (IsAtom(type))
                {
                    atom = new LeafAtom(type);
                    ParseAtom(file, position, atom);
                }
                else
                {
                    throw new Exception("Invalid atom at pos 0x" + position.ToString("X16"));
                }

                atoms.Add(atom);
                position += atom.TotalLength;
            }

            return atoms;
        }

        private bool IsAtom(string type)
        {
            return AtomTypes.Contains<string>(type);
        }

        private bool IsContainer(string type)
        {
            return ContainerTypes.Contains<string>(type);
        }

        private void ParseAtom(InputFile file, ulong position, Atom atom)
        {
            file.Seek(position);
            UInt64 length = file.ReadUInt32(position);

            if (length == 0)
            {
                atom.TotalLength = file.Length - position;
                atom.HeaderLength = 8;
            }
            else if (length == 1)
            {
                atom.TotalLength = file.ReadUInt64(position + 8);
                atom.HeaderLength = 16;
            }
            else
            {
                atom.TotalLength = length;
                atom.HeaderLength = 8;
            }

            if (atom.Type == "mdat")
            {
                if (!IsAtom(GetAtomType(file, position + atom.TotalLength)))
                {
                    EOSMovieFixer.Log("      Buggy [mdat] section (32 bit overflow detected)...");
                    EOSMovieFixer.Log("        Probing [mdat] end...");

                    /* try to find end of mdat by probing next atom */
                    EOSMovieFixer.Log("        length 0x" + atom.TotalLength.ToString("X16"));
                    while (!IsAtom(GetAtomType(file, position + atom.TotalLength)) && ((position + atom.TotalLength) < file.Length))
                    {
                        atom.TotalLength += 0x0100000000;
                        EOSMovieFixer.Log("        length 0x" + atom.TotalLength.ToString("X16"));
                    }

                    if (!IsAtom(GetAtomType(file, position + atom.TotalLength)))
                    {
                        throw new Exception("Could not find 'mdat' end");
                    }
                    else
                    {
                        EOSMovieFixer.Log("      Real atom end found successfully");
                    }
                }
            }

            atom.HeaderFileOffset = position;

            /* save original length for rewriting purposes */
            atom.OriginalPayloadLength = atom.PayloadLength;
            atom.OriginalPayloadFileOffset = atom.PayloadFileOffset;

            atom.ParsePayload(file, position);
        }

        private void ParseContainer(InputFile file, ulong position, ContainerAtom container)
        {
            ParseAtom(file, position, container); 
            
            ArrayList atoms = new ArrayList();
            UInt64 offset = container.HeaderLength;

            while (offset < container.TotalLength)
            {
                string type = GetAtomType(file, position + offset);
                Atom atom = null;

                if (IsContainer(type))
                {
                    atom = new ContainerAtom(type);
                    ParseContainer(file, position + offset, (ContainerAtom)atom);
                }
                else
                {
                    atom = new LeafAtom(type);
                    ParseAtom(file, position + offset, atom);
                }

                container.Children.Add(atom);
                offset += atom.TotalLength;
            }
        }

        private string GetAtomType(InputFile file, ulong position)
        {
            string type = "";
            byte[] buffer = new byte[4];

            file.ReadBytes(position + 4, buffer, 4);

            for (int pos = 0; pos < 4; pos++)
            {
                type += (char)buffer[pos];
            }

            return type;
        }
    }
}
