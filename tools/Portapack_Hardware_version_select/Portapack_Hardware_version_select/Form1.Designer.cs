namespace Portapack_Hardware_version_select
{
    partial class Form1
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要修改
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.FileName = new System.Windows.Forms.TextBox();
            this.File_Open = new System.Windows.Forms.Button();
            this.Hardware_version = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.Create = new System.Windows.Forms.Button();
            this.Log = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.update = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(131, 12);
            this.label1.TabIndex = 0;
            this.label1.Text = "Select firmware file:";
            // 
            // FileName
            // 
            this.FileName.Location = new System.Drawing.Point(14, 24);
            this.FileName.Name = "FileName";
            this.FileName.Size = new System.Drawing.Size(159, 21);
            this.FileName.TabIndex = 1;
            // 
            // File_Open
            // 
            this.File_Open.Location = new System.Drawing.Point(181, 22);
            this.File_Open.Name = "File_Open";
            this.File_Open.Size = new System.Drawing.Size(75, 23);
            this.File_Open.TabIndex = 2;
            this.File_Open.Text = "...";
            this.File_Open.UseVisualStyleBackColor = true;
            this.File_Open.Click += new System.EventHandler(this.File_Open_Click);
            // 
            // Hardware_version
            // 
            this.Hardware_version.FormattingEnabled = true;
            this.Hardware_version.Items.AddRange(new object[] {
            "R1_20150901",
            "R2_20170522"});
            this.Hardware_version.Location = new System.Drawing.Point(14, 63);
            this.Hardware_version.Name = "Hardware_version";
            this.Hardware_version.Size = new System.Drawing.Size(159, 20);
            this.Hardware_version.TabIndex = 3;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 48);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(149, 12);
            this.label2.TabIndex = 4;
            this.label2.Text = "Select Hardware version:";
            // 
            // Create
            // 
            this.Create.Location = new System.Drawing.Point(181, 61);
            this.Create.Name = "Create";
            this.Create.Size = new System.Drawing.Size(75, 23);
            this.Create.TabIndex = 5;
            this.Create.Text = "Create";
            this.Create.UseVisualStyleBackColor = true;
            this.Create.Click += new System.EventHandler(this.Create_Click);
            // 
            // Log
            // 
            this.Log.Location = new System.Drawing.Point(12, 129);
            this.Log.Multiline = true;
            this.Log.Name = "Log";
            this.Log.Size = new System.Drawing.Size(240, 80);
            this.Log.TabIndex = 6;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 114);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(29, 12);
            this.label3.TabIndex = 7;
            this.label3.Text = "Log:";
            // 
            // update
            // 
            this.update.AutoSize = true;
            this.update.Location = new System.Drawing.Point(14, 89);
            this.update.Name = "update";
            this.update.Size = new System.Drawing.Size(168, 16);
            this.update.TabIndex = 9;
            this.update.Text = "Update To Portapack CPLD";
            this.update.UseVisualStyleBackColor = true;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(264, 219);
            this.Controls.Add(this.update);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.Log);
            this.Controls.Add(this.Create);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.Hardware_version);
            this.Controls.Add(this.File_Open);
            this.Controls.Add(this.FileName);
            this.Controls.Add(this.label1);
            this.Name = "Form1";
            this.Text = "Portapack_Hardware_version_select";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox FileName;
        private System.Windows.Forms.Button File_Open;
        private System.Windows.Forms.ComboBox Hardware_version;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button Create;
        private System.Windows.Forms.TextBox Log;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.CheckBox update;
    }
}

