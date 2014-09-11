using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using System.ServiceProcess;

namespace WebDAVServer
{
    static class Program
    {
        /// <summary>
        /// Der Haupteinstiegspunkt für die Anwendung.
        /// </summary>
        [STAThread]
        static void Main(string[] arg)
        {
            if (Environment.UserInteractive)
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new WebDAVServerForm(arg));
            }
            else
            {
                var ServicesToRun = new ServiceBase[] { new WebDAVService() };
                ServiceBase.Run(ServicesToRun);
            }
        }
    }
}
