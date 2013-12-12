using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Threading;

namespace MLVBrowseSharp
{
    public partial class BrowseForm : Form
    {
        public BrowseForm()
        {
            InitializeComponent();
            UpdateFolders();
        }

        protected override void OnActivated(EventArgs e)
        {
            mlvFileList.StartAnimation();
            base.OnActivated(e);
        }
        protected override void OnDeactivate(EventArgs e)
        {
            mlvFileList.StopAnimation();
            base.OnDeactivate(e);
        }

        private void UpdateFolders()
        {
            foreach (var drive in DriveInfo.GetDrives())
            {
                string path = drive.Name[0] + ":\\";
                if (drive.IsReady)
                {
                    AddRootNode(path, path + " (" + drive.VolumeLabel + ")");
                }
                else
                {
                    AddRootNode(path, path + " (" + drive.DriveType + ")");
                }
            }
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            mlvFileList.Stop();
            base.OnClosing(e);
        }

        private void AddRootNode(string path, string label)
        {
            TreeNode rootnode = new TreeNode(label);
            rootnode.Tag = path;
            treeView.Nodes.Add(rootnode);
            FillChildNodes(rootnode);
        }

        private void FillChildNodes(TreeNode node)
        {
            node.Nodes.Clear();
            try
            {
                DirectoryInfo dirs = new DirectoryInfo((string)node.Tag);
                foreach (DirectoryInfo dir in dirs.GetDirectories())
                {
                    TreeNode newnode = new TreeNode(dir.Name);
                    newnode.Tag = dir.FullName;

                    node.Nodes.Add(newnode);
                    newnode.Nodes.Add("*");
                }
            }
            catch (Exception)
            {
            }
        }

        private void treeView_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
            if (e.Node.Nodes[0].Text == "*")
            {
                e.Node.Nodes.Clear();
                FillChildNodes(e.Node);
            }
        }

        private void treeView_AfterCollapse(object sender, TreeViewEventArgs e)
        {
            e.Node.Nodes.Clear();
            e.Node.Nodes.Add("*");
        }

        private void treeView_AfterSelect(object sender, TreeViewEventArgs e)
        {
            string dir = (string)e.Node.Tag;

            if (dir != null)
            {
                this.Text = "MLV Browser   |   Browsing  " + dir;
            }

            TreeNode parm = e.Node;
            if (parm.Parent == null)
            {
                FillChildNodes(e.Node);
            }

            mlvFileList.ShowDirectory(dir);
            mlvFileList.UpdateGroups();
            mlvFileList.SetIconSize(trackSize.Value);

            cmbGrouping.Items.Clear();
            cmbGrouping.Items.AddRange(mlvFileList.Groups);
        }

        private void cmbGrouping_TextChanged(object sender, EventArgs e)
        {
            mlvFileList.GroupBy(cmbGrouping.Text);
        }

        private void trackSize_ValueChanged(object sender, EventArgs e)
        {
            mlvFileList.SetIconSize(trackSize.Value);
        }

        private void chkAnimation_CheckedChanged(object sender, EventArgs e)
        {
            mlvFileList.AnimateAll = chkAnimation.Checked;
        }
    }
}
