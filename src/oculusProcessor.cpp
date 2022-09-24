//
//  sandProcesser.cpp
//  video-mush
//
//  Created by Josh McNamee on 05/09/2016.
//
//

#ifdef __APPLE__
#include <RadeonLibrary/radeonProcessor.hpp>
#include <parFramework/parProcessor.hpp>
#else
#include <App/Mush/radeonProcessor.hpp>
#include <PARtner/parProcessor.hpp>
#endif

#ifndef __APPLE__
#include "oculusVideoPlayer.hpp"
#include "oculusDraw.hpp"
#endif
#include <Mush Core/sphereMapProcess.hpp>

#include <Mush Core/fixedExposureProcess.hpp>
#include "oculusProcessor.hpp"

extern void SetThreadName(const char * threadName);

namespace mush {

	oculusProcessor::oculusProcessor(oculus_draw_source source, const parConfigStruct& p, const radeonConfig& r) : _par_config(p), _radeon_config(r), _source(source) {
	
	}

	oculusProcessor::~oculusProcessor() {}

	void oculusProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {

		steppers.push_back(make_shared<mush::frameStepper>());

		if (buffers.size() > 0) {
			_inputs.insert(_inputs.end(), buffers.begin(), buffers.end());
		}

		quitter = std::make_shared<mush::quitEventHandler>();
		videoMushAddEventHandler(quitter);
		
#ifdef _WIN32
		_oculus_draw = make_shared<oculusDraw>();
		_oculus_draw->setTagInGuiName("Oculus Draw");

#else
		_oculus_draw = make_shared<fixedExposureProcess>(0.0);
		_oculus_draw->setTagInGuiName("Oculus Draw");
#endif

		_guiBuffers.push_back(_oculus_draw);

		switch (_source) {
		case oculus_draw_source::par:
		{
			_par_config.output_mode = par_output_mode::dual_render;

			_par_processor = std::make_shared<parProcessor>(_par_config, true);
			_par_processor->init(context, buffers);

			auto par_processor_outputs = _par_processor->getBuffers();

			_oculus_draw->init(context, { par_processor_outputs[0], par_processor_outputs[1] });

			auto par_gui = _par_processor->getGuiBuffers();
			_guiBuffers.insert(_guiBuffers.end(), par_gui.begin(), par_gui.end());
		}
		break;
		case oculus_draw_source::amd:
		{
			_par_processor = std::make_shared<radeonProcessor>(_radeon_config, true);
			_par_processor->init(context, buffers);

			auto par_processor_outputs = _par_processor->getBuffers();
			_oculus_draw->init(context, { par_processor_outputs[0] });

			auto par_gui = _par_processor->getGuiBuffers();
			_guiBuffers.insert(_guiBuffers.end(), par_gui.begin(), par_gui.end());
		}
			break;
		case oculus_draw_source::direct:
			_oculus_draw->init(context, buffers);
			break;
		}

	}

	void oculusProcessor::process() {
		steppers[0]->process();

		_oculus_draw->process();
	}


	const std::vector<std::shared_ptr<mush::ringBuffer>> oculusProcessor::getBuffers() const {
		return{ _oculus_draw };
	}

	std::vector<std::shared_ptr<mush::guiAccessible>> oculusProcessor::getGuiBuffers() {
		const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
		_guiBuffers.clear();
		return buffs;
	}

	std::vector<std::shared_ptr<mush::frameStepper>> oculusProcessor::getFrameSteppers() const {
		return steppers;
	}

	std::vector<std::shared_ptr<azure::Eventable>> oculusProcessor::getEventables() const {
		return _par_processor->getEventables();
	}

	void oculusProcessor::go() {
		SetThreadName("oculus");

        std::thread par_thread(&imageProcessor::startThread, _par_processor);
#ifdef _WIN32
		std::thread view_thread(&oculusDraw::view_thread, std::dynamic_pointer_cast<oculusDraw>(_oculus_draw));
#endif

		while (!quitter->getQuit()) {
			process();
		}

		if (_oculus_draw != nullptr) {
			_oculus_draw->release();
		}

		if (steppers[0] != nullptr) {
			steppers[0]->release();
		}

		for (auto& a : _inputs) {
			if (a != nullptr) {
				a->kill();
			}
		}

		par_thread.join();
        
#ifdef _WIN32
		view_thread.join();
#endif
	}

	void oculusProcessor::destroy() {
		running = false;
	}

}
