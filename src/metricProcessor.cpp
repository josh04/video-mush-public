//
//  motionExplorer.cpp
//  video-mush
//
//  Created by Josh McNamee on 05/09/2016.
//
//


#include "metricProcessor.hpp"
#include <Mush Core/SetThreadName.hpp>


namespace mush {

	metricProcessor::metricProcessor() : mush::imageProcessor() {
	}

	metricProcessor::~metricProcessor() {}

	void metricProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {

		assert(buffers.size() == 2);
		_input = castToImage(buffers.begin()[0]);

		_input->setTagInGuiName("Compare");
		auto in2 = castToImage(buffers.begin()[1]);
		in2->setTagInGuiName("Original");

		steppers.push_back(make_shared<mush::frameStepper>());

		_psnr = std::make_shared<psnrProcess>(psnrProcess::type::linear, 1.0f);
		_psnr->init(context, buffers);
		_psnr->setTagInGuiName("PSNR");
		_lpsnr = std::make_shared<psnrProcess>(psnrProcess::type::log, 1.0f);
		_lpsnr->init(context, buffers);
        _lpsnr->setTagInGuiName("LogPSNR");
        _pupsnr = std::make_shared<psnrProcess>(psnrProcess::type::pu, 1.0f);
        _pupsnr->init(context, buffers);
        _pupsnr->setTagInGuiName("puPSNR");
        _ssim = std::make_shared<ssimProcess>(ssimProcess::type::linear);
		_ssim->init(context, buffers);
        _ssim->setTagInGuiName("SSIM");
        _pussim = std::make_shared<ssimProcess>(ssimProcess::type::pu);
        _pussim->init(context, buffers);
        _pussim->setTagInGuiName("puSSIM");

		_guiBuffers.push_back(_psnr);
        _guiBuffers.push_back(_lpsnr);
        _guiBuffers.push_back(_pupsnr);
        _guiBuffers.push_back(_ssim);
        _guiBuffers.push_back(_pussim);

		_timer = std::make_shared<timerWrapper>("");
		_timer->register_metric(_psnr, "PSNR");
        _timer->register_metric(_lpsnr, "logPSNR");
        _timer->register_metric(_pupsnr, "puPSNR");
        _timer->register_metric(_ssim, "SSIM");
        _timer->register_metric(_pussim, "puSSIM");

	}

	void metricProcessor::process() {

		steppers[0]->process();

		_timer->process_metric(_psnr);
        _timer->process_metric(_lpsnr);
        _timer->process_metric(_pupsnr);
        _timer->process_metric(_ssim);
        _timer->process_metric(_pussim);

		_timer->print_metered_report();
		if (!toggled) {
			toggled = true;
			//steppers[0]->toggle();
		}
	}


	const std::vector<std::shared_ptr<mush::ringBuffer>> metricProcessor::getBuffers() const {
		return{ _psnr, _pupsnr, _lpsnr, _ssim, _pussim };
	}

	std::vector<std::shared_ptr<mush::guiAccessible>> metricProcessor::getGuiBuffers() {
		const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
		_guiBuffers.clear();
		return buffs;
	}

	std::vector<std::shared_ptr<mush::frameStepper>> metricProcessor::getFrameSteppers() const {
		return steppers;
	}


	void metricProcessor::go() {
		SetThreadName("Metrics");

		_timer->print_header();

		while (_input->good()) {
			process();
		}

		_timer->print_final_report();

		if (_psnr != nullptr) {
			_psnr->release();
        }
        
        if (_pupsnr != nullptr) {
            _pupsnr->release();
        }

		if (_lpsnr != nullptr) {
			_lpsnr->release();
		}

		if (_ssim != nullptr) {
			_ssim->release();
        }
        
        if (_pussim != nullptr) {
            _pussim->release();
        }

		if (steppers[0] != nullptr) {
			steppers[0]->release();
		}
		/*
		for (auto& a : _inputs) {
			if (a != nullptr) {
				a->kill();
			}
		}
		*/
	}

}
