using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;


namespace mush
{
    public enum cameraType
    {
        perspective,
        perspectiveDOF,
        sphericalEquirectangular
    }
    public struct radeonConfigStruct
    {
        public void defaults()
        {

            width = 1280;
            height = 720;

            share_opencl = true;

            path = "bmw";
            model_name = "i8.obj";

            shadow_rays = 1;
            ao_rays = 5;
            ao_enabled = 1;
            progressive_enabled = 0;
            num_bounces = 5;
            num_samples = -1;

            ao_radius = 7.0f;

            camera_position = new float[4] { 0.0f, 0.0f, 0.0f, 0.0f };
            //camera_position = new float[4] { 0.0f, 0.0f, 0.0f, 0.0f };

            camera_sensor_size = new float[2] { 0.036f, 0.024f };  // default full frame sensor 36x24 mm
            camera_zcap = new float[2] { 0.0f, 100000.0f };
            camera_focal_length = 0.018f; // 35mm lens
            camera_focus_distance = 0.0f;
            camera_aperture = 0.0f;
            
            environment_map_mult = 1.0f;
            environment_map_path = "Textures";
            environment_map_name = "studio015.hdr";

            camera_type = cameraType.perspective;
            model_scale = 1.0f;

            environment_map_fish_eye = false;

            stereo_displacement = false;
            stereo_distance = 0.63f;
            automatic_camera_path = "";
            write_camera_path = "";

            max_frames = -1;

            auto_camera_frame_offset = 0;
        }


        public int width;
        public int height;

        [MarshalAs(UnmanagedType.U1)]
        public bool share_opencl;

        public string path;
        public string model_name;

        public int shadow_rays;
        public int ao_rays;
        public int ao_enabled;
        public int progressive_enabled;
        public int num_bounces;
        public int num_samples;

        public float ao_radius;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] camera_position;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
        public float[] camera_sensor_size;  // default full frame sensor 36x24 mm

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
        public float[] camera_zcap;

        public float camera_focal_length; // 35mm lens
        public float camera_focus_distance;
        public float camera_aperture;

        public float environment_map_mult;
        public string environment_map_path;
        public string environment_map_name;

        public cameraType camera_type;

        public float model_scale;

        [MarshalAs(UnmanagedType.U1)]
        public bool environment_map_fish_eye;

        [MarshalAs(UnmanagedType.U1)]
        public bool stereo_displacement;
        public float stereo_distance;

        public string automatic_camera_path;
        public string write_camera_path;
        
        [MarshalAs(UnmanagedType.U1)]
        public bool quit_on_camera_path;

        public int max_frames;

        public int auto_camera_frame_offset;
    }

}
