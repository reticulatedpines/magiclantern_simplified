namespace MLVBrowseSharp
{
    partial class BrowseForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(BrowseForm));
            this.treeView = new System.Windows.Forms.TreeView();
            this.splitContainer2 = new System.Windows.Forms.SplitContainer();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.chkAnimation = new System.Windows.Forms.CheckBox();
            this.trackSize = new System.Windows.Forms.TrackBar();
            this.label1 = new System.Windows.Forms.Label();
            this.cmbGrouping = new System.Windows.Forms.ComboBox();
            this.chkPreview = new System.Windows.Forms.CheckBox();
            this.mlvFileList = new MLVBrowseSharp.MLVFileList();
            this.splitContainer2.Panel1.SuspendLayout();
            this.splitContainer2.Panel2.SuspendLayout();
            this.splitContainer2.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackSize)).BeginInit();
            this.SuspendLayout();
            // 
            // treeView
            // 
            this.treeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.treeView.Font = new System.Drawing.Font("Arial", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.treeView.HotTracking = true;
            this.treeView.Location = new System.Drawing.Point(0, 0);
            this.treeView.Name = "treeView";
            this.treeView.Size = new System.Drawing.Size(261, 610);
            this.treeView.TabIndex = 0;
            this.treeView.AfterCollapse += new System.Windows.Forms.TreeViewEventHandler(this.treeView_AfterCollapse);
            this.treeView.BeforeExpand += new System.Windows.Forms.TreeViewCancelEventHandler(this.treeView_BeforeExpand);
            this.treeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView_AfterSelect);
            // 
            // splitContainer2
            // 
            this.splitContainer2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer2.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer2.Location = new System.Drawing.Point(0, 0);
            this.splitContainer2.Name = "splitContainer2";
            // 
            // splitContainer2.Panel1
            // 
            this.splitContainer2.Panel1.Controls.Add(this.treeView);
            // 
            // splitContainer2.Panel2
            // 
            this.splitContainer2.Panel2.Controls.Add(this.splitContainer1);
            this.splitContainer2.Size = new System.Drawing.Size(925, 610);
            this.splitContainer2.SplitterDistance = 261;
            this.splitContainer2.TabIndex = 0;
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.IsSplitterFixed = true;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.chkPreview);
            this.splitContainer1.Panel1.Controls.Add(this.chkAnimation);
            this.splitContainer1.Panel1.Controls.Add(this.trackSize);
            this.splitContainer1.Panel1.Controls.Add(this.label1);
            this.splitContainer1.Panel1.Controls.Add(this.cmbGrouping);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.mlvFileList);
            this.splitContainer1.Size = new System.Drawing.Size(660, 610);
            this.splitContainer1.SplitterDistance = 34;
            this.splitContainer1.TabIndex = 1;
            // 
            // chkAnimation
            // 
            this.chkAnimation.AutoSize = true;
            this.chkAnimation.Location = new System.Drawing.Point(442, 7);
            this.chkAnimation.Name = "chkAnimation";
            this.chkAnimation.Size = new System.Drawing.Size(64, 17);
            this.chkAnimation.TabIndex = 4;
            this.chkAnimation.Text = "Animate";
            this.chkAnimation.UseVisualStyleBackColor = true;
            this.chkAnimation.CheckedChanged += new System.EventHandler(this.chkAnimation_CheckedChanged);
            // 
            // trackSize
            // 
            this.trackSize.LargeChange = 50;
            this.trackSize.Location = new System.Drawing.Point(310, 2);
            this.trackSize.Maximum = 400;
            this.trackSize.Minimum = 20;
            this.trackSize.Name = "trackSize";
            this.trackSize.Size = new System.Drawing.Size(125, 45);
            this.trackSize.SmallChange = 10;
            this.trackSize.TabIndex = 3;
            this.trackSize.TickFrequency = 20;
            this.trackSize.Value = 150;
            this.trackSize.ValueChanged += new System.EventHandler(this.trackSize_ValueChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 8);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(50, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "Group by";
            // 
            // cmbGrouping
            // 
            this.cmbGrouping.FormattingEnabled = true;
            this.cmbGrouping.Location = new System.Drawing.Point(59, 5);
            this.cmbGrouping.Name = "cmbGrouping";
            this.cmbGrouping.Size = new System.Drawing.Size(154, 21);
            this.cmbGrouping.TabIndex = 2;
            this.cmbGrouping.TextChanged += new System.EventHandler(this.cmbGrouping_TextChanged);
            // 
            // chkPreview
            // 
            this.chkPreview.AutoSize = true;
            this.chkPreview.Checked = true;
            this.chkPreview.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkPreview.Location = new System.Drawing.Point(219, 7);
            this.chkPreview.Name = "chkPreview";
            this.chkPreview.Size = new System.Drawing.Size(88, 17);
            this.chkPreview.TabIndex = 5;
            this.chkPreview.Text = "Preview size:";
            this.chkPreview.UseVisualStyleBackColor = true;
            this.chkPreview.CheckedChanged += new System.EventHandler(this.chkPreview_CheckedChanged);
            // 
            // mlvFileList
            // 
            this.mlvFileList.Dock = System.Windows.Forms.DockStyle.Fill;
            this.mlvFileList.Location = new System.Drawing.Point(0, 0);
            this.mlvFileList.Name = "mlvFileList";
            this.mlvFileList.Size = new System.Drawing.Size(660, 572);
            this.mlvFileList.TabIndex = 1;
            // 
            // BrowseForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(925, 610);
            this.Controls.Add(this.splitContainer2);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "BrowseForm";
            this.Text = "MLV Browser";
            this.splitContainer2.Panel1.ResumeLayout(false);
            this.splitContainer2.Panel2.ResumeLayout(false);
            this.splitContainer2.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.trackSize)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TreeView treeView;
        private System.Windows.Forms.SplitContainer splitContainer2;
        private MLVFileList mlvFileList;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox cmbGrouping;
        private System.Windows.Forms.TrackBar trackSize;
        private System.Windows.Forms.CheckBox chkAnimation;
        private System.Windows.Forms.CheckBox chkPreview;
    }
}

