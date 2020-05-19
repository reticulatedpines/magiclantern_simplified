using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace MLVBrowseSharp
{
    static class Program
    {
        /// <summary>
        /// Der Haupteinstiegspunkt für die Anwendung.
        /// </summary>
        [STAThread]
        static void Main()
        {
            AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler(CurrentDomain_UnhandledException);

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new BrowseForm());
        }

        static void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Exception ex = (Exception)e.ExceptionObject;

            MessageBox.Show("Please contact g3gg0 with this information:" + Environment.NewLine + Environment.NewLine + ex.Message + Environment.NewLine + ex.StackTrace,
                  "Unhandled Exception", MessageBoxButtons.OK, MessageBoxIcon.Stop);
        }
    }
}
