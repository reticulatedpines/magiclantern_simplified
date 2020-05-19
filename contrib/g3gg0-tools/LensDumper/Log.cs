using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace LensDumper
{
    public class Log
    {
        public static void WriteLine(string format, params object[] parms)
        {
            Console.Out.WriteLine(format, parms);
        }
    }
}
