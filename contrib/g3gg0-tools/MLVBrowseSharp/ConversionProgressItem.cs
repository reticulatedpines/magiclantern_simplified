using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace MLVBrowseSharp
{
    public partial class ConversionProgressItem : UserControl
    {
        private ShellProgramHelper.ConversionInstance Conv = null;
        private Dictionary<int, string> Errorcodes = new Dictionary<int, string>();

        public ConversionProgressItem(ShellProgramHelper.ConversionInstance conv)
        {
            InitializeComponent();
            Conv = conv;
            lblName.Text = conv.FileName;

            /* setuo MLV error codes */
            Errorcodes.Add(-1, "Aborted");
            Errorcodes.Add(0, "No error");

            ConversionProgressWindow wnd = ConversionProgressWindow.Instance;
            if (wnd == null)
            {
                wnd = new ConversionProgressWindow();
                wnd.Show();
            }

            wnd.AddItem(this);

            Timer tim = new Timer();
            tim.Tick += new EventHandler(tim_Tick);
            tim.Interval = 250;
            tim.Start();
        }

        void tim_Tick(object sender, EventArgs e)
        {
            progressBar1.Minimum = 0;
            progressBar1.Maximum = 100;
            progressBar1.Value = (int) Math.Min(progressBar1.Maximum, Conv.Progress * progressBar1.Maximum);

            if (Conv.Exited)
            {
                btnRemove.Enabled = true;
                btnCancel.Text = "Show log";
                progressBar1.Visible = false;
                lblStatus.Visible = true;

                if (Conv.Returncode == 0)
                {
                    lblStatus.Text = "Status:   Success";
                }
                else
                {
                    if (Errorcodes.ContainsKey(Conv.Returncode))
                    {
                        lblStatus.Text = "Status:   ERROR: " + Errorcodes[Conv.Returncode];
                    }
                    else
                    {
                        lblStatus.Text = "Status:   ERROR #" + Conv.Returncode;
                    }
                }
            }
            else
            {
                lblName.Text = new FileInfo(Conv.FileName).Name + "  V-Frame #" + Conv.VideoFrameNumber + "/" + Conv.VideoFrameCount;
            }
        }

        public void Remove()
        {
            Parent.Controls.Remove(this);
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            if (!Conv.Exited)
            {
                Conv.Cancel();
            }
            else
            {
                MessageWindow wnd = new MessageWindow("Output of: " + Conv.FileName, "Exec: '" + Conv.Commandline + "'" + Environment.NewLine + Conv.StdOutMixedString);
                wnd.ShowDialog();
            }
        }

        private void btnRemove_Click(object sender, EventArgs e)
        {
            if (Conv.Exited)
            {
                Parent.Controls.Remove(this);
            }
        }
    }
}
