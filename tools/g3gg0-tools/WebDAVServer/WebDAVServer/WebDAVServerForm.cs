using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Threading;
using System.Configuration.Install;
using System.Reflection;
using Microsoft.Win32;
using System.Security;
using System.Collections;
using System.Runtime.InteropServices;

namespace WebDAVServer
{
    public partial class WebDAVServerForm : Form
    {
        private bool AllowClosing = false;
        public WebDAVServer Server = null;
        private System.Windows.Forms.Timer UpdateTimer = null;
        private bool StartServerAfterServiceStopped = false;
        private bool StartServiceAfterServiceStopped = false;

        public static event SessionEndingEventHandler SessionEnding = null;


        public WebDAVServerForm(string[] arg)
        {
            InitializeComponent();

            /* append version number */
            Text += " v" + WebDAVServer.Version;

            /* register stuff for minimizing etc */
            Resize += new EventHandler(WebDAVServer_Resize);

            if (Environment.OSVersion.Platform == PlatformID.Win32NT)
            {
                notifyIcon.MouseClick += new MouseEventHandler(notifyIcon_MouseClick);
                notifyIcon.ContextMenuStrip = contextMenuToolbar;
                this.notifyIcon.Visible = true;
            }


            UpdateTimer = new System.Windows.Forms.Timer();
            UpdateTimer.Interval = 100;
            UpdateTimer.Tick += new EventHandler(UpdateTimer_Tick);
            UpdateTimer.Start();

            UpdateButtons();

            SessionEnding += new SessionEndingEventHandler(WebDAVServerForm_SessionEnding);

            Server = new WebDAVServer(arg);
            Server.LogUpdated += new EventHandler(Server_LogUpdated);
            Server.Start(false);


            txtPath.Text = Server.Settings.Path;
            txtPort.Text = Server.Settings.Port.ToString();
            txtAuth.Text = Server.Settings.AuthTokens;
            txtCacheTime.Text = Server.Settings.CacheTime.ToString();
            txtPrefetch.Text = Server.Settings.PrefetchCount.ToString();
            chkShowInfos.Checked = Server.Settings.ShowInfos;
            chkShowJpeg.Checked = Server.Settings.ShowJpeg;
            chkShowFits.Checked = Server.Settings.ShowFits;
            chkShowDng.Checked = Server.Settings.ShowDng;
            chkShowWav.Checked = Server.Settings.ShowWav;

            UpdateDriveLetters();
        }

        void UpdateTimer_Tick(object sender, EventArgs e)
        {
            if (Visible)
            {
                RefreshLog();
                RefreshBar();
            }
        }

        private void RefreshBar()
        {
            lock (progressBar)
            {
                long total = 0;
                long transferred = 0;

                Server.TransferGetStats(out total, out transferred);

                if (total > 0)
                {
                    progressBar.Visible = true;
                    progressBar.Value = (int)Math.Max(0,Math.Min(1000,((1000.0f * transferred) / total)));
                }
                else
                {
                    progressBar.Visible = false;
                }

            }
        }

        private void RefreshLog()
        {
            lock (txtLog)
            {
                if (StartServerAfterServiceStopped && !WebDAVService.Running)
                {
                    StartServerAfterServiceStopped = false;
                    Server.Restart();
                }

                if (StartServiceAfterServiceStopped && !WebDAVService.Running)
                {
                    StartServiceAfterServiceStopped = false;
                    WebDAVService.StartService();
                }

                UpdateButtons();
                txtLog.Text = Server.Statistics + Environment.NewLine + Server.LogMessages;
            }
        }

        void Server_LogUpdated(object sender, EventArgs e)
        {
            if (Visible)
            {
                try
                {
                    BeginInvoke(new Action(() =>
                    {
                        RefreshLog();
                    }));
                }
                catch (Exception)
                {
                }
            }
        }

        void notifyIcon_MouseClick(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                if (!Visible)
                {
                    Show();
                }
                else
                {
                    Hide();
                }
                WindowState = FormWindowState.Normal;
            }
        }

        private void WebDAVServer_Resize(object sender, System.EventArgs e)
        {
            if (FormWindowState.Minimized == WindowState)
            {
                Hide();
            }
            WindowState = FormWindowState.Normal;
        }

        void WebDAVServerForm_SessionEnding(object sender, SessionEndingEventArgs e)
        {
            Server.Stop();
            Close();
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            if (AllowClosing)
            {
                Server.Stop();
            }
            else
            {
                e.Cancel = true;
                Hide();
            }
            base.OnClosing(e);
        }

        private void UpdateButtons()
        {
            if (WebDAVService.Installed)
            {
                btnInstall.Enabled = false;
                btnUninstall.Enabled = true;

                if (WebDAVService.Running)
                {
                    btnStartService.Enabled = false;
                    btnStopService.Enabled = true;
                }
                else
                {
                    btnStartService.Enabled = true;
                    btnStopService.Enabled = false;
                }
            }
            else
            {
                btnInstall.Enabled = true;
                btnUninstall.Enabled = false;
                btnStartService.Enabled = false;
                btnStopService.Enabled = false;
            }
        }

        private void btnWriteConfig_Click(object sender, EventArgs e)
        {
            SaveConfig();

            if (WebDAVService.Installed && WebDAVService.Running)
            {
                StartServiceAfterServiceStopped = true;

                new Thread(() =>
                {
                    try
                    {
                        WebDAVService.StopService();
                    }
                    catch (InvalidOperationException ex)
                    {
                        MessageBox.Show("Could not restart service. Make sure you run the server as a privileged user (e.g. Administrator).", "Failed to stop Windows Service");
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.ToString());
                    }
                }).Start();
            }
        }

        private void SaveConfig()
        {
            string file = Server.GetExecutableRoot() + "\\" + Server.DefaultConfigFileName;
            Server.SaveConfigFile(file);
        }

        private void btnDeleteConfig_Click(object sender, EventArgs e)
        {
            string file = Server.GetExecutableRoot() + "\\" + Server.DefaultConfigFileName;
            File.Delete(file);
        }

        private void btnInstall_Click(object sender, EventArgs e)
        {
            new Thread(() =>
            {
                Server.Stop("Installed Windows Service");

                string executable = Assembly.GetAssembly(typeof(WebDAVServer)).Location;

                try
                {
                    ManagedInstallerClass.InstallHelper(new[] { "/i", executable });
                    WebDAVService.StartService();
                }
                catch (InvalidOperationException ex)
                {
                    MessageBox.Show("Could not install service. Make sure you run the server as a privileged user (e.g. Administrator).", "Failed to install Windows Service");
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.ToString());
                }
            }).Start();
        }

        private void btnUninstall_Click(object sender, EventArgs e)
        {
            /* when detected server stop, start this server */
            StartServerAfterServiceStopped = true;

            WebDAVService.StopService();

            new Thread(() =>
            {
                string executable = Assembly.GetAssembly(typeof(WebDAVServer)).Location;

                try
                {
                    ManagedInstallerClass.InstallHelper(new[] { "/u", executable });
                }
                catch (InvalidOperationException ex)
                {
                    MessageBox.Show("Could not uninstall service. Make sure you run the server as a privileged user (e.g. Administrator).", "Failed to uninstall Windows Service");
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.ToString());
                }
            }).Start();
        }

        private void btnStartService_Click(object sender, EventArgs e)
        {
            new Thread(() =>
            {
                try
                {
                    Server.Stop("Started Windows Service");
                    WebDAVService.StartService();
                }
                catch (InvalidOperationException ex)
                {
                    MessageBox.Show("Could not start service. Make sure you run the server as a privileged user (e.g. Administrator).", "Failed to start Windows Service");
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.ToString());
                }
            }).Start();
        }

        private void btnStopService_Click(object sender, EventArgs e)
        {
            /* when detected server stop, start this server */
            StartServerAfterServiceStopped = true;

            new Thread(() =>
            {
                try
                {
                    WebDAVService.StopService();
                }
                catch (InvalidOperationException ex)
                {
                    MessageBox.Show("Could not stop service. Make sure you run the server as a privileged user (e.g. Administrator).", "Failed to stop Windows Service");
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.ToString());
                }
            }).Start();
        }

        private void btnPath_Click(object sender, EventArgs e)
        {
            string folderName = "[Folder]";
            Console.WriteLine("btnPath_Click");
            OpenFileDialog dlg = new OpenFileDialog();


            dlg.CheckFileExists = false;
            dlg.FileName = folderName;

            Console.WriteLine("ShowDialog...");
            if (dlg.ShowDialog() == DialogResult.OK)
            {
                Console.WriteLine("ShowDialog... OK");
                string folder = "";
                string[] splits = dlg.FileName.Split('\\');

                for (int pos = 0; pos < splits.Length - 1; pos++)
                {
                    folder += splits[pos] + '\\';
                }

                txtPath.Text = folder;
            }
            Console.WriteLine("ShowDialog... done");
        }

        private void txtPath_TextChanged(object sender, EventArgs e)
        {
            if (Directory.Exists(txtPath.Text))
            {
                Server.Settings.Path = txtPath.Text;
                Server.Restart();
                txtPath.BackColor = Color.White;
            }
            else
            {
                txtPath.BackColor = Color.Red;
            }
        }

        private void txtPort_TextChanged(object sender, EventArgs e)
        {
            int port = 0;
            int.TryParse(txtPort.Text, out port);

            if (port > 0 && port < 65536)
            {
                Server.Settings.Port = port;
                Server.Restart();
                txtPort.BackColor = Color.White;
            }
            else
            {
                txtPort.BackColor = Color.Red;
            }
        }

        private void txtAuth_TextChanged(object sender, EventArgs e)
        {
            Server.Settings.AuthTokens = txtAuth.Text;
            Server.Restart();
        }

        private void ctxShow_Click(object sender, EventArgs e)
        {
            Show();
        }

        private void ctxQuit_Click(object sender, EventArgs e)
        {
            AllowClosing = true;
            Close();
        }

        private void cmbDrives_DropDown(object sender, EventArgs e)
        {
            UpdateDriveLetters();
        }

        private void UpdateDriveLetters()
        {
            DriveInfo[] drives = DriveInfo.GetDrives();
            ArrayList letters = new ArrayList();

            cmbDrives.Text = "";
            cmbDrives.Items.Clear();

            for (char letter = 'C'; letter <= 'Z'; letter++)
            {
                bool found = false;
                foreach (var drive in drives)
                {
                    if (drive.Name == letter + ":\\")
                    {
                        found = true;
                    }
                }
                if (!found)
                {
                    cmbDrives.Items.Add(letter + ":");
                }
            }

            if (cmbDrives.Items.Count > 0)
            {
                cmbDrives.Text = cmbDrives.Items[0].ToString();
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        private class NETRESOURCE
        {
            public int dwScope = 0;
            public int dwType = 0;
            public int dwDisplayType = 0;
            public int dwUsage = 0;
            public string lpLocalName = "";
            public string lpRemoteName = "";
            public string lpComment = "";
            public string lpProvider = "";
        }

        [DllImport("Mpr.dll")]
        private static extern int WNetUseConnection(IntPtr hwndOwner, NETRESOURCE lpNetResource, string lpPassword, string lpUserID, int dwFlags, string lpAccessName, string lpBufferSize, string lpResult);

        private void btnMapDrive_Click(object sender, EventArgs e)
        {
            string letter = cmbDrives.Text;

            if (letter.Length == 2 && letter[1] == ':')
            {
                NETRESOURCE res = new NETRESOURCE();

                res.dwType = 0x00000001;
                res.lpRemoteName = "\\\\127.0.0.1@" + Server.Settings.Port;
                res.lpLocalName = cmbDrives.Text;
                res.lpComment = "WebDAV Share for " + Server.Settings.Path;

                int error = WNetUseConnection(IntPtr.Zero, res, "", "", 0, null, null, null);
                if (error != 0)
                {
                    MessageBox.Show("Sorry, mapping the drive failed with error " + error, "Failed to connect");
                }
                else
                {
                    MessageBox.Show("Successfully mapped the network drive to " + res.lpLocalName, "Success");
                }
            }
            else
            {
                MessageBox.Show("Select a valid drive letter first", "Success");
            }

            UpdateDriveLetters();
        }

        private void txtPrefetch_TextChanged(object sender, EventArgs e)
        {
            int.TryParse(txtPrefetch.Text, out Server.Settings.PrefetchCount);
        }

        private void txtCacheTime_TextChanged(object sender, EventArgs e)
        {
            int.TryParse(txtCacheTime.Text, out Server.Settings.CacheTime);
        }

        private void chkShowJpeg_CheckedChanged(object sender, EventArgs e)
        {
            Server.Settings.ShowJpeg = chkShowJpeg.Checked;
        }

        private void chkShowInfos_CheckedChanged(object sender, EventArgs e)
        {
            Server.Settings.ShowInfos = chkShowInfos.Checked;
        }

        private void chkShowFits_CheckedChanged(object sender, EventArgs e)
        {
            Server.Settings.ShowFits = chkShowFits.Checked;
        }

        private void chkShowDng_CheckedChanged(object sender, EventArgs e)
        {
            Server.Settings.ShowDng = chkShowDng.Checked;
        }

        private void chkShowWav_CheckedChanged(object sender, EventArgs e)
        {
            Server.Settings.ShowWav = chkShowWav.Checked;
        }
    }
}
