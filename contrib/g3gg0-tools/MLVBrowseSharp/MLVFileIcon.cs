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
using System.Diagnostics;
using System.Globalization;

namespace MLVBrowseSharp
{
    public partial class MLVFileIcon : UserControl
    {
        public FileInfo FileInfo = null;
        private MLVFileList ParentList = null;
        private bool _Selected = false;
        internal bool Paused = false;
        internal bool SingleStep = true;
        private bool SeekMode = false;
        private Thread DisplayThread = null;
        private int NextBlockNumber = -1;

        private MLVReader Reader = null;
        private MLVHandler Handler = new MLVHandler();

        public Dictionary<string, string> Metadata = new Dictionary<string, string>();
        

        public MLVFileIcon(MLVFileList parent, FileInfo file)
        {
            InitializeComponent();

            FileInfo = file;
            ParentList = parent;
            Selected = false;

            SingleStep = false;
            Paused = true;

            textLabel.Text = FileInfo.Name;
        }

        public bool Exited
        {
            get
            {
                return DisplayThread == null;
            }
        }

        public void Start()
        {
            if (DisplayThread == null)
            {
                DisplayThread = new Thread(DisplayFunc);
                DisplayThread.Priority = ThreadPriority.Lowest;
                DisplayThread.Start();
            }
        }

        public void Stop()
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
                SetText("Load failed");
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
                            SetText("Not supported");
                            Reader.Close();
                            DisplayThread = null;
                            return;
                        }

                        UpdateMetadata();

                        if (Handler.FrameUpdated)
                        {
                            try
                            {
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

                                        textLabel.Text = FileInfo.Name + Environment.NewLine + "(Frame " + (Handler.VidfHeader.frameNumber + 1) + "/" + Reader.TotalFrameCount + ")";

                                        if (Reader.FrameErrors > 0)
                                        {
                                            textLabel.BackColor = Color.Red;
                                            textLabel.Text += " (Errors)";

                                            if (Reader.FrameMissingErrors > 0)
                                            {
                                                SetMetadata("Error: Missing frames", Reader.FrameMissingErrors + " frame IDs are missing in the MLV. If this is a multi-chunk video, make sure you have all chunks in the same place.");
                                            }
                                            if(Reader.FrameRedundantErrors > 0)
                                            {
                                                SetMetadata("Error: Redundant frames", Reader.FrameRedundantErrors + " frames with same ID appeared more than once in the MLV. Report please.");
                                            }
                                        }
                                    }
                                    catch (Exception e)
                                    {
                                        SetText(e.GetType().ToString());
                                    }
                                }));
                            }
                            catch (Exception ex)
                            {
                            }

                            /* videos that are not selected, run very slowly */
                            if (!Selected)
                            {
                                Thread.Sleep(1000);
                            }
                            
                            if (SingleStep)
                            {
                                SingleStep = false;
                                Paused = true;
                            }

                            /* dont break when there was no preview frame processed yet */
                            while (Paused && pictureBox.Image != null)
                            {
                                Thread.Sleep(50);
                            }
                        }

                        if (Reader.CurrentBlockNumber < Reader.MaxBlockNumber - 1)
                        {
                            Reader.CurrentBlockNumber++;
                        }
                        else
                        {
                            if (pictureBox.Image == null)
                            {
                                SetText("Contains no video");
                                DisplayThread = null;
                                return;
                            }
                            Reader.CurrentBlockNumber = 0;
                        }

                        if (NextBlockNumber >= 0)
                        {
                            Reader.CurrentBlockNumber = NextBlockNumber;
                            NextBlockNumber = -1;
                        }
                    }
                }
            }
            catch (ThreadAbortException ex)
            {
                Reader.Close();
                throw ex;
            }
            catch (IOException ex)
            {
                SetText(ex.GetType().ToString());
                return;
            }
            catch (Exception ex)
            {
                SetText(ex.GetType().ToString());
                return;
            }
        }

        private string GetMetadata()
        {
            StringBuilder sb = new StringBuilder();

            lock (Metadata)
            {
                foreach (var elem in Metadata)
                {
                    sb.AppendLine(elem.Key + " : " + elem.Value);
                }
            }

            return sb.ToString();
        }

        private void UpdateMetadata()
        {
            lock (Metadata)
            {
                ulong serial = 0;
                ulong.TryParse(Handler.IdntHeader.cameraSerial, NumberStyles.HexNumber, CultureInfo.CurrentCulture, out serial);
                SetMetadata("Camera: Body serial", serial.ToString());
                SetMetadata("Camera: Model name", Handler.IdntHeader.cameraName);
                SetMetadata("Camera: Model ID", "0x" + Handler.IdntHeader.cameraModel.ToString("X8"));

                SetMetadata("Lens: Lens ID", Handler.LensHeader.lensID.ToString());
                SetMetadata("Lens: Lens name", Handler.LensHeader.lensName);
                SetMetadata("Lens: Aperture", "f/" + ((float)Handler.LensHeader.aperture / 100).ToString("0.00"));
                SetMetadata("Lens: Focal length", Handler.LensHeader.focalLength + " mm");
                SetMetadata("Lens: Focal distance", Handler.LensHeader.focalDist + " mm");

                SetMetadata("Info: Info string", Handler.InfoString);

                SetMetadata("Raw: Resolution", Handler.RawiHeader.xRes + "x" + Handler.RawiHeader.yRes);
                SetMetadata("Raw: Bits per pixel", Handler.RawiHeader.raw_info.bits_per_pixel.ToString());
                SetMetadata("Raw: Frame size", (Handler.RawiHeader.raw_info.frame_size / 1024 / 1024) + " MiB");

                if (Handler.RawiHeader.raw_info.dynamic_range < Int32.MaxValue)
                {
                    SetMetadata("Raw: Dynamic range", (Handler.RawiHeader.raw_info.dynamic_range / 100).ToString("0.00") + " EV");
                }

                SetMetadata("Style: Picture style name", Handler.StylHeader.picStyleName);
                SetMetadata("Style: Picture style id", Handler.StylHeader.picStyleId.ToString());
                SetMetadata("Style: Colortone", Handler.StylHeader.colortone.ToString());
                SetMetadata("Style: Contrast", Handler.StylHeader.contrast.ToString());
                SetMetadata("Style: Saturation", Handler.StylHeader.saturation.ToString());
                SetMetadata("Style: Sharpness", Handler.StylHeader.sharpness.ToString());

                ushort tm_year = (ushort)(Handler.RtciHeader.tm_year + 1900);
                ushort tm_mon = Handler.RtciHeader.tm_mon;
                ushort tm_mday = Handler.RtciHeader.tm_mday;
                ushort tm_hour = Handler.RtciHeader.tm_hour;
                ushort tm_min = Handler.RtciHeader.tm_min;
                ushort tm_sec = Handler.RtciHeader.tm_sec;

                if (tm_year > 1900 && tm_mon > 0 && tm_mday > 0)
                {
                    DateTime date = new DateTime(tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec);
                    SetMetadata("Time: Date/Time", date.ToLongDateString() + " " + date.ToLongTimeString());
                }
                else
                {
                    SetMetadata("Time: Date/Time", tm_year.ToString() + "/" + tm_mon + "/" + tm_mday + " " + tm_hour + ":" + tm_min + ":" + tm_sec);
                }

                SetMetadata("White: White balance mode", Handler.WbalHeader.wb_mode.ToString());
                SetMetadata("White: Color temperature", Handler.WbalHeader.kelvin.ToString());
            }
        }

        private void SetMetadata(string desc, string value)
        {
            if (!string.IsNullOrEmpty(value))
            {
                Metadata[desc] = value;
            }
        }

        private void SetText(string text)
        {
            try
            {
                BeginInvoke(new Action(() =>
                {
                    try
                    {
                        Bitmap bmp = new Bitmap(pictureBox.Width, pictureBox.Height);
                        Graphics graph = Graphics.FromImage(bmp);
                        pictureBox.Image = bmp;
                        graph.DrawString(text, new Font("Courier New", 10), new SolidBrush(Color.Black), new PointF(0, pictureBox.Height/2));
                    }
                    catch (Exception)
                    {
                    }
                }));
            }
            catch (Exception)
            {
            }
        }

        private void mouseClick(object sender, EventArgs e)
        {
            if (!ParentList.Focused)
            {
                //ParentList.Focus();
            }

            if (e is MouseEventArgs)
            {
                MouseEventArgs arg = (MouseEventArgs)e;

                if (arg.Button == MouseButtons.Left)
                {
                    if (Form.ModifierKeys != Keys.Control)
                    {
                        ParentList.UnselectAll();
                    }
                    Selected ^= true;
                }
                if (arg.Button == MouseButtons.Right)
                {
                    if (!Selected)
                    {
                        /* parent takes care about which icon is being animated */
                        ParentList.UnselectAll();
                        ParentList.IconSelected(this);

                        Selected = true;
                        SeekMode = false;
                    }
                    ParentList.RightClick(new Point(arg.X, arg.Y));
                }
                ParentList.UpdateAnimationStatus();
            }
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
                    textLabel.BackColor = Color.LightSkyBlue;
                    splitContainer1.BackColor = Color.LightSkyBlue;
                    NextBlockNumber = 0;
                }
                else
                {
                    BackColor = ParentList.BackColor;
                    textLabel.BackColor = ParentList.BackColor;
                    splitContainer1.BackColor = ParentList.BackColor;

                    if (Reader != null && Reader.FrameErrors > 0)
                    {
                        BackColor = Color.Red;
                        textLabel.BackColor = Color.Red;
                        splitContainer1.BackColor = Color.Red;
                    }
                }
            }
        }

        internal void StopAnimation()
        {
            SingleStep = false;
            Paused = true;
        }

        internal void StartAnimation()
        {
            SingleStep = false;
            Paused = false;
        }

        private void pictureBox_DoubleClick(object sender, EventArgs e)
        {
            MLVViewerForm form = new MLVViewerForm(FileInfo.FullName);
            form.Show();
        }

        private void pictureBox_MouseMove(object sender, MouseEventArgs e)
        {
            if (Reader == null)
            {
                return;
            }

            if (Selected && SeekMode)
            {
                float pct = (float)e.X / (float)Width;

                NextBlockNumber = (int)Math.Min(Reader.MaxBlockNumber, Math.Max(0, ((float)Reader.MaxBlockNumber * pct)));
                SingleStep = true;
                Paused = false;
            }
        }

        private void pictureBox_MouseDown(object sender, MouseEventArgs e)
        {
            SeekMode = true;
        }

        private void pictureBox_MouseUp(object sender, MouseEventArgs e)
        {
            SeekMode = false;
        }

        private void pictureBox_MouseEnter(object sender, EventArgs e)
        {
            toolTip.SetToolTip(pictureBox, GetMetadata());
        }

        private void pictureBox_MouseLeave(object sender, EventArgs e)
        {
            toolTip.RemoveAll();
        }

        internal object TryGetMetadata(string p)
        {
            lock (Metadata)
            {
                if (Metadata.ContainsKey(p))
                {
                    return Metadata[p];
                }
            }

            return "";
        }

        internal void SetSize(int p)
        {
            Width = p;
            Height = p;
        }
    }
}
