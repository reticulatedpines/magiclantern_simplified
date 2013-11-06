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
            /*
            Matrix conv = new Matrix(3, 3);
            Matrix n = new Matrix(3, 3);
            Matrix value = new Matrix(3, 1);

            conv[0, 0] = 1;
            conv[0, 1] = 2;
            conv[0, 2] = 3;
            conv[1, 0] = 2;
            conv[1, 1] = 5;
            conv[1, 2] = 3;
            conv[2, 0] = 1;
            conv[2, 1] = 0;
            conv[2, 2] = 8;

            value[0, 0] = 4;
            value[1, 0] = 5;
            value[2, 0] = 6;

            n[0, 0] = 1;
            n[1, 1] = 0.5f;
            n[2, 2] = 1;

            Matrix tt = n.Inverse();
            Matrix t2 = n * value;


            Matrix mn = conv * value;
            Matrix mo = conv * n;
            Console.WriteLine(mn + mo);
            */
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
