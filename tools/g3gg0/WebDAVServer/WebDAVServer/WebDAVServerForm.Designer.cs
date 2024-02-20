namespace WebDAVServer
{
    partial class WebDAVServerForm
    {
        /// <summary>
        /// Erforderliche Designervariable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Verwendete Ressourcen bereinigen.
        /// </summary>
        /// <param name="disposing">True, wenn verwaltete Ressourcen gelöscht werden sollen; andernfalls False.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Vom Windows Form-Designer generierter Code

        /// <summary>
        /// Erforderliche Methode für die Designerunterstützung.
        /// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WebDAVServerForm));
            this.txtLog = new System.Windows.Forms.TextBox();
            this.notifyIcon = new System.Windows.Forms.NotifyIcon(this.components);
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPageLog = new System.Windows.Forms.TabPage();
            this.tabPageAutostart = new System.Windows.Forms.TabPage();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.chkShowInfos = new System.Windows.Forms.CheckBox();
            this.chkShowJpeg = new System.Windows.Forms.CheckBox();
            this.txtCacheTime = new System.Windows.Forms.TextBox();
            this.txtPrefetch = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.label1 = new System.Windows.Forms.Label();
            this.cmbDrives = new System.Windows.Forms.ComboBox();
            this.btnMapDrive = new System.Windows.Forms.Button();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.btnPath = new System.Windows.Forms.Button();
            this.txtAuth = new System.Windows.Forms.TextBox();
            this.txtPort = new System.Windows.Forms.TextBox();
            this.txtPath = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.btnDeleteConfig = new System.Windows.Forms.Button();
            this.btnWriteConfig = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.btnStopService = new System.Windows.Forms.Button();
            this.btnStartService = new System.Windows.Forms.Button();
            this.btnUninstall = new System.Windows.Forms.Button();
            this.btnInstall = new System.Windows.Forms.Button();
            this.contextMenuToolbar = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.webDAVServerByG3gg0deToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator();
            this.ctxShow = new System.Windows.Forms.ToolStripMenuItem();
            this.ctxQuit = new System.Windows.Forms.ToolStripMenuItem();
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.chkShowFits = new System.Windows.Forms.CheckBox();
            this.chkShowDng = new System.Windows.Forms.CheckBox();
            this.chkShowWav = new System.Windows.Forms.CheckBox();
            this.tabControl1.SuspendLayout();
            this.tabPageLog.SuspendLayout();
            this.tabPageAutostart.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.contextMenuToolbar.SuspendLayout();
            this.SuspendLayout();
            // 
            // txtLog
            // 
            this.txtLog.Dock = System.Windows.Forms.DockStyle.Fill;
            this.txtLog.Font = new System.Drawing.Font("Courier New", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtLog.Location = new System.Drawing.Point(3, 3);
            this.txtLog.Multiline = true;
            this.txtLog.Name = "txtLog";
            this.txtLog.ReadOnly = true;
            this.txtLog.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtLog.Size = new System.Drawing.Size(538, 273);
            this.txtLog.TabIndex = 0;
            // 
            // notifyIcon
            // 
            this.notifyIcon.Icon = ((System.Drawing.Icon)(resources.GetObject("notifyIcon.Icon")));
            this.notifyIcon.Text = "g3gg0.de WebDAVServer";
            // 
            // tabControl1
            // 
            this.tabControl1.Appearance = System.Windows.Forms.TabAppearance.FlatButtons;
            this.tabControl1.Controls.Add(this.tabPageLog);
            this.tabControl1.Controls.Add(this.tabPageAutostart);
            this.tabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tabControl1.Location = new System.Drawing.Point(0, 0);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(552, 308);
            this.tabControl1.TabIndex = 1;
            // 
            // tabPageLog
            // 
            this.tabPageLog.Controls.Add(this.txtLog);
            this.tabPageLog.Location = new System.Drawing.Point(4, 25);
            this.tabPageLog.Name = "tabPageLog";
            this.tabPageLog.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageLog.Size = new System.Drawing.Size(544, 279);
            this.tabPageLog.TabIndex = 0;
            this.tabPageLog.Text = "Log Messages";
            this.tabPageLog.UseVisualStyleBackColor = true;
            // 
            // tabPageAutostart
            // 
            this.tabPageAutostart.Controls.Add(this.groupBox5);
            this.tabPageAutostart.Controls.Add(this.groupBox4);
            this.tabPageAutostart.Controls.Add(this.groupBox3);
            this.tabPageAutostart.Controls.Add(this.textBox1);
            this.tabPageAutostart.Controls.Add(this.groupBox2);
            this.tabPageAutostart.Controls.Add(this.groupBox1);
            this.tabPageAutostart.Location = new System.Drawing.Point(4, 25);
            this.tabPageAutostart.Name = "tabPageAutostart";
            this.tabPageAutostart.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageAutostart.Size = new System.Drawing.Size(544, 279);
            this.tabPageAutostart.TabIndex = 1;
            this.tabPageAutostart.Text = "Setup";
            this.tabPageAutostart.UseVisualStyleBackColor = true;
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.chkShowWav);
            this.groupBox5.Controls.Add(this.chkShowDng);
            this.groupBox5.Controls.Add(this.chkShowFits);
            this.groupBox5.Controls.Add(this.chkShowInfos);
            this.groupBox5.Controls.Add(this.chkShowJpeg);
            this.groupBox5.Controls.Add(this.txtCacheTime);
            this.groupBox5.Controls.Add(this.txtPrefetch);
            this.groupBox5.Controls.Add(this.label5);
            this.groupBox5.Controls.Add(this.label4);
            this.groupBox5.Location = new System.Drawing.Point(339, 114);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Size = new System.Drawing.Size(196, 157);
            this.groupBox5.TabIndex = 7;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "MLV Options";
            // 
            // chkShowInfos
            // 
            this.chkShowInfos.AutoSize = true;
            this.chkShowInfos.Location = new System.Drawing.Point(9, 128);
            this.chkShowInfos.Name = "chkShowInfos";
            this.chkShowInfos.Size = new System.Drawing.Size(79, 17);
            this.chkShowInfos.TabIndex = 2;
            this.chkShowInfos.Text = "Show Infos";
            this.chkShowInfos.UseVisualStyleBackColor = true;
            this.chkShowInfos.CheckedChanged += new System.EventHandler(this.chkShowInfos_CheckedChanged);
            // 
            // chkShowJpeg
            // 
            this.chkShowJpeg.AutoSize = true;
            this.chkShowJpeg.Location = new System.Drawing.Point(92, 82);
            this.chkShowJpeg.Name = "chkShowJpeg";
            this.chkShowJpeg.Size = new System.Drawing.Size(46, 17);
            this.chkShowJpeg.TabIndex = 2;
            this.chkShowJpeg.Text = "JPG";
            this.chkShowJpeg.UseVisualStyleBackColor = true;
            this.chkShowJpeg.CheckedChanged += new System.EventHandler(this.chkShowJpeg_CheckedChanged);
            // 
            // txtCacheTime
            // 
            this.txtCacheTime.Location = new System.Drawing.Point(120, 55);
            this.txtCacheTime.Name = "txtCacheTime";
            this.txtCacheTime.Size = new System.Drawing.Size(70, 20);
            this.txtCacheTime.TabIndex = 1;
            this.txtCacheTime.TextChanged += new System.EventHandler(this.txtCacheTime_TextChanged);
            // 
            // txtPrefetch
            // 
            this.txtPrefetch.Location = new System.Drawing.Point(120, 25);
            this.txtPrefetch.Name = "txtPrefetch";
            this.txtPrefetch.Size = new System.Drawing.Size(70, 20);
            this.txtPrefetch.TabIndex = 1;
            this.txtPrefetch.TextChanged += new System.EventHandler(this.txtPrefetch_TextChanged);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(6, 58);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(74, 13);
            this.label5.TabIndex = 0;
            this.label5.Text = "Cache time [s]";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(6, 28);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(83, 13);
            this.label4.TabIndex = 0;
            this.label4.Text = "Prefetch images";
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.label1);
            this.groupBox4.Controls.Add(this.cmbDrives);
            this.groupBox4.Controls.Add(this.btnMapDrive);
            this.groupBox4.Location = new System.Drawing.Point(339, 7);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(196, 100);
            this.groupBox4.TabIndex = 6;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Map Drive";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 33);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(37, 13);
            this.label1.TabIndex = 7;
            this.label1.Text = "Letter:";
            // 
            // cmbDrives
            // 
            this.cmbDrives.FormattingEnabled = true;
            this.cmbDrives.Location = new System.Drawing.Point(74, 30);
            this.cmbDrives.Name = "cmbDrives";
            this.cmbDrives.Size = new System.Drawing.Size(50, 21);
            this.cmbDrives.TabIndex = 6;
            this.cmbDrives.DropDown += new System.EventHandler(this.cmbDrives_DropDown);
            // 
            // btnMapDrive
            // 
            this.btnMapDrive.Location = new System.Drawing.Point(6, 57);
            this.btnMapDrive.Name = "btnMapDrive";
            this.btnMapDrive.Size = new System.Drawing.Size(118, 23);
            this.btnMapDrive.TabIndex = 5;
            this.btnMapDrive.Text = "Map";
            this.btnMapDrive.UseVisualStyleBackColor = true;
            this.btnMapDrive.Click += new System.EventHandler(this.btnMapDrive_Click);
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.btnPath);
            this.groupBox3.Controls.Add(this.txtAuth);
            this.groupBox3.Controls.Add(this.txtPort);
            this.groupBox3.Controls.Add(this.txtPath);
            this.groupBox3.Controls.Add(this.label3);
            this.groupBox3.Controls.Add(this.label2);
            this.groupBox3.Location = new System.Drawing.Point(8, 6);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(185, 106);
            this.groupBox3.TabIndex = 4;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Config";
            // 
            // btnPath
            // 
            this.btnPath.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btnPath.Location = new System.Drawing.Point(9, 22);
            this.btnPath.Margin = new System.Windows.Forms.Padding(0);
            this.btnPath.Name = "btnPath";
            this.btnPath.Size = new System.Drawing.Size(48, 23);
            this.btnPath.TabIndex = 2;
            this.btnPath.Text = "Path:";
            this.btnPath.UseVisualStyleBackColor = true;
            this.btnPath.Click += new System.EventHandler(this.btnPath_Click);
            // 
            // txtAuth
            // 
            this.txtAuth.Location = new System.Drawing.Point(66, 74);
            this.txtAuth.Name = "txtAuth";
            this.txtAuth.Size = new System.Drawing.Size(112, 20);
            this.txtAuth.TabIndex = 1;
            this.txtAuth.TextChanged += new System.EventHandler(this.txtAuth_TextChanged);
            // 
            // txtPort
            // 
            this.txtPort.Location = new System.Drawing.Point(66, 48);
            this.txtPort.Name = "txtPort";
            this.txtPort.Size = new System.Drawing.Size(112, 20);
            this.txtPort.TabIndex = 1;
            this.txtPort.TextChanged += new System.EventHandler(this.txtPort_TextChanged);
            // 
            // txtPath
            // 
            this.txtPath.Location = new System.Drawing.Point(66, 22);
            this.txtPath.Name = "txtPath";
            this.txtPath.Size = new System.Drawing.Size(112, 20);
            this.txtPath.TabIndex = 1;
            this.txtPath.TextChanged += new System.EventHandler(this.txtPath_TextChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(19, 77);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(32, 13);
            this.label3.TabIndex = 0;
            this.label3.Text = "Auth:";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(19, 50);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(29, 13);
            this.label2.TabIndex = 0;
            this.label2.Text = "Port:";
            // 
            // textBox1
            // 
            this.textBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.textBox1.Cursor = System.Windows.Forms.Cursors.Arrow;
            this.textBox1.Enabled = false;
            this.textBox1.Location = new System.Drawing.Point(30, 148);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.ReadOnly = true;
            this.textBox1.Size = new System.Drawing.Size(135, 54);
            this.textBox1.TabIndex = 3;
            this.textBox1.Text = "Note:\r\nWhen installing as a service, you will not see any icon or log window.";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.btnDeleteConfig);
            this.groupBox2.Controls.Add(this.btnWriteConfig);
            this.groupBox2.Location = new System.Drawing.Point(197, 6);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(135, 101);
            this.groupBox2.TabIndex = 1;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Default Config";
            // 
            // btnDeleteConfig
            // 
            this.btnDeleteConfig.Location = new System.Drawing.Point(6, 58);
            this.btnDeleteConfig.Name = "btnDeleteConfig";
            this.btnDeleteConfig.Size = new System.Drawing.Size(120, 23);
            this.btnDeleteConfig.TabIndex = 0;
            this.btnDeleteConfig.Text = "Delete";
            this.btnDeleteConfig.UseVisualStyleBackColor = true;
            this.btnDeleteConfig.Click += new System.EventHandler(this.btnDeleteConfig_Click);
            // 
            // btnWriteConfig
            // 
            this.btnWriteConfig.Location = new System.Drawing.Point(7, 29);
            this.btnWriteConfig.Name = "btnWriteConfig";
            this.btnWriteConfig.Size = new System.Drawing.Size(120, 23);
            this.btnWriteConfig.TabIndex = 0;
            this.btnWriteConfig.Text = "Write";
            this.btnWriteConfig.UseVisualStyleBackColor = true;
            this.btnWriteConfig.Click += new System.EventHandler(this.btnWriteConfig_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.btnStopService);
            this.groupBox1.Controls.Add(this.btnStartService);
            this.groupBox1.Controls.Add(this.btnUninstall);
            this.groupBox1.Controls.Add(this.btnInstall);
            this.groupBox1.Location = new System.Drawing.Point(199, 113);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(133, 142);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Service";
            // 
            // btnStopService
            // 
            this.btnStopService.Location = new System.Drawing.Point(7, 108);
            this.btnStopService.Name = "btnStopService";
            this.btnStopService.Size = new System.Drawing.Size(120, 23);
            this.btnStopService.TabIndex = 1;
            this.btnStopService.Text = "Stop";
            this.btnStopService.UseVisualStyleBackColor = true;
            this.btnStopService.Click += new System.EventHandler(this.btnStopService_Click);
            // 
            // btnStartService
            // 
            this.btnStartService.Location = new System.Drawing.Point(7, 79);
            this.btnStartService.Name = "btnStartService";
            this.btnStartService.Size = new System.Drawing.Size(120, 23);
            this.btnStartService.TabIndex = 1;
            this.btnStartService.Text = "Start";
            this.btnStartService.UseVisualStyleBackColor = true;
            this.btnStartService.Click += new System.EventHandler(this.btnStartService_Click);
            // 
            // btnUninstall
            // 
            this.btnUninstall.Location = new System.Drawing.Point(7, 49);
            this.btnUninstall.Name = "btnUninstall";
            this.btnUninstall.Size = new System.Drawing.Size(120, 23);
            this.btnUninstall.TabIndex = 0;
            this.btnUninstall.Text = "Uninstall";
            this.btnUninstall.UseVisualStyleBackColor = true;
            this.btnUninstall.Click += new System.EventHandler(this.btnUninstall_Click);
            // 
            // btnInstall
            // 
            this.btnInstall.Location = new System.Drawing.Point(7, 20);
            this.btnInstall.Name = "btnInstall";
            this.btnInstall.Size = new System.Drawing.Size(120, 23);
            this.btnInstall.TabIndex = 0;
            this.btnInstall.Text = "Install";
            this.btnInstall.UseVisualStyleBackColor = true;
            this.btnInstall.Click += new System.EventHandler(this.btnInstall_Click);
            // 
            // contextMenuToolbar
            // 
            this.contextMenuToolbar.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.webDAVServerByG3gg0deToolStripMenuItem,
            this.toolStripMenuItem1,
            this.ctxShow,
            this.ctxQuit});
            this.contextMenuToolbar.Name = "contextMenuToolbar";
            this.contextMenuToolbar.Size = new System.Drawing.Size(222, 76);
            // 
            // webDAVServerByG3gg0deToolStripMenuItem
            // 
            this.webDAVServerByG3gg0deToolStripMenuItem.Enabled = false;
            this.webDAVServerByG3gg0deToolStripMenuItem.Name = "webDAVServerByG3gg0deToolStripMenuItem";
            this.webDAVServerByG3gg0deToolStripMenuItem.Size = new System.Drawing.Size(221, 22);
            this.webDAVServerByG3gg0deToolStripMenuItem.Text = "WebDAVServer by g3gg0.de";
            // 
            // toolStripMenuItem1
            // 
            this.toolStripMenuItem1.Name = "toolStripMenuItem1";
            this.toolStripMenuItem1.Size = new System.Drawing.Size(218, 6);
            // 
            // ctxShow
            // 
            this.ctxShow.Name = "ctxShow";
            this.ctxShow.Size = new System.Drawing.Size(221, 22);
            this.ctxShow.Text = "Show Menu";
            this.ctxShow.Click += new System.EventHandler(this.ctxShow_Click);
            // 
            // ctxQuit
            // 
            this.ctxQuit.Name = "ctxQuit";
            this.ctxQuit.Size = new System.Drawing.Size(221, 22);
            this.ctxQuit.Text = "Quit";
            this.ctxQuit.Click += new System.EventHandler(this.ctxQuit_Click);
            // 
            // progressBar
            // 
            this.progressBar.Location = new System.Drawing.Point(350, 5);
            this.progressBar.Maximum = 1000;
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(189, 15);
            this.progressBar.TabIndex = 5;
            this.progressBar.Visible = false;
            // 
            // chkShowFits
            // 
            this.chkShowFits.AutoSize = true;
            this.chkShowFits.Location = new System.Drawing.Point(9, 105);
            this.chkShowFits.Name = "chkShowFits";
            this.chkShowFits.Size = new System.Drawing.Size(49, 17);
            this.chkShowFits.TabIndex = 3;
            this.chkShowFits.Text = "FITS";
            this.chkShowFits.UseVisualStyleBackColor = true;
            this.chkShowFits.CheckedChanged += new System.EventHandler(this.chkShowFits_CheckedChanged);
            // 
            // chkShowDng
            // 
            this.chkShowDng.AutoSize = true;
            this.chkShowDng.Location = new System.Drawing.Point(9, 82);
            this.chkShowDng.Name = "chkShowDng";
            this.chkShowDng.Size = new System.Drawing.Size(50, 17);
            this.chkShowDng.TabIndex = 4;
            this.chkShowDng.Text = "DNG";
            this.chkShowDng.UseVisualStyleBackColor = true;
            this.chkShowDng.CheckedChanged += new System.EventHandler(this.chkShowDng_CheckedChanged);
            // 
            // chkShowWav
            // 
            this.chkShowWav.AutoSize = true;
            this.chkShowWav.Location = new System.Drawing.Point(92, 105);
            this.chkShowWav.Name = "chkShowWav";
            this.chkShowWav.Size = new System.Drawing.Size(51, 17);
            this.chkShowWav.TabIndex = 5;
            this.chkShowWav.Text = "WAV";
            this.chkShowWav.UseVisualStyleBackColor = true;
            this.chkShowWav.CheckedChanged += new System.EventHandler(this.chkShowWav_CheckedChanged);
            // 
            // WebDAVServerForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(552, 308);
            this.Controls.Add(this.progressBar);
            this.Controls.Add(this.tabControl1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "WebDAVServerForm";
            this.Text = "[g3gg0.de] MLV WebDAVServer";
            this.tabControl1.ResumeLayout(false);
            this.tabPageLog.ResumeLayout(false);
            this.tabPageLog.PerformLayout();
            this.tabPageAutostart.ResumeLayout(false);
            this.tabPageAutostart.PerformLayout();
            this.groupBox5.ResumeLayout(false);
            this.groupBox5.PerformLayout();
            this.groupBox4.ResumeLayout(false);
            this.groupBox4.PerformLayout();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.contextMenuToolbar.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox txtLog;
        private System.Windows.Forms.NotifyIcon notifyIcon;
        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabPageLog;
        private System.Windows.Forms.TabPage tabPageAutostart;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Button btnDeleteConfig;
        private System.Windows.Forms.Button btnWriteConfig;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button btnUninstall;
        private System.Windows.Forms.Button btnInstall;
        private System.Windows.Forms.Button btnStopService;
        private System.Windows.Forms.Button btnStartService;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.Button btnPath;
        private System.Windows.Forms.TextBox txtAuth;
        private System.Windows.Forms.TextBox txtPort;
        private System.Windows.Forms.TextBox txtPath;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ContextMenuStrip contextMenuToolbar;
        private System.Windows.Forms.ToolStripMenuItem webDAVServerByG3gg0deToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem ctxShow;
        private System.Windows.Forms.ToolStripMenuItem ctxQuit;
        private System.Windows.Forms.ProgressBar progressBar;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox cmbDrives;
        private System.Windows.Forms.Button btnMapDrive;
        private System.Windows.Forms.GroupBox groupBox5;
        private System.Windows.Forms.CheckBox chkShowInfos;
        private System.Windows.Forms.CheckBox chkShowJpeg;
        private System.Windows.Forms.TextBox txtCacheTime;
        private System.Windows.Forms.TextBox txtPrefetch;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.CheckBox chkShowFits;
        private System.Windows.Forms.CheckBox chkShowWav;
        private System.Windows.Forms.CheckBox chkShowDng;
    }
}

