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
    public partial class MessageWindow : Form
    {
        public MessageWindow(string title, string msg)
        {
            InitializeComponent();

            Text = title;
            textBox1.Text = msg.Replace("\r\n", "\n").Replace("\n", Environment.NewLine);
            textBox1.Select(0, 0);
        }
    }
}
