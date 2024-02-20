using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EOSMovieFixer.Atoms
{
    public class LeafAtom : Atom
    {
        public LeafAtom()
        {
        }
        public LeafAtom(string type)
        {
            Type = type;
        }

        public override void ParsePayload(InputFile file, ulong position)
        {
            base.ParsePayload(file, position);
        }
    }
}
