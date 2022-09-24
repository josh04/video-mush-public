//
//  motionExplorer.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_motionExplorer_hpp
#define video_mush_motionExplorer_hpp

#include <Mush Core/registerContainer.hpp>
#include <Mush Core/imageProcessor.hpp>
#include <thread>
#include <vector>

#include "ConfigStruct.hpp"

class singleMotionGenerator;
class motionReprojection;
class edgeEncodingProcessor;
class parProcessor;

namespace par {
    class rasteriser;
}

namespace mush {
    class quitEventHandler;
    class frameStepper;
    class timerWrapper;
    class ffmpegEncodeDecode;
	class parRasteriser;
    class imageProcess;
    class repeaterProcess;
    
	namespace camera {
		class base;
		class camera_event_handler;
	}
	class motionExplorer : public mush::imageProcessor {
	public:
		motionExplorer(const parConfigStruct& par_config, const config::motionExplorerStruct& mot_config);

		~motionExplorer();

		void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;

		void process() override;

		const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;

		std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;

		std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override;

		std::vector<std::shared_ptr<azure::Eventable>> getEventables() const override;

		void go() override;

		void destroy();

	private:
		bool running = true;
        
        std::shared_ptr<mush::imageBuffer> _original = nullptr;
        mush::registerContainer<mush::imageBuffer> _original_r;
        std::shared_ptr<mush::imageBuffer> _motion = nullptr;
        std::shared_ptr<mush::imageBuffer> _small = nullptr;
        std::shared_ptr<mush::imageBuffer> _depth = nullptr;

        std::shared_ptr<par::rasteriser> _gl = nullptr;
        std::shared_ptr<mush::imageProcess> _gl_d = nullptr;
		std::shared_ptr<mush::camera::camera_event_handler> _ev = nullptr;

        std::shared_ptr<motionReprojection> _reproject = nullptr;
        std::shared_ptr<singleMotionGenerator> _generator = nullptr;
		std::shared_ptr<mush::repeaterProcess> _repeater = nullptr;
        
        std::shared_ptr<edgeEncodingProcessor> _edge = nullptr;
        std::shared_ptr<mush::timerWrapper> _timer = nullptr;

		std::vector<std::shared_ptr<mush::frameStepper>> steppers;

		std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;

		std::shared_ptr<mush::quitEventHandler> quitter = nullptr;
        
        
        std::shared_ptr<mush::ffmpegEncodeDecode> _ffmpeg_depth = nullptr;
        
        std::shared_ptr<parProcessor> _par = nullptr;
        parConfigStruct _par_config;
		config::motionExplorerStruct _mot_config;
		bool _first_run = true;
		std::shared_ptr<mush::camera::base> _path_camera = nullptr;
		std::shared_ptr<mush::camera::camera_event_handler> _path_event_handler = nullptr;
	};
}

#endif
