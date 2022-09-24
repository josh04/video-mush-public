using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace mush
{
    
    public enum scene_names : int
    {
        sibenik = 3,
        conference = 7,
        sponza = 11,
        generic_scenes = 15
    }
    public enum scene_generic_variable : int
    {
        kitchen = 3
    }

    public enum par_camera_type
    {
        perspective,
        spherical
    }
    public enum par_output_mode
    {
        just_render,
	    with_metadata,
	    dual_render,
	    dual_render_with_reprojection,
	    dual_render_with_metadata,
        four_output
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct parConfigStruct
    {
        public void defaults()
        {
            camera_config.defaults();
            gl_width = 1280;
            gl_height = 720;
            scene = scene_names.sponza; // sponza
            scene_variable = 1;
            iterations = 100;
            offset = 0;
            frame_count = 1000;
            use_remote_render_method = false;
            opengl_only = false;
            
            camera_type = par_camera_type.perspective;

            auto_cores = true;
            manual_cores = 3;
            model_dir_path = "./Models";

            just_get_metadata = false;

            place_camera = false;

            second_view = false;
            second_view_just_metadata = false;
            second_view_stereo = true;

            second_camera_x_diff = 0;
            second_camera_y_diff = 0;
            second_camera_z_diff = 0;
            second_camera_theta_diff = 0;
            second_camera_phi_diff = 0;
            second_camera_fov_diff = 90.0f;

            low_graphics_memory = false;

            output_mode = par_output_mode.with_metadata;
            use_360_player = false;
            final_snapshot = false;

            use_mush_model = false;

            draw_depths = true;
            draw_normals = true;
            draw_motion = true;

            four_output = false;

            flip_normals = false;
        }

        public cameraConfigStruct camera_config;
        
        public UInt32 gl_width;
        public UInt32 gl_height;

        public scene_names scene;
        public int scene_variable;

        public int iterations;

        public int offset;

        public int frame_count;

        [MarshalAs(UnmanagedType.U1)]
        public bool use_remote_render_method;

        [MarshalAs(UnmanagedType.U1)]
        public bool opengl_only;

        [MarshalAs(UnmanagedType.U1)]
        public bool automatic_camera_opengl_playback;

        public par_camera_type camera_type;

        [MarshalAs(UnmanagedType.U1)]
        public bool auto_cores;
        public UInt32 manual_cores;

        public string model_dir_path;

        [MarshalAs(UnmanagedType.U1)]
        public bool just_get_metadata;

        [MarshalAs(UnmanagedType.U1)]
        public bool place_camera;

        [MarshalAs(UnmanagedType.U1)]
        public bool second_view;

        [MarshalAs(UnmanagedType.U1)]
        public bool second_view_just_metadata;

        [MarshalAs(UnmanagedType.U1)]
        public bool second_view_stereo;

        public float second_camera_x_diff;
        public float second_camera_y_diff;
        public float second_camera_z_diff;
        public float second_camera_theta_diff;
        public float second_camera_phi_diff;
        public float second_camera_fov_diff;

        [MarshalAs(UnmanagedType.U1)]
        public bool low_graphics_memory;

        public par_output_mode output_mode;

        [MarshalAs(UnmanagedType.U1)]
        public bool use_360_player;
        [MarshalAs(UnmanagedType.U1)]
        public bool final_snapshot;

        [MarshalAs(UnmanagedType.U1)]
        public bool use_mush_model;

        [MarshalAs(UnmanagedType.U1)]
        public bool draw_depths;
        [MarshalAs(UnmanagedType.U1)]
        public bool draw_normals;
        [MarshalAs(UnmanagedType.U1)]
        public bool draw_motion;

        [MarshalAs(UnmanagedType.U1)]
        public bool four_output;

        [MarshalAs(UnmanagedType.U1)]
        public bool flip_normals;
    }

}
