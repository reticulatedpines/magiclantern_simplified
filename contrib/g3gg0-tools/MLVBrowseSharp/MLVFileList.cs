using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Collections;
using System.Threading;
using System.Diagnostics;

namespace MLVBrowseSharp
{
    public partial class MLVFileList : UserControl
    {
        private ArrayList FileIcons = new ArrayList();
        public string[] Groups = new string[0];
        private bool _AnimateAll = false;

        public MLVFileList()
        {
            InitializeComponent();
        }

        internal bool AnimateAll
        {
            get
            {
                return _AnimateAll;
            }
            set
            {
                _AnimateAll = value;
                UpdateAnimationStatus();
            }
        }

        public void Stop()
        {
            foreach (MLVFileIcon icon in FileIcons)
            {
                icon.Stop();
            }
        }

        internal void ShowDirectory(string dir)
        {
            /* first hide all shown icons */
            Controls.Clear();
            Controls.Add(fileList);
            fileList.Controls.Clear();

            /* now destroy all allocated ones */
            foreach (MLVFileIcon icon in FileIcons)
            {
                icon.Stop();
            }
            FileIcons.Clear();

            if (dir == null)
            {
                return;
            }

            /* build a new list of icons */
            ArrayList files = new ArrayList();

            try
            {
                DirectoryInfo dirs = new DirectoryInfo(dir);
                foreach (FileInfo file in dirs.GetFiles())
                {
                    if (file.Extension.ToLower() == ".mlv" || file.Extension.ToLower() == ".raw")
                    {
                        files.Add(file);
                    }
                }
            }
            catch (Exception)
            {
            }

            /* now instanciate all icons */
            foreach(FileInfo file in files)
            {
                MLVFileIcon icon = new MLVFileIcon(this, file);
                FileIcons.Add(icon);
            }

            UpdateFileIcons();

            /* delay a bit so the GUI has enough time to render until the cpu load raises */
            Thread renderStart = new Thread(() =>
            {
                Thread.Sleep(100);

                /* first time animate until first video frame appears */
                foreach (MLVFileIcon icon in FileIcons)
                {
                    icon.SingleStep = true;
                    icon.Paused = false;

                    icon.Start();
                }

                UpdateAnimationStatus();
            });
            renderStart.Start();
        }

        private void UpdateFileIcons()
        {
            UpdateGroups();
            fileList.Controls.Clear();
            fileList.Controls.AddRange((MLVFileIcon[])FileIcons.ToArray(typeof(MLVFileIcon)));
        }

        public void UpdateGroups()
        {
            ArrayList fields = new ArrayList();

            foreach (MLVFileIcon icon in FileIcons)
            {
                lock (icon.Metadata)
                {
                    foreach (var elem in icon.Metadata)
                    {
                        if (!fields.Contains(elem.Key))
                        {
                            fields.Add(elem.Key);
                        }
                    }
                }
            }

            Groups = (string[])fields.ToArray(typeof(string));
        }

        internal void UnselectAll()
        {
            foreach (MLVFileIcon icon in FileIcons)
            {
                icon.Selected = false;
            }
        }

        internal void RightClick(Point pos)
        {
            if (Form.ModifierKeys == Keys.Shift)
            {

            }
            else
            {
                ContextMenu menu = new ContextMenu();

                menu.MenuItems.Add("Export selected files as .RAW + .WAV", new EventHandler(Menu_ExportRaw));
                menu.MenuItems.Add("Export selected files as .DNG + .WAV", new EventHandler(Menu_ExportDng));
                menu.MenuItems.Add("-");
                menu.MenuItems.Add("Open Shell Context Menu", new EventHandler(Menu_ShellContextMenu));

                menu.Show(this, PointToClient(MousePosition));
            }
        }

        private void RunProgram(string program, string[] parameters)
        {
            string param = String.Join(" ", parameters);
            string prefix = "";

            if (!Environment.OSVersion.Platform.ToString().Contains("Win32"))
            {
                prefix = "./";
            }

            string path = Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location) + System.IO.Path.DirectorySeparatorChar;

            Process proc = Process.Start(prefix + program, param);

            proc.WaitForExit();
            Thread.Sleep(5000);
        }


        private void ReplaceParameters(ref string[] parameters, string variable, string value)
        {
            for (int pos = 0; pos < parameters.Length; pos++)
            {
                if (parameters[pos].Contains(variable))
                {
                    parameters[pos] = parameters[pos].Replace(variable, value);
                }
            }
        }

        private void RunProgramForSelected(string program, string[] parameters)
        {
            foreach (MLVFileIcon icon in FileIcons)
            {
                if (icon.Selected)
                {
                    MLVFileIcon fileIcon = icon;
                    ReplaceParameters(ref parameters, "%INFILE%", "\"" + fileIcon.FileInfo.FullName + "\"");
                    ReplaceParameters(ref parameters, "%INFILENAME%", fileIcon.FileInfo.Name);

                    /* delete index file */
                    string idx = fileIcon.FileInfo.FullName.ToUpper().Replace(".MLV", ".IDX");
                    if (File.Exists(idx))
                    {
                        try
                        {
                            File.Delete(idx);
                        }
                        catch (Exception e)
                        {
                            MessageBox.Show("There is a index file called " + idx + " which i cannot delete. Please delete it if you get problems during export");
                        }
                    }

                    fileIcon.Processing = true;
                    Thread processThread = new Thread(() =>
                    {
                        RunProgram(program, parameters);

                        fileIcon.Invoke(new Action(() =>
                        {
                            fileIcon.Processing = false;
                        }));
                    });

                    processThread.Start();
                }
            }
        }

        private string GetSavePath()
        {
            SaveFileDialog dlg = new SaveFileDialog();
            dlg.FileName = "[here]";
            dlg.ValidateNames = false;
            dlg.Title = "Select the path to write the .RAW to";

            if (dlg.ShowDialog() == DialogResult.OK)
            {
                string path = dlg.FileName.Remove(dlg.FileName.LastIndexOf(System.IO.Path.DirectorySeparatorChar)) + System.IO.Path.DirectorySeparatorChar;
                return path;
            }
            
            return null;
        }

        void Menu_ExportRaw(object sender, EventArgs args)
        {
            string path = GetSavePath();

            if(path == null)
            {
                return;
            }

            ArrayList mlvDumpParams = new ArrayList();
            mlvDumpParams.Add("-r");
            mlvDumpParams.Add("-o");
            mlvDumpParams.Add("\"" + path + "%INFILENAME%.RAW\"");
            mlvDumpParams.Add("%INFILE%");

            RunProgramForSelected("mlv_dump", (string[])mlvDumpParams.ToArray(typeof(string)));
        }

        void Menu_ExportDng(object sender, EventArgs args)
        {
            string path = GetSavePath();

            if (path == null)
            {
                return;
            }

            ArrayList mlvDumpParams = new ArrayList();
            mlvDumpParams.Add("--dng");
            mlvDumpParams.Add("-o");
            mlvDumpParams.Add("\"" + path + "%INFILENAME%.frame_\"");
            mlvDumpParams.Add("%INFILE%");

            RunProgramForSelected("mlv_dump", (string[])mlvDumpParams.ToArray(typeof(string)));
        }

        void Menu_ShellContextMenu(object sender, EventArgs args)
        {
            ArrayList selected = new ArrayList();
            foreach (MLVFileIcon icon in FileIcons)
            {
                if (icon.Selected)
                {
                    selected.Add(icon.FileInfo);
                }
            }

            ShellContextMenu ctxMnu = new ShellContextMenu();
            FileInfo[] arrFI = (FileInfo[])selected.ToArray(typeof(FileInfo));
            ctxMnu.ShowContextMenu(arrFI, MousePosition);
        }

        internal void StartAnimation()
        {
            if (!AnimateAll)
            {
                return;
            }

            foreach (MLVFileIcon icon in FileIcons)
            {
                icon.StartAnimation();
            }
        }

        internal void StopAnimation()
        {
            foreach (MLVFileIcon icon in FileIcons)
            {
                icon.StopAnimation();
            }
        }

        internal void UpdateAnimationStatus()
        {
            int selected = 0;

            foreach (MLVFileIcon icon in FileIcons)
            {
                if (icon.Selected)
                {
                    selected++;
                }
            }

            if (selected > 1 || !AnimateAll)
            {
                StopAnimation();
            }
            else if (selected == 1)
            {
                StopAnimation();
                foreach (MLVFileIcon icon in FileIcons)
                {
                    if (icon.Selected)
                    {
                        icon.StartAnimation();
                    }
                }
            }
            else
            {
                StartAnimation();
            }
        }

        internal void IconSelected(MLVFileIcon icon)
        {
            UpdateAnimationStatus();
        }

        internal void GroupBy(string name)
        {
            TableLayoutPanel table = new TableLayoutPanel();

            table.Dock = DockStyle.Fill;
            table.ColumnStyles.Clear();
            table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            table.ColumnCount = 1;
            table.AutoScroll = true;

            table.RowStyles.Clear();
            table.RowCount = 0;


            MLVFileIcon[] list = (MLVFileIcon[])FileIcons.ToArray(typeof(MLVFileIcon));
            var result = list.GroupBy(a => a.TryGetMetadata(name), a => a).OrderBy(b => b.Key).ToList();

            foreach (var grp in result)
            {
                GroupBox box = new GroupBox();
                FlowLayoutPanel panel = new FlowLayoutPanel();

                box.Text = name + "  |  " + grp.Key;
                box.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
                box.Dock = DockStyle.Fill;
                box.AutoSize = true;
                box.AutoSizeMode = AutoSizeMode.GrowAndShrink;

                panel.Location = new System.Drawing.Point(6, 16);
                panel.Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top;
                panel.AutoSize = true;
                panel.AutoSizeMode = AutoSizeMode.GrowAndShrink;

                box.Controls.Add(panel);

                foreach (var icon in grp.OrderBy(b => b.TryGetMetadata("Time: Date/Time")))
                {
                    panel.Controls.Add(icon);
                }

                table.RowStyles.Add(new RowStyle(SizeType.AutoSize, 0));
                table.Controls.Add(box, 0, table.RowCount);
                table.RowCount = table.RowStyles.Count;
            }

            Dock = DockStyle.Fill;
            AutoSize = true;
            AutoSizeMode = AutoSizeMode.GrowAndShrink;
            Controls.Clear();
            Controls.Add(table);
        }

        internal void SetIconSize(int p)
        {
            SuspendLayout();
            foreach (MLVFileIcon icon in FileIcons)
            {
                icon.SuspendLayout();
            }
            foreach (MLVFileIcon icon in FileIcons)
            {
                icon.SetSize(p);
            }
            foreach (MLVFileIcon icon in FileIcons)
            {
                icon.ResumeLayout();
            }
            ResumeLayout();
        }

        private void fileList_MouseEnter(object sender, EventArgs e)
        {
            Focus();
        }
    }
}
