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
        public MLVFileList()
        {
            InitializeComponent();
        }

        public void Stop()
        {
            foreach (MLVFileIcon icon in fileList.Controls)
            {
                icon.Stop();
            }
        }

        internal void ShowDirectory(string dir)
        {
            foreach (MLVFileIcon icon in fileList.Controls)
            {
                icon.Stop();
            }
            fileList.Controls.Clear();

            if (dir == null)
            {
                return;
            }

            try
            {
                DirectoryInfo dirs = new DirectoryInfo(dir);
                foreach (FileInfo file in dirs.GetFiles())
                {
                    if (file.Extension.ToLower() == ".mlv" || file.Extension.ToLower() == ".raw")
                    {
                        MLVFileIcon icon = new MLVFileIcon(this, file);
                        fileList.Controls.Add(icon);
                    }
                }
            }
            catch (Exception)
            {
            }
        }

        internal void UnselectAll()
        {
            foreach (MLVFileIcon icon in fileList.Controls)
            {
                icon.Unselect();
            }
        }

        internal void RightClick(Point pos)
        {
            ArrayList selected = new ArrayList();
            foreach (MLVFileIcon icon in fileList.Controls)
            {
                if (icon.Selected)
                {
                    selected.Add(icon.FileInfo);
                }
            }
            ShellContextMenu ctxMnu = new ShellContextMenu();
            FileInfo[] arrFI = (FileInfo[])selected.ToArray(typeof(FileInfo));
            ctxMnu.ShowContextMenu(arrFI, this.PointToScreen(pos));
        }

        internal void StartAnimation()
        {
            foreach (MLVFileIcon icon in fileList.Controls)
            {
                icon.StartAnimation();
            }
        }

        internal void StopAnimation()
        {
            foreach (MLVFileIcon icon in fileList.Controls)
            {
                icon.StopAnimation();
            }
        }
    }
}
