namespace WindowsFormsApplication
{
    partial class Form1
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
            this.btnNdPllng = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.btnUpdtSctrTrlrs = new System.Windows.Forms.Button();
            this.btnRd = new System.Windows.Forms.Button();
            this.btnWrt = new System.Windows.Forms.Button();
            this.btnStrtPllng = new System.Windows.Forms.Button();
            this.btnGtCSN = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // btnNdPllng
            // 
            this.btnNdPllng.Enabled = false;
            this.btnNdPllng.Location = new System.Drawing.Point(74, 313);
            this.btnNdPllng.Name = "btnNdPllng";
            this.btnNdPllng.Size = new System.Drawing.Size(137, 33);
            this.btnNdPllng.TabIndex = 2;
            this.btnNdPllng.Text = "End Polling";
            this.btnNdPllng.UseVisualStyleBackColor = true;
            this.btnNdPllng.Click += new System.EventHandler(this.btnNdPllng_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(19, 22);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(247, 13);
            this.label1.TabIndex = 3;
            this.label1.Text = "** polling is started automatically in Form1_Load() **";
            // 
            // btnUpdtSctrTrlrs
            // 
            this.btnUpdtSctrTrlrs.Enabled = false;
            this.btnUpdtSctrTrlrs.Location = new System.Drawing.Point(74, 109);
            this.btnUpdtSctrTrlrs.Name = "btnUpdtSctrTrlrs";
            this.btnUpdtSctrTrlrs.Size = new System.Drawing.Size(137, 33);
            this.btnUpdtSctrTrlrs.TabIndex = 4;
            this.btnUpdtSctrTrlrs.Text = "Update Sector Trailers";
            this.btnUpdtSctrTrlrs.UseVisualStyleBackColor = true;
            this.btnUpdtSctrTrlrs.Click += new System.EventHandler(this.btnUpdtSctrTrlrs_Click);
            // 
            // btnRd
            // 
            this.btnRd.Enabled = false;
            this.btnRd.Location = new System.Drawing.Point(74, 160);
            this.btnRd.Name = "btnRd";
            this.btnRd.Size = new System.Drawing.Size(137, 33);
            this.btnRd.TabIndex = 5;
            this.btnRd.Text = "Read";
            this.btnRd.UseVisualStyleBackColor = true;
            this.btnRd.Click += new System.EventHandler(this.btnRd_Click);
            // 
            // btnWrt
            // 
            this.btnWrt.Enabled = false;
            this.btnWrt.Location = new System.Drawing.Point(74, 211);
            this.btnWrt.Name = "btnWrt";
            this.btnWrt.Size = new System.Drawing.Size(137, 33);
            this.btnWrt.TabIndex = 6;
            this.btnWrt.Text = "Write";
            this.btnWrt.UseVisualStyleBackColor = true;
            this.btnWrt.Click += new System.EventHandler(this.btnWrt_Click);
            // 
            // btnStrtPllng
            // 
            this.btnStrtPllng.Enabled = false;
            this.btnStrtPllng.Location = new System.Drawing.Point(74, 58);
            this.btnStrtPllng.Name = "btnStrtPllng";
            this.btnStrtPllng.Size = new System.Drawing.Size(137, 33);
            this.btnStrtPllng.TabIndex = 7;
            this.btnStrtPllng.Text = "Start Polling";
            this.btnStrtPllng.UseVisualStyleBackColor = true;
            this.btnStrtPllng.Click += new System.EventHandler(this.btnStrtPllng_Click);
            // 
            // btnGtCSN
            // 
            this.btnGtCSN.Enabled = false;
            this.btnGtCSN.Location = new System.Drawing.Point(74, 262);
            this.btnGtCSN.Name = "btnGtCSN";
            this.btnGtCSN.Size = new System.Drawing.Size(137, 33);
            this.btnGtCSN.TabIndex = 8;
            this.btnGtCSN.Text = "Get CSN";
            this.btnGtCSN.UseVisualStyleBackColor = true;
            this.btnGtCSN.Click += new System.EventHandler(this.btnGtCSN_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(284, 381);
            this.Controls.Add(this.btnGtCSN);
            this.Controls.Add(this.btnStrtPllng);
            this.Controls.Add(this.btnWrt);
            this.Controls.Add(this.btnRd);
            this.Controls.Add(this.btnUpdtSctrTrlrs);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.btnNdPllng);
            this.Name = "Form1";
            this.Text = "Form1";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnNdPllng;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnUpdtSctrTrlrs;
        private System.Windows.Forms.Button btnRd;
        private System.Windows.Forms.Button btnWrt;
        private System.Windows.Forms.Button btnStrtPllng;
        private System.Windows.Forms.Button btnGtCSN;

    }
}

