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

            /* first time animate until first video frame appears */
            foreach (MLVFileIcon icon in FileIcons)
            {
                icon.SingleStep = true;
                icon.Paused = false;
            }

            UpdateAnimationStatus();
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
                foreach (var elem in icon.Metadata)
                {
                    if (!fields.Contains(elem.Key))
                    {
                        fields.Add(elem.Key);
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
            ArrayList selected = new ArrayList();
            foreach (MLVFileIcon icon in FileIcons)
            {
                if (icon.Selected)
                {
                    selected.Add(icon.FileInfo);
                }
            }

            if (Form.ModifierKeys == Keys.Shift)
            {

            }
            else
            {
                ShellContextMenu ctxMnu = new ShellContextMenu();
                FileInfo[] arrFI = (FileInfo[])selected.ToArray(typeof(FileInfo));
                ctxMnu.ShowContextMenu(arrFI, this.PointToScreen(pos));
            }
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
