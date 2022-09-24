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
    public partial class RadeonProperties : Window
    {
        
        public mush.radeonConfigStruct get_config_struct(mush.cameraConfigStruct conf)
        {
            

            mush.radeonConfigStruct rc = new mush.radeonConfigStruct();
            rc.defaults();
            rc.width = conf.width;
            rc.height = conf.height; 

            try
            {
                rc.shadow_rays = Int32.Parse(amdShadowRays.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { rc.shadow_rays = 1; }
            try
            {
                rc.ao_rays = Int32.Parse(amdAORays.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { rc.shadow_rays = 1; }
            try
            {
                rc.num_bounces = Int32.Parse(amdBounces.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { rc.num_bounces = 5; }
            try
            {
                rc.num_samples = Int32.Parse(amdSamples.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { rc.num_samples = -1; }
            try
            {
                rc.ao_radius = float.Parse(amdAORadius.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { rc.ao_radius = 7.0f; }

            try
            {
                rc.camera_sensor_size[0] = float.Parse(amdSensorSize.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { rc.camera_sensor_size[0] = 0.036f; }

            rc.camera_sensor_size[1] = rc.height * rc.camera_sensor_size[0] / rc.width;
            
            float r_fov = conf.fov * (float)Math.PI / 180.0f;

            float dist = (rc.camera_sensor_size[1] / 2.0f) / (float)Math.Tan(r_fov / 2.0f);
            rc.camera_focal_length = dist;

            try
            {
                rc.camera_focus_distance = float.Parse(amdFocusDistance.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { rc.camera_focus_distance = 0.0f; }
            try
            {
                rc.camera_aperture = float.Parse(amdAperture.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { rc.camera_aperture = 0.0f; }

            rc.share_opencl = amdShareOpenCL.IsChecked.Value;
            rc.ao_enabled = amdEnableAO.IsChecked.Value ? 1 : 0;
            rc.progressive_enabled = 1;
            rc.environment_map_fish_eye = amdProgressive.IsChecked.Value;

            switch (amdCameraBox.SelectedIndex)
            {
                case 0:
                    rc.camera_type = mush.cameraType.perspective;
                    break;
                case 1:
                    rc.camera_type = mush.cameraType.perspectiveDOF;
                    break;
                case 2:
                    rc.camera_type = mush.cameraType.sphericalEquirectangular;
                    break;
            }

            try
            {
                FileInfo model = new FileInfo(conf.model_path);
                rc.model_name = model.Name;
                rc.path = model.Directory.FullName;
            } catch (System.ArgumentException e)
            {
                rc.model_name = "";
                rc.path = "";
            }
            rc.model_scale = conf.model_scaling;
            try
            {
                rc.environment_map_mult = float.Parse(amdEnvMapScale.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { rc.environment_map_mult = 1.0f; }

            rc.stereo_displacement = amdStereoDisplacement.IsChecked.Value;
            if (conf.autocam)
            {
                rc.automatic_camera_path = conf.load_path;
            } else
            {
                rc.automatic_camera_path = "";
            }
            rc.write_camera_path = conf.save_path;

            rc.quit_on_camera_path = amdPathQuit.IsChecked.Value;
            
            try
            {
                rc.max_frames = int.Parse(amdMaxFrames.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { rc.max_frames = 0; }

            rc.camera_position[0] = conf.position_x;
            rc.camera_position[1] = conf.position_y;
            rc.camera_position[2] = conf.position_z;

            return rc;
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

        public RadeonProperties()
        {
            InitializeComponent();

            //saveOutput_Checked(saveOutput, null);
            
        }
    }
}
