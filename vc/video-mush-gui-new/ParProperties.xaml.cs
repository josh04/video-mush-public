using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using System.Runtime.InteropServices;
using System.Globalization;
using System.IO;
using System.Threading;

namespace video_mush_gui_new
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class ParProperties : Window
    {
        
        public mush.parConfigStruct get_config_struct()
        {
            
            mush.scene_names scene = mush.scene_names.sponza; // sponza
            int scene_variable = 1;

            if (sponzaButton.IsChecked == true)
            {
                scene = mush.scene_names.sponza;
            }

            if (sibenikButton.IsChecked == true)
            {
                scene = mush.scene_names.sibenik;
            }

            if (officeButton.IsChecked == true)
            {
                scene = mush.scene_names.conference;
            }

            if (kitchenButton.IsChecked == true)
            {
                scene = mush.scene_names.generic_scenes;
                scene_variable = (int)mush.scene_generic_variable.kitchen;
            }


            mush.parConfigStruct pc = new mush.parConfigStruct();
            pc.defaults();

            try
            {
                pc.iterations = Int32.Parse(iterationsBox.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.iterations = 1; }
           
            pc.iterations = Math.Max(pc.iterations, 1);

            try
            {
                pc.offset = Int32.Parse(offsetBox.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.offset = 0; }

            try
            {
                pc.frame_count = Int32.Parse(countBox.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.frame_count = 10000; }
            pc.scene = scene;
            pc.scene_variable = scene_variable;
            
            pc.use_remote_render_method = false;
            pc.opengl_only = false;
            pc.automatic_camera_opengl_playback = false;
            if (openglCheck.IsChecked == true)
            {
                pc.opengl_only = true;
                pc.automatic_camera_opengl_playback = true;
            }
            
            try
            {
                pc.gl_width = uint.Parse(glWidthBox.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.gl_width = 1280; }

            try
            {
                pc.gl_height = uint.Parse(glHeightBox.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.gl_height = 720; }

            switch (parCameraTypeBox.SelectedIndex)
            {
                case 0:
                    pc.camera_type = mush.par_camera_type.perspective;
                    break;
                case 1:
                    pc.camera_type = mush.par_camera_type.spherical;
                    break;
            }

            pc.auto_cores = parCoresCheck.IsChecked.Value;

            try
            {
                pc.manual_cores = uint.Parse(parCores.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.manual_cores = 3; }

            pc.just_get_metadata = parJustGetMetadata.IsChecked.Value;

            pc.place_camera = parCameraSet.IsChecked.Value;
            
            pc.second_view = parCameraSecond.IsChecked.Value;
            pc.second_view_just_metadata = parCameraSecondJustMetadata.IsChecked.Value;

            try
            {
                pc.second_camera_x_diff = float.Parse(second_camera_x.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.second_camera_x_diff = 0.0f; }

            try
            {
                pc.second_camera_y_diff = float.Parse(second_camera_y.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.second_camera_y_diff = 0.0f; }

            try
            {
                pc.second_camera_z_diff = float.Parse(second_camera_z.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.second_camera_z_diff = 0.0f; }

            try
            {
                pc.second_camera_theta_diff = float.Parse(second_camera_theta.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.second_camera_theta_diff = 0.0f; }

            try
            {
                pc.second_camera_phi_diff = float.Parse(second_camera_phi.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.second_camera_phi_diff = 0.0f; }

            try
            {
                pc.second_camera_fov_diff = float.Parse(second_camera_fov.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { pc.second_camera_fov_diff = 90.0f; }

            pc.low_graphics_memory = parLowMem.IsChecked.Value;
            pc.use_360_player = par360Viewer.IsChecked.Value;

            pc.final_snapshot = parFinalSnapshot.IsChecked.Value;

            pc.use_mush_model = parMushModel.IsChecked.Value;
            
            pc.four_output = fourOutputs.IsChecked.Value;
            if (pc.four_output)
            {
                pc.second_view = true;
                pc.output_mode = mush.par_output_mode.four_output;
            }

            pc.flip_normals = parFlipNormals.IsChecked.Value;
            return pc;
        }

        private void commonFileDialogMaker(ComboBox combo, UInt32 chooseNum, bool wantsFolder, String extension, bool isSave)
        {

            if (combo.SelectedIndex == chooseNum)
            {
                Microsoft.WindowsAPICodePack.Dialogs.CommonFileDialog dialog;

                if (isSave == true)
                {
                    dialog = new Microsoft.WindowsAPICodePack.Dialogs.CommonSaveFileDialog();
                }
                else
                {
                    dialog = new Microsoft.WindowsAPICodePack.Dialogs.CommonOpenFileDialog();
                    ((Microsoft.WindowsAPICodePack.Dialogs.CommonOpenFileDialog)dialog).IsFolderPicker = wantsFolder;
                }

                dialog.DefaultExtension = extension;

                Microsoft.WindowsAPICodePack.Dialogs.CommonFileDialogResult ans = dialog.ShowDialog();

                // Process open file dialog box results 
                if (ans == Microsoft.WindowsAPICodePack.Dialogs.CommonFileDialogResult.Ok)
                {
                    String val;
                    if (wantsFolder == true)
                    {
                        val = Directory.Exists(dialog.FileName) ? dialog.FileName : System.IO.Path.GetDirectoryName(dialog.FileName);
                    }
                    else
                    {
                        val = dialog.FileName;
                    }
                    ComboBoxItem itm = new ComboBoxItem(val, val);
                    combo.Items.Add(itm);
                    combo.SelectedItem = itm;
                }
                else
                {
                    combo.SelectedIndex = 0;
                }
            }
        }
      
        bool override_closed = false;

        public void OverrideClose()
        {
            override_closed = true;
            Close();
        }

        // Disclaimer, untested! 
        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            if (!override_closed)
            {
                e.Cancel = true;  // cancels the window close    
                this.Hide();      // Programmatically hides the window
            }
        }

        public ParProperties()
        {
            InitializeComponent();

            //saveOutput_Checked(saveOutput, null);
            //autoCam_Checked(autoCam, null);
            
        }

        private void parCameraSecondStereo_Checked(object sender, RoutedEventArgs e)
        {
            if (parCameraSecondStereo.IsChecked.Value == true)
            {
                second_camera_x.IsEnabled = false;
                second_camera_y.IsEnabled = false;
                second_camera_z.IsEnabled = false;
                second_camera_theta.IsEnabled = false;
                second_camera_phi.IsEnabled = false;
                second_camera_fov.IsEnabled = false;
            } else
            {
                second_camera_x.IsEnabled = true;
                second_camera_y.IsEnabled = true;
                second_camera_z.IsEnabled = true;
                second_camera_theta.IsEnabled = true;
                second_camera_phi.IsEnabled = true;
                second_camera_fov.IsEnabled = true;
            }
        }
    }
}
