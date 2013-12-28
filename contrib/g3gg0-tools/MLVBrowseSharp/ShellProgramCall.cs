using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Threading;

namespace MLVBrowseSharp
{
    public class ShellProgramCall
    {
        private string BinaryName;
        private string Description;
        private string Parameters;
        public string Commandline;
        public delegate void LineCallbackDelegate(string line);
        private LineCallbackDelegate LineCallback = null;
        private Process Proc = null;
        public string ErrorMessage = "";

        public ShellProgramCall(string binary, string description, string parameters, LineCallbackDelegate cbr)
        {
            BinaryName = binary;
            Description = description;
            Parameters = parameters;
            LineCallback = cbr;
        }

        public int Run()
        {
            string prefix = "";
            string postfix = "";

            PlatformID platform = Environment.OSVersion.Platform;

            if (File.Exists("/mach_kernel"))
            {
                platform = PlatformID.MacOSX;
            }

            switch (platform)
            {
                case PlatformID.Win32NT:
                    prefix = "";
                    postfix = ".exe";
                    break;
                case PlatformID.MacOSX:
                    prefix = "./";
                    postfix = ".osx";
                    break;
                case PlatformID.Unix:
                    prefix = "./";
                    postfix = ".linux";
                    break;
            }

            string binary = prefix + BinaryName + postfix;
            string path = Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location) + System.IO.Path.DirectorySeparatorChar;

            Commandline = binary + " " + Parameters;
            try
            {
                Console.WriteLine("Executing: '" + Commandline + "'");

                Proc = new Process();

                /* create no window, don't use shell */
                Proc.StartInfo.RedirectStandardOutput = true;
                Proc.StartInfo.UseShellExecute = false;
                Proc.StartInfo.FileName = binary;
                Proc.StartInfo.Arguments = Parameters;
                Proc.StartInfo.CreateNoWindow = true;

                /* use own handler for stdout messages */
                Proc.OutputDataReceived += ((sender, e) =>
                {
                    LineCallback(e.Data);
                });

                /* now start and wait for being finished */
                Proc.Start();
                Proc.BeginOutputReadLine();
                Proc.WaitForExit();

                Console.WriteLine("Executing: '" + Commandline + "' DONE");
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed: " + e.ToString());
                ErrorMessage = e.ToString();
                return -1;
            }

            return Proc.ExitCode;
        }

        internal void Cancel()
        {
            if(Proc == null)
            {
                return;
            }
            Proc.Kill();
        }
    }
}
