using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Threading;

namespace mlv_view_sharp
{
    public partial class MLVViewerForm : Form
    {
        private string FileName = "";
        private Thread PlayThread = null;
        private bool Paused = false;
        MLVHandler Handler = new MLVHandler();
        public string AutoplayFile = null;

        public MLVViewerForm()
        {
            InitializeComponent();

            pictureBox.ContextMenu = new ContextMenu(new MenuItem[] { 
                new MenuItem("Debayer Algorithm:"),
                new MenuItem("   Bilinear (slow, accurate)", new EventHandler(menu_DebayerBilin)),
                new MenuItem("   1/2 Resolution (fast)", new EventHandler(menu_Debayer_2)),
                new MenuItem("   1/4 Resolution (faster)", new EventHandler(menu_Debayer_4)), 
                new MenuItem("   1/8 Resolution (guess what..)", new EventHandler(menu_Debayer_8)),
                new MenuItem("-"), 
                new MenuItem("Color correction matrices (white balance, colorspace...):"),
                new MenuItem("   Enable (slow but better colors)", new EventHandler(menu_CorrectionOn)),
                new MenuItem("   Disable (fast)", new EventHandler(menu_CorrectionOff)),
                new MenuItem("-"), 
                new MenuItem("Highlight recovery:"),
                new MenuItem("   Enable (slow)", new EventHandler(menu_HighlightRecoveryOn)),
                new MenuItem("   Disable (fast)", new EventHandler(menu_HighlightRecoveryOff)),
                new MenuItem("-"), 
                new MenuItem("Reset RGB White Balance", new EventHandler(menu_ResetWb)),
            });
        }

        protected override void OnLoad(EventArgs e)
        {
            if (AutoplayFile != null)
            {
                PlaybackFile(AutoplayFile);
                AutoplayFile = null;
            }
            base.OnLoad(e);
        }

        protected void menu_ResetWb(Object sender, EventArgs e)
        {
            Handler.ResetWhite();
        }

        protected void menu_HighlightRecoveryOn(Object sender, EventArgs e)
        {
            Handler.HighlightRecovery = true;
        }
        protected void menu_HighlightRecoveryOff(Object sender, EventArgs e)
        {
            Handler.HighlightRecovery = false;
        }

        protected void menu_CorrectionOn(Object sender, EventArgs e)
        {
            Handler.UseCorrectionMatrices = true;
        }
        protected void menu_CorrectionOff(Object sender, EventArgs e)
        {
            Handler.UseCorrectionMatrices = false;
        }

        protected void menu_DebayerBilin(Object sender, EventArgs e)
        {
            Handler.SelectDebayer(0);
        }

        protected void menu_Debayer_2(Object sender, EventArgs e)
        {
            Handler.SelectDebayer(1);
        }

        protected void menu_Debayer_4(Object sender, EventArgs e)
        {
            Handler.SelectDebayer(2);
        }

        protected void menu_Debayer_8(Object sender, EventArgs e)
        {
            Handler.SelectDebayer(3);
        }

        protected override void OnDragEnter(DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                e.Effect = DragDropEffects.Copy;
            }
            else
            {
                e.Effect = DragDropEffects.None;
            }
        }

        protected override void OnDragDrop(DragEventArgs e)
        {
            string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

            if (files.Length > 0)
            {
                PlaybackFile(files[0]);
            }
        }

        private void PlaybackFile(string file)
        {
            if (PlayThread != null)
            {
                PlayThread.Abort();
            }
            pictureBox.Image = null;

            pictureBox.Image = new Bitmap(pictureBox.Size.Width, pictureBox.Size.Height);
            Graphics g = Graphics.FromImage(pictureBox.Image);
            g.DrawString("Loading File...", new Font("Courier New", 48), Brushes.Black, new Point(10, 10));
            g.Dispose();

            FileName = file;
            PlayThread = new Thread(PlayFile);
            PlayThread.Start();
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            if (PlayThread != null)
            {
                PlayThread.Abort();
                PlayThread = null;
            }

            base.OnClosing(e);
        }

        public void PlayFile()
        {
            MLVReader reader = null;
            Graphics picGraph = null;

            string suffix = FileName.ToLower().Substring(FileName.Length - 4, 2);

            try
            {
                if (suffix == ".m")
                {
                    reader = new MLVReader(FileName, Handler.BlockHandler);
                }
                else
                {
                    reader = new RAWReader(FileName, Handler.BlockHandler);
                }
            }
            catch (ArgumentException)
            {
                MessageBox.Show("Failed to load file");
                PlayThread = null;
                return;
            }

            Invoke(new Action(() =>
            {
                trackBarPosition.Maximum = 0;
                trackBarPosition.Value = 0;
            }));
            try
            {
                while (true)
                {
                    while (reader.ReadBlock())
                    {
                        if (Handler.FileHeader.videoClass != 0x01)
                        {
                            MessageBox.Show("This video has an unsupported format (" + Handler.FileHeader.videoClass + ")");
                            reader.Close();
                            return;
                        }


                        /* create info string */
                        string metaData = CreateMetaData();

                        /* update controls everytime */
                        Invoke(new Action(() =>
                        {
                            try
                            {
                                txtInfo.Text = metaData.ToString();
                                trackBarPosition.Maximum = reader.MaxBlockNumber;
                                if ((trackBarPosition.Value == reader.CurrentBlockNumber) || (trackBarPosition.Value == reader.CurrentBlockNumber - 1))
                                {
                                    trackBarPosition.Value = reader.CurrentBlockNumber;
                                }
                                else
                                {
                                    reader.CurrentBlockNumber = trackBarPosition.Value;
                                }
                            }
                            catch (Exception e)
                            {
                                MessageBox.Show("Exception: " + e);
                            }
                        }
                        ));

                        if (Handler.FrameUpdated)
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
                                }
                                catch (Exception e)
                                {
                                    MessageBox.Show("Exception: " + e);
                                }
                            }));
                        }

                        if (Paused && reader.LastType == "VIDF")
                        {
                            while (Paused)
                            {
                                bool reread = false;

                                Thread.Sleep(100);
                                Invoke(new Action(() =>
                                {
                                    try
                                    {
                                        if (trackBarPosition.Value != reader.CurrentBlockNumber)
                                        {
                                            reader.CurrentBlockNumber = trackBarPosition.Value;
                                            reread = true;
                                        }
                                    }
                                    catch (Exception e)
                                    {
                                        MessageBox.Show("Exception: " + e);
                                    }
                                }));

                                if (reread)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (reader.CurrentBlockNumber < reader.MaxBlockNumber - 1)
                            {
                                reader.CurrentBlockNumber++;
                            }
                            else
                            {
                                reader.CurrentBlockNumber = 0;
                                Invoke(new Action(() =>
                                {
                                    trackBarPosition.Value = reader.CurrentBlockNumber;
                                }));
                            }
                        }
                    }
                }
            }
            catch (ThreadAbortException ex)
            {
                reader.Close();
                throw ex;
            }
        }

        private string CreateMetaData()
        {
            StringBuilder metaData = new StringBuilder();

            /* this field is obligatory */
            metaData.Append(" Res: " + Handler.RawiHeader.xRes + "x" + Handler.RawiHeader.yRes);

            if (!string.IsNullOrEmpty(Handler.IdntHeader.cameraName))
            {
                metaData.Append(" | Camera: " + Handler.IdntHeader.cameraName);
            }
            if (!string.IsNullOrEmpty(Handler.LensHeader.lensName))
            {
                metaData.Append(" | Lens: " + Handler.LensHeader.lensName + " at " + Handler.LensHeader.focalLength + " mm" + " (" + Handler.LensHeader.focalDist + " mm) f/" + ((float)Handler.LensHeader.aperture / 100).ToString("0.00"));
            }
            if (!string.IsNullOrEmpty(Handler.InfoString))
            {
                metaData.Append(" | Info: " + Handler.InfoString);
            }
            if (Handler.ExpoHeader.shutterValue != 0)
            {
                metaData.Append(" | Expo: ISO " + Handler.ExpoHeader.isoValue + " 1/" + (1000000.0f / Handler.ExpoHeader.shutterValue).ToString("0") + " s");
            }
            if (!string.IsNullOrEmpty(Handler.StylHeader.picStyleName))
            {
                metaData.Append(" | Style: " + Handler.StylHeader.picStyleName);
            }
            if (Handler.WbalHeader.kelvin != 0)
            {
                metaData.Append(" | WB: " + Handler.WbalHeader.kelvin + "K");
            }

            return metaData.ToString();
        }

        private void btnPlayPause_Click(object sender, EventArgs e)
        {
            Paused ^= true;
        }

        private void pictureBox_DoubleClick(object sender, EventArgs e)
        {
            if (pictureBox.Image != null)
            {
                Size = pictureBox.Image.Size;
                pictureBox.Size = pictureBox.Image.Size;
            }
        }

        private void trackBarExposure_ValueChanged(object sender, EventArgs e)
        {
            float ev = (float)(trackBarExposure.Value) / 2.0f;

            if (trackBarExposure.Value == 0)
            {
                lblExposure.Text = "Exposure: Auto";
            }
            else
            {
                lblExposure.Text = "Exposure: " + ev.ToString() + " EV";
            }
            Handler.ExposureCorrection = ev;
        }

        private void pictureBox_MouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                pictureBox.SizeMode = PictureBoxSizeMode.Normal;
                toolTip1.SetToolTip(pictureBox, "Press SHIFT and release mouse button to set the current position as neutral gray.");
            }
        }

        private void pictureBox_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                toolTip1.Hide(pictureBox);

                if (Form.ModifierKeys == Keys.Shift && pictureBox.Image != null)
                {
                    Point pos = pictureBox.PointToClient(Cursor.Position);
                    Bitmap bmp = new Bitmap(pictureBox.Image);
                    int width = 8;

                    int posX = Math.Min(Math.Max(width, pos.X - width), bmp.Width);
                    int posY = Math.Min(Math.Max(width, pos.Y - width), bmp.Height);

                    float rValue = 0;
                    float gValue = 0;
                    float bValue = 0;

                    for (int x = posX - width / 2; x < posX + width / 2; x++)
                    {
                        for (int y = posY - width / 2; y < posY + width / 2; y++)
                        {
                            Color pixelColor = bmp.GetPixel(x, y);

                            rValue += pixelColor.R;
                            gValue += pixelColor.G;
                            bValue += pixelColor.B;
                        }
                    }

                    Handler.UseCorrectionMatrices = true;
                    Handler.SetWhite(rValue, gValue, bValue);
                }
                pictureBox.SizeMode = PictureBoxSizeMode.Zoom;
            }
        }

        private void trackBarTemperature_ValueChanged(object sender, EventArgs e)
        {
            Handler.ColorTemperature = trackBarTemperature.Value;
            lblTemperature.Text = "Temperature: " + Handler.ColorTemperature.ToString() + " K";
        }
    }
}
