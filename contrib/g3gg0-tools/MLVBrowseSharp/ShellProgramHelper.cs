using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Windows.Forms;
using System.Threading;

namespace MLVBrowseSharp
{
    public class ShellProgramHelper
    {
        public static void ReplaceParameters(ref string[] parameters, string variable, string value)
        {
            for (int pos = 0; pos < parameters.Length; pos++)
            {
                if (parameters[pos].Contains(variable))
                {
                    parameters[pos] = parameters[pos].Replace(variable, value);
                }
            }
        }

        public class ConversionInstance
        {
            private ShellProgramCall Call = null;
            public string FileName = "";
            public float Progress = 0;
            public bool Exited = false;

            public int BlockNumber = 0;
            public int BlockCount = 0;
            public int VideoFrameNumber = 0;
            public int VideoFrameCount = 0;
            public int AudioFrameNumber = 0;
            public int AudioFrameCount = 0;

            public int Returncode = 0;

            private StringBuilder StdOutMixed = new StringBuilder();
            private StringBuilder StdOut = new StringBuilder();
            private StringBuilder StdErr = new StringBuilder();

            public ConversionInstance(string name)
            {
                FileName = name;
            }

            public string Commandline
            {
                get
                {
                    return Call.Commandline;
                }
            }

            public string StdOutString
            {
                get
                {
                    return StdOut.ToString();
                }
            }

            public string StdErrString
            {
                get
                {
                    return StdErr.ToString();
                }
            }

            public string StdOutMixedString
            {
                get
                {
                    return StdOutMixed.ToString();
                }
            }

            public void LineCallback(string line)
            {
                if (line == null)
                {
                    Exited = true;
                    Console.WriteLine("File: '" + FileName + "' EXITED");
                    return;
                }

                if (line.Length < 5)
                {
                    return;
                }

                string content = line.Substring(4);
                string header = line.Substring(0, 3);

                switch (header)
                {
                    case "[I]":
                    {
                        StdOut.AppendLine(content);
                        StdOutMixed.AppendLine(content);
                        break;
                    }
                    case "[E]":
                    {
                        StdErr.AppendLine(content);
                        StdOutMixed.AppendLine(content);
                        break;
                    }
                    case "[P]":
                    {
                        string[] fields = content.Split(' ');

                        foreach (string stat in fields)
                        {
                            string[] elements = stat.Split(':');

                            switch (elements[0])
                            {
                                case "B":
                                {
                                    string[] values = elements[1].Split('/');

                                    int.TryParse(values[0], out BlockNumber);
                                    int.TryParse(values[1], out BlockCount);
                                    break;
                                }

                                case "A":
                                {
                                    string[] values = elements[1].Split('/');

                                    int.TryParse(values[0], out AudioFrameNumber);
                                    int.TryParse(values[1], out AudioFrameCount);
                                    break;
                                }

                                case "V":
                                {
                                    string[] values = elements[1].Split('/');

                                    int.TryParse(values[0], out VideoFrameNumber);
                                    int.TryParse(values[1], out VideoFrameCount);

                                    if (BlockCount > 0)
                                    {
                                        Progress = (float)VideoFrameNumber / (float)VideoFrameCount;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
            }

            internal void Execute(string binary, string description, string parameters)
            {
                Call = new ShellProgramCall(binary, description, parameters, LineCallback);
                Returncode = Call.Run();
            }

            internal void Cancel()
            {
                Call.Cancel();
            }
        }

        public static void RunForAll(string[] files, string binary, string description, string[] parameters)
        {
            foreach (string file in files)
            {
                ReplaceParameters(ref parameters, "%INFILE%", "\"" + file + "\"");
                ReplaceParameters(ref parameters, "%INFILENAME%", new DirectoryInfo(file).Name);


                /* delete index file */
                string idx = file.ToUpper().Replace(".MLV", ".IDX");
                if (File.Exists(idx))
                {
                    try
                    {
                        //File.Delete(idx);
                    }
                    catch (Exception e)
                    {
                        MessageBox.Show("There is a index file called " + idx + " which i cannot delete. Please delete it if you get problems during export");
                    }
                }

                ConversionInstance instance = new ConversionInstance(file);
                ConversionProgressItem convItem = new ConversionProgressItem(instance);

                Thread processThread = new Thread(() =>
                {
                    instance.Execute(binary, description, String.Join(" ", parameters));
                });

                processThread.Start();
            }
        }
    }
}
