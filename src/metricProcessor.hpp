//
//  motionExplorer.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_metricProcessor_hpp
#define video_mush_metricProcessor_hpp

//#include "exports.hpp"
#include <Mush Core/quitEventHandler.hpp>
#include <Mush Core/frameStepper.hpp>
#include <Mush Core/imageProcessor.hpp>
#include <Mush Core/psnrProcess.hpp>
#include <Mush Core/ssimProcess.hpp>
#include <Mush Core/timerWrapper.hpp>
#include <thread>
#include <vector>

namespace mush {
	namespace camera {
		class base;
		class camera_event_handler;
	}
	class metricProcessor : public mush::imageProcessor {
	public:
		metricProcessor();

		~metricProcessor();

		void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;

		void process() override;

		const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;

		std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;

		std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override;

		void go() override;


	private:

		std::shared_ptr<mush::imageBuffer> _input = nullptr;

		std::shared_ptr <mush::psnrProcess> _psnr = nullptr;
        std::shared_ptr <mush::psnrProcess> _lpsnr = nullptr;
        std::shared_ptr <mush::psnrProcess> _pupsnr = nullptr;
        std::shared_ptr <mush::ssimProcess> _ssim = nullptr;
        std::shared_ptr <mush::ssimProcess> _pussim = nullptr;
		std::shared_ptr <mush::timerWrapper> _timer = nullptr;

		std::vector<std::shared_ptr<mush::frameStepper>> steppers;

		std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;


		bool toggled = false;

	};
}

#endif
