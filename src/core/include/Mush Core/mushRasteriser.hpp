#ifndef MUSH_RASTERISER
#define MUSH_RASTERISER

#include "mush-core-dll.hpp"
#include "openglProcess.hpp"
#include "registerContainer.hpp"
#include <memory>

#include "cameraConfig.hpp"

namespace mush {
	namespace raster {
		class engine;
		class cubeMapDrawer;
        class sphere;
	}

	class MUSHEXPORTS_API rasteriser : public openglProcess {
	public:
		rasteriser(const core::cameraConfigStruct& c, bool draw_sbs_stereo);
		~rasteriser();

		void release() override;

		void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;

		void process() override;
		void process_depth();

		std::shared_ptr<azure::Eventable> get_eventable() const override;
        
        void enable_depth_program();
        void disable_depth_program();
        
        void enable_spherical();
        void disable_spherical();
        
        std::shared_ptr<mush::raster::engine> get_engine() const {
            return _engine;
        }
        
        void inUnlock() override;
        
        void enable_auto_camera();
        void disable_auto_camera();
        
        void single_auto_camera_tick() {
            _single_auto_camera_tick = true;
        }

	protected:
        void create_cube_map();
		void draw_cube_map();

		cl::Kernel * _pack = nullptr;
        cl::Image2D * _left = nullptr;
        cl::ImageGL * _main = nullptr;
        std::shared_ptr<azure::Framebuffer> _frame_main = nullptr;
        cl::ImageGL * _depth = nullptr;
        std::shared_ptr<azure::Framebuffer> _frame_depth = nullptr;
		std::shared_ptr<mush::raster::engine> _engine = nullptr;
		core::cameraConfigStruct _config;
		bool _draw_sbs_stereo;
        bool _use_depth_program = false;
        bool _created_cube_map = false;
        bool _use_cube_map = false;
        
        bool _enable_auto_camera = true;
        std::atomic_bool _single_auto_camera_tick;

		std::shared_ptr<azure::Framebuffer> _cube_map = nullptr;
		std::shared_ptr<raster::cubeMapDrawer> _cube_map_drawer = nullptr;
        
        mush::registerContainer<mush::imageBuffer> _input = nullptr;
        std::shared_ptr<mush::raster::sphere> _sphere = nullptr;
        cl::ImageGL * _input_texture = nullptr;
        
	};
}

#endif

