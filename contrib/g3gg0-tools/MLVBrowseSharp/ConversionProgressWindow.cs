using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace MLVBrowseSharp
{
    public partial class ConversionProgressWindow : Form
    {
        public static ConversionProgressWindow Instance = null;

        public ConversionProgressWindow()
        {
            InitializeComponent();
            Instance = this;
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            Instance = null;
            base.OnClosing(e);
        }

        internal void AddItem(ConversionProgressItem item)
        {
            panelItems.Controls.Add(item);
        }
    }
}
