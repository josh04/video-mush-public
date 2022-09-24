using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace mush
{
    public struct cameraConfigStruct
    {
        public void defaults()
        {
            position_x = 0.0f;
            position_y = 0.0f;
            position_z = 0.0f;
            position_theta = 0.0f;
            position_phi = 0.0f;

            width = 1280;
            height = 720;

            fov = 75.0f;
            speed = 1.0f;

            autocam = false;
            display_cam = false;

            load_path = "";
            save_path = "";

            model_path = "";

            model_scaling = 1.0f;

            stereo = false;
            equirectangular = false;
        }

        public float position_x;
        public float position_y;
        public float position_z;
        public float position_theta;
        public float position_phi;

        public int width;
        public int height;

        public float fov;
        public float speed;

        [MarshalAs(UnmanagedType.U1)]
        public bool autocam;
        [MarshalAs(UnmanagedType.U1)]
        public bool display_cam;

        public string load_path;
        public string save_path;

        public string model_path;

        public float model_scaling;

        [MarshalAs(UnmanagedType.U1)]
        public bool stereo;
        [MarshalAs(UnmanagedType.U1)]
        public bool equirectangular;

    }

}
