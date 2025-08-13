using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using System.ServiceProcess;
using System.IO;
using System.Reflection;
using System.Resources;
using System.Collections;

namespace WebDAVServer
{
    static class Program
    {
        public static WebDAVServerForm Instance = null;
        public static WebDAVService Service = null;

        public static int Exceptions = 0;

        /// <summary>
        /// Der Haupteinstiegspunkt für die Anwendung.
        /// </summary>
        [STAThread]
        static void Main(string[] arg)
        {
            AppDomain currentDomain = AppDomain.CurrentDomain;

            currentDomain.UnhandledException += currentDomain_UnhandledException;
            //currentDomain.FirstChanceException += currentDomain_FirstChanceException;
            currentDomain.ProcessExit += currentDomain_ProcessExit;


            if (Environment.UserInteractive || Environment.OSVersion.Platform == PlatformID.MacOSX || Environment.OSVersion.Platform == PlatformID.Unix)
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Instance = new WebDAVServerForm(arg);
                Application.Run(Instance);
            }
            else
            {
                Service = new WebDAVService();
                var ServicesToRun = new ServiceBase[] { Service };
                ServiceBase.Run(ServicesToRun);
            }
        }


        private static void WriteLog()
        {
            try
            {
                if (Instance != null)
                {
                    if (Instance.Server.EnableRequestLog)
                    {
                        string log = "Exiting, " + Exceptions + " Exceptions handled" + Environment.NewLine +Environment.NewLine;

                        log += Instance.Server._LogMessages.ToString() + Environment.NewLine + Environment.NewLine;
                        log += "Detailed Request log:" + Environment.NewLine + Environment.NewLine;
                        log += Instance.Server._RequestMessages.ToString() + Environment.NewLine;

                        File.WriteAllText("WebDAVServer.log", log);
                    }
                }
            }
            catch (Exception e)
            {

            }
        }


        static void currentDomain_ProcessExit(object sender, EventArgs e)
        {
            WriteLog();
        }
        /*
        static void currentDomain_FirstChanceException(object sender, System.Runtime.ExceptionServices.FirstChanceExceptionEventArgs e)
        {
            Exceptions++;
        }*/

        static void currentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            WriteLog();
        }
    }
}

