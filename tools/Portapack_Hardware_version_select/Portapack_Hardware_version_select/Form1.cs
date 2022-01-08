using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.IO;
using System.Diagnostics;

namespace Portapack_Hardware_version_select
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
            Hardware_version.SelectedIndex = 0;//Default R1_20150901
            update.Checked = true;//Default update CPLD
        }

        private void File_Open_Click(object sender, EventArgs e)
        {
            OpenFileDialog file = new OpenFileDialog();
            file.Filter = "Binary Files(*.bin,*.raw)|*.bin;*.raw|All files (*.*)|*.*"; //Filter file types
            file.InitialDirectory = Application.StartupPath + "\\Temp\\";//Set initial directory
            file.ShowReadOnly = true; //Sets whether the file is read-only
            DialogResult r = file.ShowDialog();
            if (r == DialogResult.OK)
            {
                //Carry out subsequent treatment
                FileName.Clear();
                FileName.AppendText(file.FileName);
            }
        }

        private void Create_Click(object sender, EventArgs e)
        {
            if (FileName.Text == "")//Determine whether there is no file path
            {
                Log.Clear();
                Log.AppendText("Please select a file to configure！" + "\x0d\x0a");
            }
            else
            {
                if (File.Exists(FileName.Text))////Determine the existence of the file
                {
                    Log.Clear();
                    Log.AppendText("Configuration to file：" + "\x0d\x0a   " + Path.GetFileName(FileName.Text) + "\x0d\x0a");
                    //Log.AppendText(AppDomain.CurrentDomain.BaseDirectory + @"\mac.bin" + "\x0d\x0a");
                    //Log.AppendText(FileName.Text + "\x0d\x0a");
                    FileStream fs = new FileStream(FileName.Text, FileMode.Open, FileAccess.Write);//FileMode.Open, FileAccess.Write or FileAccess.Read
                    BinaryWriter gm = new BinaryWriter(fs); //Write configuration
                    Byte Addr = (byte)(0xA4 | Convert.ToInt32(update.Checked) << 3 | ((Hardware_version.SelectedIndex + 1) & 0x3) );

                    Log.AppendText("Set Hardware version：" + Hardware_version.Text + "\x0d\x0a");

                    gm.Seek(0xFFFFF, SeekOrigin.Current);
                    gm.Write(Addr);
                    gm.Close();
                    fs.Close();
                    Log.AppendText("Configuration write：OK" + "\x0d\x0a");
                }
            }
        }
    }
}
