using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Threading;
using mlv_view_sharp;

namespace MLVBrowseSharp
{
    public partial class MLVFileIcon : UserControl
    {
        public FileInfo FileInfo = null;
        private MLVFileList ParentList = null;
        private bool _Selected = false;
        private bool Paused = false;        
        private Thread DisplayThread = null;

        private MLVReader Reader = null;
        private MLVHandler Handler = new MLVHandler();

        public MLVFileIcon(MLVFileList parent, FileInfo file)
        {
            InitializeComponent();

            FileInfo = file;
            ParentList = parent;
            Selected = false;

            label1.Text = FileInfo.Name;

            DisplayThread = new Thread(DisplayFunc);
            DisplayThread.Priority = ThreadPriority.Lowest;
            DisplayThread.Start();
        }

        public new void Stop()
        {
            if (DisplayThread != null)
            {
                DisplayThread.Abort();
                DisplayThread = null;
            }
        }

        private void DisplayFunc()
        {
            Graphics picGraph = null;

            Thread.Sleep(300);

            Handler.UseCorrectionMatrices = false;
            Handler.SelectDebayer(2);

            string suffix = FileInfo.Extension.ToLower().Substring(0, 2);

            try
            {
                if (suffix == ".m")
                {
                    Reader = new MLVReader(FileInfo.FullName, Handler.BlockHandler);
                }
                else
                {
                    Reader = new RAWReader(FileInfo.FullName, Handler.BlockHandler);
                }
            }
            catch (ArgumentException)
            {
                MessageBox.Show("Failed to load file");
                DisplayThread = null;
                return;
            }

            try
            {
                while (true)
                {
                    while (Reader.ReadBlock())
                    {
                        if (Handler.FileHeader.videoClass != 0x01)
                        {
                            Reader.Close();
                            DisplayThread = null;
                            return;
                        }

                        if (Handler.FrameUpdated)
                        {
                            try{
                            Invoke(new Action(() =>
                            {
                                try
                                {
                                    Bitmap frame = Handler.CurrentFrame;

                                    /* update picturebox - this is very slow and inefficient */
                                    if (frame != null)
                                    {
                                        if (pictureBox.Image == null || pictureBox.Image.Size != frame.Size)
                                        {
                                            if (picGraph != null)
                                            {
                                                picGraph.Dispose();
                                            }
                                            Bitmap bmp = new Bitmap(frame);
                                            picGraph = Graphics.FromImage(bmp);
                                            pictureBox.Image = bmp;
                                        }
                                        picGraph.DrawImage(frame, 0, 0, frame.Size.Width, Handler.CurrentFrame.Size.Height);
                                        pictureBox.Refresh();
                                    }
                                }
                                catch (Exception e)
                                {
                                    MessageBox.Show("Exception: " + e);
                                }
                            }));
                            }
                            catch (Exception)
                            {
                            }

                            if (!Selected)
                            {
                                Thread.Sleep(1000);
                            }

                            while (Paused)
                            {
                                Thread.Sleep(500);
                            }
                        }

                        if (Reader.CurrentBlockNumber < Reader.MaxBlockNumber - 1)
                        {
                            Reader.CurrentBlockNumber++;
                        }
                        else
                        {
                            Reader.CurrentBlockNumber = 0;
                        }
                    }
                }
            }
            catch (ThreadAbortException ex)
            {
                Reader.Close();
                throw ex;
            }
        }

        private void mouseClick(object sender, EventArgs e)
        {
            if (e is MouseEventArgs)
            {
                MouseEventArgs arg = (MouseEventArgs)e;

                if (arg.Button == MouseButtons.Left)
                {
                    if (Form.ModifierKeys != Keys.Control)
                    {
                        ParentList.UnselectAll();
                    }
                    Selected = true;
                }
                if (arg.Button == MouseButtons.Right)
                {
                    if (!Selected)
                    {
                        ParentList.UnselectAll();
                        Selected = true;
                    }
                    ParentList.RightClick(new Point(arg.X, arg.Y));
                }
            }
        }

        internal void Unselect()
        {
            Selected = false;
        }

        public bool Selected
        {
            get
            {
                return _Selected;
            }
            set
            {
                _Selected = value;

                if (_Selected)
                {
                    BackColor = Color.LightSkyBlue;
                    label1.BackColor = Color.LightSkyBlue;
                    Reader.CurrentBlockNumber = 0;
                }
                else
                {
                    BackColor = ParentList.BackColor;
                    label1.BackColor = ParentList.BackColor;
                }
            }
        }

        internal void StopAnimation()
        {
            Paused = true;
        }

        internal void StartAnimation()
        {
            Paused = false;
        }
    }
}
