using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Collections;
using System.Threading;
using System.IO;

namespace TimerGen
{
    public partial class TimerGenForm : Form
    {
        public double MaxDelta = 0.1;
        Thread ScanThread = null;
        string[] MlConfig = null;
        string MlConfigFile = "-";
        string MlVersion = "-";
        string MlCardInfo = "-";
        ModelInfo SelectedModel;

        public ModelInfo[] Models = new ModelInfo[] 
        {
            new ModelInfo("5D Mark II", 24000000, 0x230, 0x2000, 0x380, 0x3FFF),
            new ModelInfo("50D", 28800000, 0x230, 0x2000, 0x380, 0x3FFF),
            new ModelInfo("60D", 28800000, 0x230, 0x2000, 0x380, 0x3FFF),
            new ModelInfo("500D", 32000000, 0x230, 0x2000, 0x380, 0x3FFF),
            new ModelInfo("550D", 28800000, 0x230, 0x2000, 0x380, 0x3FFF),
            new ModelInfo("600D", 28800000, 0x230, 0x2000, 0x380, 0x3FFF),
            new ModelInfo("1100D", 32000000, 0x230, 0x2000, 0x380, 0x3FFF),
        };

        public TimerGenForm()
        {
            InitializeComponent();

            SelectedModel = Models[0];

            foreach (ModelInfo info in Models)
            {
                cmbModel.Items.Add(info.Name);
            }

            cmbModel.SelectedIndex = 0;

            ScanThread = new Thread(ScanThreadMain);
            ScanThread.Start();
        }

        private void UpdateMlInfo()
        {
            if (!string.IsNullOrEmpty(MlConfigFile))
            {
                lblMLPath.Text = MlConfigFile + " (" + MlCardInfo + ")";
                lblMLVersion.Text = MlVersion;
            }
            else
            {
                lblMLPath.Text = "";
                lblMLVersion.Text = "(not found)";
            }
        }

        private void ScanThreadMain()
        {
            while (true)
            {
                try
                {
                    bool found = false;
                    DriveInfo[] infos = DriveInfo.GetDrives();

                    foreach (DriveInfo info in infos)
                    {
                        if (info.DriveType == DriveType.Removable)
                        {
                            string mlCfg = info.Name + "ML" + Path.DirectorySeparatorChar + "SETTINGS" + Path.DirectorySeparatorChar + "magic.cfg";

                            if (File.Exists(mlCfg))
                            {
                                LoadConfig(mlCfg);

                                long realSize = (info.TotalSize / 1024 / 1024 / 1024);
                                long approxSize = 1;
                                while (realSize > 0)
                                {
                                    approxSize <<= 1;
                                    realSize >>= 1;
                                }
                                MlCardInfo = info.DriveFormat + ", ~" + approxSize + " GiB";
                                found = true;
                            }
                        }
                    }

                    if (!found)
                    {
                        MlConfig = null;
                        MlVersion = "";
                        MlCardInfo = "";
                        MlConfigFile = "";
                    }

                    BeginInvoke(new Action(() => { UpdateMlInfo(); }));
                }
                catch (Exception ex)
                {
                }

                Thread.Sleep(500);
            }
        }

        public class ModelInfo
        {
            public string Name;
            public ulong BaseFreq;
            public uint T1min;
            public uint T1max;
            public uint T2min;
            public uint T2max;

            public ModelInfo(string name, ulong baseFreq, uint t1min, uint t1max, uint t2min, uint t2max)
            {
                Name = name;
                BaseFreq = baseFreq;
                T1min = t1min;
                T1max = t1max;
                T2min = t2min;
                T2max = t2max;
            }
        }

        public class TimingInfo
        {
            public uint T1Value;
            public uint T2Value;
            public double Fps;
            public bool ShowTimers = false;

            public TimingInfo(uint t1, uint t2, double fps)
            {
                T1Value = t1;
                T2Value = t2;
                Fps = fps;
            }

            public TimingInfo(TimingInfo info)
            {
                T1Value = info.T1Value;
                T2Value = info.T2Value;
                Fps = info.Fps;
            }

            public override string ToString()
            {
                if (ShowTimers)
                {
                    return string.Format("0x{0:x4} 0x{1:x4}", T1Value - 1, T2Value - 1);
                }
                else
                {
                    return Fps.ToString();
                }
            }
        }

        public class ResultEntry
        {
            public TimingInfo[] Timings;
            public double Fps;

            public ResultEntry(double fps, TimingInfo[] timings)
            {
                Fps = fps;
                Timings = timings;
            }

            public override string ToString()
            {
                return Fps.ToString();
            }
        }

        private void btnGenerate_Click(object sender, EventArgs e)
        {
            ulong baseFreq = 0;
            double fps = 0;
            ulong maxResults = 0;
            SortedList<double, ArrayList> timingInfos = new SortedList<double, ArrayList>();

            timingInfos.Clear();

            if (!double.TryParse(txtFps.Text, out fps))
            {
                return;
            }

            if (!ulong.TryParse(txtBaseFreq.Text, out baseFreq))
            {
                return;
            }

            if (!ulong.TryParse(txtResults.Text, out maxResults))
            {
                return;
            }

            double clock = (double)baseFreq / fps;
            uint t1Min = SelectedModel.T1min;
            uint t1Max = SelectedModel.T1max;
            uint t2Min = SelectedModel.T2min;
            uint t2Max = SelectedModel.T2max;

            for (uint t1 = t1Min; t1 <= t1Max; t1++)
            {
                uint t2_a = (uint)Math.Floor(clock / (double)t1);
                uint t2_b = (uint)Math.Ceiling(clock / (double)t1);

                if ((t2_a > t2Min && t2_a < t2Max) && (t2_b > t2Min && t2_b < t2Max))
                {
                    double exactFps_a = Math.Round((double)baseFreq / t1 / t2_a, 9);
                    double exactFps_b = Math.Round((double)baseFreq / t1 / t2_b, 9);

                    if ((Math.Abs(exactFps_a - fps) < MaxDelta) && (Math.Abs(exactFps_b - fps) < MaxDelta))
                    {
                        TimingInfo info_a = new TimingInfo(t1, t2_a, exactFps_a);
                        TimingInfo info_b = new TimingInfo(t1, t2_b, exactFps_b);

                        AddInfo(timingInfos, info_a);
                        AddInfo(timingInfos, info_b);
                    }
                }
            }

            double[] filtered = FilterEntries(timingInfos, fps, maxResults);
            DisplayResults(timingInfos, filtered);

            return;
        }

        private void DisplayResults(SortedList<double, ArrayList> timingInfos, double[] filtered)
        {
            lstResults.Items.Clear();
            foreach (double val in filtered)
            {
                ResultEntry entry = new ResultEntry(val, (TimingInfo[])timingInfos[val].ToArray(typeof(TimingInfo)));
                lstResults.Items.Add(entry);
            }
        }

        private double[] FilterEntries(SortedList<double, ArrayList> timingInfos, double fps, ulong maxItems)
        {
            var nearestHigherValues = timingInfos.Keys.Where(i => i >= fps).OrderBy(i => i).Take((int)maxItems / 2);
            var nearestLowerValues = timingInfos.Keys.Where(i => i < fps).OrderBy(i => fps - i).Take((int)maxItems / 2);

            var combined = nearestHigherValues.Concat(nearestLowerValues).OrderBy(i => i);

            return combined.ToArray<double>();
        }


        private void AddInfo(SortedList<double, ArrayList> timingInfos, TimingInfo newInfo)
        {
            if (!timingInfos.ContainsKey(newInfo.Fps))
            {
                timingInfos.Add(newInfo.Fps, new ArrayList());
            }

            bool found = false;
            foreach (TimingInfo info in timingInfos[newInfo.Fps])
            {
                if (info.T1Value == newInfo.T1Value && info.T2Value == newInfo.T2Value)
                {
                    found = true;
                }
                if (info.T2Value == newInfo.T1Value && info.T1Value == newInfo.T2Value)
                {
                    //found = true;
                }
            }

            if (!found)
            {
                timingInfos[newInfo.Fps].Add(newInfo);
            }
        }

        private void cmbModel_SelectedIndexChanged(object sender, EventArgs e)
        {
            foreach (ModelInfo info in Models)
            {
                if (info.Name == cmbModel.Text)
                {
                    SelectedModel = info;
                    txtBaseFreq.Text = info.BaseFreq.ToString();
                }
            }
        }

        private void lstResults_SelectedIndexChanged(object sender, EventArgs e)
        {
            ResultEntry result = (ResultEntry)lstResults.SelectedItem;

            DumpTimings(result);
        }

        private void DumpTimings(ResultEntry result)
        {
            StringBuilder builder = new StringBuilder();
            TimingInfo[] infos = result.Timings;

            lstTimerValues.Items.Clear();
            btnWriteConfig.Enabled = false;

            foreach (TimingInfo info in infos)
            {
                TimingInfo infoTimers = new TimingInfo(info);
                infoTimers.ShowTimers = true;
                lstTimerValues.Items.Add(infoTimers);
            }
        }

        private void lstTimerValues_SelectedIndexChanged(object sender, EventArgs e)
        {
            btnWriteConfig.Enabled = (lstTimerValues.SelectedIndex >= 0);
        }

        private void btnWriteConfig_Click(object sender, EventArgs e)
        {
            try
            {
                if (string.IsNullOrEmpty(MlConfigFile))
                {
                    MessageBox.Show("No Magic Lantern installation found. Please enter the path where to modify the config file.");
                    OpenFileDialog dlg = new OpenFileDialog();

                    dlg.FileName = "magic.cfg";
                    if (dlg.ShowDialog() == DialogResult.OK)
                    {
                        MlCardInfo = "local disk";
                        LoadConfig(dlg.FileName);
                    }
                    else
                    {
                        return;
                    }
                }

                if (string.IsNullOrEmpty(MlConfigFile))
                {
                    MessageBox.Show("This seems not to be a correct Magic Lantern config file.");
                    return;
                }

                TimingInfo info = (TimingInfo)lstTimerValues.SelectedItem;
                bool tAFound = false;
                bool tBFound = false;

                for (int pos = 0; pos < MlConfig.Length; pos++)
                {
                    if (MlConfig[pos].StartsWith("fps.timer.a.off"))
                    {
                        MlConfig[pos] = "fps.timer.a.off = " + info.T1Value;
                        tAFound = true;
                    }
                    if (MlConfig[pos].StartsWith("fps.timer.b.off"))
                    {
                        MlConfig[pos] = "fps.timer.b.off = " + info.T2Value;
                        tBFound = true;
                    }
                }

                if (!(tAFound && tBFound))
                {
                    MessageBox.Show("This seems not to be a correct Magic Lantern config file. Could not find 'fps.timer.a/b.off'");
                    return;
                }

                File.WriteAllLines(MlConfigFile, MlConfig);

                string oldText = btnWriteConfig.Text;
                btnWriteConfig.Text = "Done";

                Thread btnTextThread = new Thread(() =>
                {
                    for (int loop = 0; loop < 5; loop++)
                    {
                        BeginInvoke(new Action(() =>
                        {
                            btnWriteConfig.Text = btnWriteConfig.Text + ".";
                        }));
                        Thread.Sleep(400);
                    }
                    BeginInvoke(new Action(() =>
                    {
                        btnWriteConfig.Text = oldText;
                    }));
                });

                btnTextThread.Start();
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to write config file: " + ex.ToString());
            }
        }

        private void LoadConfig(string file)
        {
            MlConfig = File.ReadAllLines(file);

            if (MlConfig[0].StartsWith("# Magic Lantern"))
            {
                MlVersion = MlConfig[0].Replace("# ", "").Split('(')[0];
                MlConfigFile = file;
            }
            else
            {
                MlVersion = "";
                MlConfigFile = "";
            }
        }
    }
}
