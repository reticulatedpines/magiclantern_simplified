namespace TimerGen
{
    partial class TimerGenForm
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
            ScanThread.Abort();

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
            this.label1 = new System.Windows.Forms.Label();
            this.txtBaseFreq = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.txtFps = new System.Windows.Forms.TextBox();
            this.btnGenerate = new System.Windows.Forms.Button();
            this.cmbModel = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.txtResults = new System.Windows.Forms.TextBox();
            this.lstResults = new System.Windows.Forms.ListBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.lblMLPath = new System.Windows.Forms.Label();
            this.lblMLVersion = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.btnWriteConfig = new System.Windows.Forms.Button();
            this.lstTimerValues = new System.Windows.Forms.ListBox();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(16, 58);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(58, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Base Freq:";
            // 
            // txtBaseFreq
            // 
            this.txtBaseFreq.Font = new System.Drawing.Font("Courier New", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtBaseFreq.Location = new System.Drawing.Point(90, 55);
            this.txtBaseFreq.Name = "txtBaseFreq";
            this.txtBaseFreq.Size = new System.Drawing.Size(146, 20);
            this.txtBaseFreq.TabIndex = 2;
            this.txtBaseFreq.Text = "28800000";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(16, 84);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(69, 13);
            this.label2.TabIndex = 0;
            this.label2.Text = "Desired FPS:";
            // 
            // txtFps
            // 
            this.txtFps.Font = new System.Drawing.Font("Courier New", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtFps.Location = new System.Drawing.Point(90, 81);
            this.txtFps.Name = "txtFps";
            this.txtFps.Size = new System.Drawing.Size(146, 20);
            this.txtFps.TabIndex = 3;
            this.txtFps.Text = "25";
            // 
            // btnGenerate
            // 
            this.btnGenerate.Location = new System.Drawing.Point(275, 12);
            this.btnGenerate.Name = "btnGenerate";
            this.btnGenerate.Size = new System.Drawing.Size(135, 146);
            this.btnGenerate.TabIndex = 5;
            this.btnGenerate.Text = "Generate";
            this.btnGenerate.UseVisualStyleBackColor = true;
            this.btnGenerate.Click += new System.EventHandler(this.btnGenerate_Click);
            // 
            // cmbModel
            // 
            this.cmbModel.Font = new System.Drawing.Font("Courier New", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.cmbModel.FormattingEnabled = true;
            this.cmbModel.Location = new System.Drawing.Point(90, 28);
            this.cmbModel.Name = "cmbModel";
            this.cmbModel.Size = new System.Drawing.Size(146, 22);
            this.cmbModel.TabIndex = 1;
            this.cmbModel.Text = "<select>";
            this.cmbModel.SelectedIndexChanged += new System.EventHandler(this.cmbModel_SelectedIndexChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(16, 31);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(39, 13);
            this.label3.TabIndex = 0;
            this.label3.Text = "Model:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(16, 110);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(63, 13);
            this.label4.TabIndex = 0;
            this.label4.Text = "Max results:";
            // 
            // txtResults
            // 
            this.txtResults.Font = new System.Drawing.Font("Courier New", 8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtResults.Location = new System.Drawing.Point(90, 107);
            this.txtResults.Name = "txtResults";
            this.txtResults.Size = new System.Drawing.Size(146, 20);
            this.txtResults.TabIndex = 4;
            this.txtResults.Text = "10";
            // 
            // lstResults
            // 
            this.lstResults.Font = new System.Drawing.Font("Courier New", 7F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lstResults.FormattingEnabled = true;
            this.lstResults.ItemHeight = 12;
            this.lstResults.Location = new System.Drawing.Point(9, 19);
            this.lstResults.Name = "lstResults";
            this.lstResults.Size = new System.Drawing.Size(188, 184);
            this.lstResults.TabIndex = 6;
            this.lstResults.SelectedIndexChanged += new System.EventHandler(this.lstResults_SelectedIndexChanged);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.cmbModel);
            this.groupBox1.Controls.Add(this.txtBaseFreq);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.label4);
            this.groupBox1.Controls.Add(this.txtResults);
            this.groupBox1.Controls.Add(this.txtFps);
            this.groupBox1.Location = new System.Drawing.Point(12, 12);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(257, 146);
            this.groupBox1.TabIndex = 6;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Parameters";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.lblMLPath);
            this.groupBox2.Controls.Add(this.lblMLVersion);
            this.groupBox2.Controls.Add(this.label5);
            this.groupBox2.Controls.Add(this.btnWriteConfig);
            this.groupBox2.Controls.Add(this.lstTimerValues);
            this.groupBox2.Controls.Add(this.lstResults);
            this.groupBox2.Location = new System.Drawing.Point(12, 164);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(398, 281);
            this.groupBox2.TabIndex = 7;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Results";
            // 
            // lblMLPath
            // 
            this.lblMLPath.AutoSize = true;
            this.lblMLPath.Font = new System.Drawing.Font("Courier New", 7F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblMLPath.Location = new System.Drawing.Point(20, 252);
            this.lblMLPath.Name = "lblMLPath";
            this.lblMLPath.Size = new System.Drawing.Size(11, 12);
            this.lblMLPath.TabIndex = 8;
            this.lblMLPath.Text = "-";
            // 
            // lblMLVersion
            // 
            this.lblMLVersion.AutoSize = true;
            this.lblMLVersion.Font = new System.Drawing.Font("Courier New", 7F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblMLVersion.Location = new System.Drawing.Point(20, 239);
            this.lblMLVersion.Name = "lblMLVersion";
            this.lblMLVersion.Size = new System.Drawing.Size(11, 12);
            this.lblMLVersion.TabIndex = 8;
            this.lblMLVersion.Text = "-";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(6, 219);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(78, 13);
            this.label5.TabIndex = 8;
            this.label5.Text = "Magic Lantern:";
            // 
            // btnWriteConfig
            // 
            this.btnWriteConfig.Enabled = false;
            this.btnWriteConfig.Location = new System.Drawing.Point(203, 209);
            this.btnWriteConfig.Name = "btnWriteConfig";
            this.btnWriteConfig.Size = new System.Drawing.Size(189, 23);
            this.btnWriteConfig.TabIndex = 7;
            this.btnWriteConfig.Text = "Write ML Config";
            this.btnWriteConfig.UseVisualStyleBackColor = true;
            this.btnWriteConfig.Click += new System.EventHandler(this.btnWriteConfig_Click);
            // 
            // lstTimerValues
            // 
            this.lstTimerValues.Font = new System.Drawing.Font("Courier New", 7F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lstTimerValues.FormattingEnabled = true;
            this.lstTimerValues.ItemHeight = 12;
            this.lstTimerValues.Location = new System.Drawing.Point(203, 19);
            this.lstTimerValues.Name = "lstTimerValues";
            this.lstTimerValues.Size = new System.Drawing.Size(189, 184);
            this.lstTimerValues.TabIndex = 6;
            this.lstTimerValues.SelectedIndexChanged += new System.EventHandler(this.lstTimerValues_SelectedIndexChanged);
            // 
            // TimerGenForm
            // 
            this.AcceptButton = this.btnGenerate;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(422, 455);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.btnGenerate);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.Name = "TimerGenForm";
            this.Text = "EOS Video Timer Generator v2.0 (by g3gg0.de)";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox txtBaseFreq;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox txtFps;
        private System.Windows.Forms.Button btnGenerate;
        private System.Windows.Forms.ComboBox cmbModel;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox txtResults;
        private System.Windows.Forms.ListBox lstResults;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.ListBox lstTimerValues;
        private System.Windows.Forms.Button btnWriteConfig;
        private System.Windows.Forms.Label lblMLPath;
        private System.Windows.Forms.Label lblMLVersion;
        private System.Windows.Forms.Label label5;
    }
}

