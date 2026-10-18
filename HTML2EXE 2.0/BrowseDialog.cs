
using System.Diagnostics;

namespace HTML2EXE_2
{
    public partial class BrowseDialog : Form
    {
        public ConfigDialog configDialog = new ConfigDialog();

        public BrowseDialog()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            Text = $"HTML2EXE 2.0 v{(HTML2EXE.IsBigBuild ? $"{HTML2EXE.CurrentVersion} (BIG)" : HTML2EXE.CurrentVersion)}";
        }

        private void selectFileBtn_Click(object sender, EventArgs e)
        {
            openFileDialog1.ShowDialog();
            string htmlPath = openFileDialog1.FileName;
            if (File.Exists(htmlPath))
            {
                // Copy the file's entire parent directory so all assets (images, CSS, JS, subfolders)
                // are included in the final EXE — not just the selected HTML file.
                // Safety guard: never copy a root/drive directory (e.g. C:\).
                string? parentDir = Path.GetDirectoryName(htmlPath);
                if (!string.IsNullOrEmpty(parentDir) && Path.GetPathRoot(parentDir) != parentDir)
                    HTML2EXE.CopyDirectory(parentDir, HTML2EXE.tmpWebfilesPath);
                else
                    File.Copy(htmlPath, Path.Combine(HTML2EXE.tmpWebfilesPath, Path.GetFileName(htmlPath)), true);

                Visible = false;
                configDialog.ShowDialog();
                Close(); // BUG-11 FIX: Only close after a successful selection, not on cancel
            }
            // If no file was selected the dialog stays open so the user can try again
        }

        private void selectFolderBtn_Click(object sender, EventArgs e)
        {
            folderBrowserDialog1.ShowDialog();
            string folderPath = folderBrowserDialog1.SelectedPath;

            if (Directory.Exists(folderPath))
            {
                HTML2EXE.CopyDirectory(folderPath, HTML2EXE.tmpWebfilesPath);

                Visible = false;
                configDialog.ShowDialog();
                Close(); // BUG-11 FIX: Only close after a successful selection, not on cancel
            }
            // If no folder was selected the dialog stays open so the user can try again
        }

        private void noFileBtn_Click_1(object sender, EventArgs e)
        {
            Visible = false;
            configDialog.ShowDialog();
            Close();
        }

        private void HeaderPanel_Click(object sender, EventArgs e)
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = "https://jgc777.github.io/HTML2EXE-2.0/",
                UseShellExecute = true
            });
        }
    }
}
