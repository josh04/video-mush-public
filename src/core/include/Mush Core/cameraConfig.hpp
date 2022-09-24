
#ifndef cameraConfig_h
#define cameraConfig_h

namespace mush {
	namespace core {
		struct cameraConfigStruct {
			void defaults()
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
                
				model_scale = 1.0f;

				stereo = false;

				equirectangular = false;
                
                quit_at_camera_path_end = false;
			}

			float position_x;
			float position_y;
			float position_z;
			float position_theta;
			float position_phi;

			int width;
			int height;

			float fov;
			float speed;

			bool autocam;
			bool display_cam;

			const char * load_path;
			const char * save_path;

			const char * model_path;
            
            float model_scale;

			bool stereo;

			bool equirectangular;
		
            bool quit_at_camera_path_end;
        };
	}
}

#endif
