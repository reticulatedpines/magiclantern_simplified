using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections;
using EOSMovieFixer.Atoms;

namespace EOSMovieFixer
{
    public class AtomWriter
    {
        public AtomWriter()
        {
        }

        public void SaveFile(InputFile inFile, OutputFile outFile, ArrayList atoms)
        {
            foreach (var obj in atoms)
            {
                if (obj is ContainerAtom)
                {
                    SaveContainer(inFile, outFile, (ContainerAtom)obj);
                }
                else
                {
                    SaveAtom(inFile, outFile, (LeafAtom)obj);
                }
            }
        }

        private void SaveContainer(InputFile inFile, OutputFile outFile, ContainerAtom container)
        {
            if (container.HeaderLength == 8)
            {
                outFile.WriteUInt32((UInt32)container.TotalLength);
                outFile.WriteChars(container.Type.ToCharArray());
            }
            else
            {
                outFile.WriteUInt32(1);
                outFile.WriteChars(container.Type.ToCharArray());
                outFile.WriteUInt64(container.TotalLength);
            }

            foreach (var obj in container.Children)
            {
                if (obj is ContainerAtom)
                {
                    SaveContainer(inFile, outFile, (ContainerAtom)obj);
                }
                else
                {
                    SaveAtom(inFile, outFile, (LeafAtom)obj);
                }
            }
        }

        private void SaveAtom(InputFile inFile, OutputFile outFile, LeafAtom leaf)
        {
            if (leaf.HeaderLength == 8)
            {
                outFile.WriteUInt32((UInt32)leaf.TotalLength);
                outFile.WriteChars(leaf.Type.ToCharArray());
            }
            else
            {
                outFile.WriteUInt32(1);
                outFile.WriteChars(leaf.Type.ToCharArray());
                outFile.WriteUInt64(leaf.TotalLength);
            }

            if (leaf.PayloadData != null)
            {
                outFile.WriteBytes(leaf.PayloadData);
            }
            else
            {
                if (leaf.OriginalPayloadFileOffset > 0)
                {
                    outFile.WriteFromInput(inFile, leaf.OriginalPayloadFileOffset, leaf.PayloadLength);
                }
                else
                {
                    outFile.WriteFromInput(inFile, leaf.PayloadFileOffset, leaf.PayloadLength);
                }
            }
        }
    }
}
