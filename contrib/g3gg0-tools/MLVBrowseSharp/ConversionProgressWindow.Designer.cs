namespace MLVBrowseSharp
{
    partial class ConversionProgressWindow
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ConversionProgressWindow));
            this.panelItems = new System.Windows.Forms.FlowLayoutPanel();
            this.SuspendLayout();
            // 
            // panelItems
            // 
            this.panelItems.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelItems.Location = new System.Drawing.Point(0, 0);
            this.panelItems.Name = "panelItems";
            this.panelItems.Size = new System.Drawing.Size(364, 258);
            this.panelItems.TabIndex = 0;
            // 
            // ConversionProgressWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(364, 258);
            this.Controls.Add(this.panelItems);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(380, 120);
            this.Name = "ConversionProgressWindow";
            this.Text = "ConversionProgressWindow";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.FlowLayoutPanel panelItems;
    }
}