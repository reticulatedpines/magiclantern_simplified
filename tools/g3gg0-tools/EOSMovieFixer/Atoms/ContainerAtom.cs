using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections;

namespace EOSMovieFixer.Atoms
{
    public class ContainerAtom : Atom
    {
        public ArrayList Children = new ArrayList();

        public ContainerAtom()
        {
        }

        public ContainerAtom(string type)
        {
            Type = type;
        }

        public override void ParsePayload(InputFile file, ulong position)
        {
            /* must not do anything */
        }
    }
}
