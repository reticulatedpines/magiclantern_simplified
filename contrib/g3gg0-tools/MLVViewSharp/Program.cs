using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using MLVViewSharp;

namespace mlv_view_sharp
{
    static class Program
    {
        /// <summary>
        /// Der Haupteinstiegspunkt für die Anwendung.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            MLVViewerForm form = new MLVViewerForm();

            if (args.Length != 0)
            {
                form.AutoplayFile = args[0];
            }
            Application.Run(form);
        }
    }
}
