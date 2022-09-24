//
//  sandProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_oculusProcessor_hpp
#define video_mush_oculusProcessor_hpp

#include "exports.hpp"
#include <Mush Core/quitEventHandler.hpp>
#include "nullProcess.hpp"
#include <Mush Core/frameStepper.hpp>
#include "sandProcess.hpp"
#include "textDrawProcess.hpp"
#include <Mush Core/sphereMapProcess.hpp>
#include <Mush Core/imageProcessor.hpp>
#include <thread>
#include <vector>

namespace mush {
	class oculusProcessor : public mush::imageProcessor {
	public:
		oculusProcessor(oculus_draw_source source, const parConfigStruct& p, const radeonConfig& r);

		~oculusProcessor();

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

		std::vector<std::shared_ptr<mush::ringBuffer>> _inputs;

		std::shared_ptr <mush::imageProcess> _oculus_draw = nullptr;

		std::vector<std::shared_ptr<mush::frameStepper>> steppers;

		std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;

		unsigned int _width = 1280, _height = 720;

		std::shared_ptr<mush::quitEventHandler> quitter = nullptr;

		parConfigStruct _par_config;
		radeonConfig _radeon_config;
		std::shared_ptr<imageProcessor> _par_processor = nullptr;

		oculus_draw_source _source;

	};
}

#endif
