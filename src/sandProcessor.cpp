//
//  sandProcesser.cpp
//  video-mush
//
//  Created by Josh McNamee on 05/09/2016.
//
//

#ifndef __APPLE__
#include "oculusVideoPlayer.hpp"
#include "oculusDraw.hpp"
#endif

#include <Mush Core/psnrProcess.hpp>
#include <Mush Core/ssimProcess.hpp>

#include "sandProcessor.hpp"
#include "remote-render/motionReprojection.hpp"
#include "delayProcess.hpp"
#include "anaglyphProcess.hpp"
#include "motionGenerator.hpp"
#include "sbsPackProcess.hpp"
#include "sbsUnpackProcess.hpp"
#include "videoAverageProcess.hpp"
#include "mseProcess.hpp"
#include <Mush Core/fixedExposureProcess.hpp>"
#include <Mush Core/mushRasteriser.hpp>

extern void SetThreadName(const char * threadName);

namespace mush {
    
    sandProcessor::sandProcessor(config::generatorProcessStruct g, unsigned int width, unsigned int height, const char * resource_dir, parConfigStruct par_config, mush::config config) : _width(width), _height(height), _resource_dir(resource_dir), _g(g), _par_config(par_config), _config(config) {
    }
    
    sandProcessor::~sandProcessor() {}
    
    void sandProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        
        steppers.push_back(make_shared<mush::frameStepper>());
        
        if (buffers.size() > 0) {
            _inputs.insert(_inputs.end(), buffers.begin(), buffers.end());
        }
        
        quitter = std::make_shared<mush::quitEventHandler>();
        videoMushAddEventHandler(quitter);
        
        switch (_g.type) {
            case generatorProcess::text:
                sand = make_shared<mush::textDrawProcess>(_g.text_output_string, _width, _height, _resource_dir, std::array<float, 4>{_g.bg_colour[0], _g.bg_colour[1], _g.bg_colour[2], _g.bg_colour[3]}, std::array<float, 4>{_g.text_colour[0], _g.text_colour[1], _g.text_colour[2], _g.text_colour[3]});
                sand->setTagInGuiName("Text");
                break;
            case generatorProcess::sand:
                sand = make_shared<sandProcess>(_width, _height);
                sand->setTagInGuiName("Sand");
                break;
            case generatorProcess::sphere:
            {
                auto s = make_shared<sphereMapProcess>(_width, _height, _par_config.camera_config.autocam, _par_config.camera_config.load_path, _par_config.camera_config.speed, _par_config.camera_config.quit_at_camera_path_end);
                sand = s;
                sand->setTagInGuiName("Sphere Map");
            }
                break;
#ifndef __APPLE__
			case generatorProcess::oculusVideo:
				sand = make_shared<oculusVideoPlayer>();
				sand->setTagInGuiName("Oculus 360 Video");
				break;
			case generatorProcess::oculusDraw:
				sand = make_shared<oculusDraw>();
				sand->setTagInGuiName("Oculus Direct");
				break;
#endif
			case generatorProcess::motionReprojection:
            {
				auto m = make_shared<motionReprojection>();
                //m->set_update_previous(true);
                sand = m;
				sand->setTagInGuiName("Motion Reproj Demo");
				_delay = make_shared<delayProcess>();
				_delay->setTagInGuiName("Delayed input");
				_delay->init(context, buffers.begin()[0]);
            }
                break;
            case generatorProcess::anaglyph:
                sand = make_shared<anaglyphProcess>();
                sand->setTagInGuiName("Anaglyph");
                break;
            case generatorProcess::motionGenerator:
                sand = make_shared<motionGenerator>(_par_config.camera_type == par_camera_type::spherical, _par_config.camera_config.load_path, _par_config.camera_config.speed);
                sand->setTagInGuiName("Motion Generator");
            
            motionReproj = std::make_shared<motionReprojection>();
            motionReproj->init(context, {buffers.begin()[1], sand});
            motionReproj->setTagInGuiName("Motion Reproj Demo");
            
            motionCopy = std::make_shared<mush::fixedExposureProcess>(0.0f);
            motionCopy->init(context, buffers.begin()[1]);
            motionCopy->setTagInGuiName("Left Copy");
                break;

            case generatorProcess::sbsPack:
                sand = make_shared<sbsPackProcess>();
                sand->setTagInGuiName("SBS Pack");
                break;
            
            case generatorProcess::sbsUnpack:
				sand = make_shared<sbsUnpackProcess>(false);
				sand->setTagInGuiName("SBS Unpack Left");
				unpackRight = make_shared<sbsUnpackProcess>(true);
				unpackRight->setTagInGuiName("SBS Unpack Right");
				unpackRight->init(context, buffers);
				break;

			case generatorProcess::mse:
				sand = std::make_shared<mush::videoAverageProcess>();
				//sand->init(context, buffers.begin()[0]);
				sand->setTagInGuiName("Video Average");
				break;

			case generatorProcess::raster:
			{
				sand = std::make_shared<mush::rasteriser>(_config.cameraConfig, _config.cameraConfig.stereo);
				//sand->init(context, buffers.begin()[0]);
				sand->setTagInGuiName("Raster Engine");
			}
				break;
        }
        sand->init(context, buffers);
        
        _guiBuffers.push_back(sand);
        //steppers[0]->toggle();

		if (_g.type == generatorProcess::mse) {
			mse = std::make_shared<mseProcess>();
			mse->init(context, { sand, buffers.begin()[0] });
			mse->setTagInGuiName("MSE");
			_guiBuffers.push_back(mse);
		}

		if (_delay != nullptr) {
			_guiBuffers.push_back(_delay);
		}
        
        if (motionReproj != nullptr) {
            _guiBuffers.push_back(motionReproj);
        }
        
        if (motionCopy != nullptr) {
            _guiBuffers.push_back(motionCopy);
        }
        
        if (unpackRight != nullptr) {
            _guiBuffers.push_back(unpackRight);
        }
    }
    
    void sandProcessor::process() {
        
        steppers[0]->process();
        
        sand->process();
        
        if (motionReproj != nullptr) {
            motionReproj->process();
        }
        if (motionCopy != nullptr) {
            motionCopy->process();
        }

		if (_delay != nullptr) {
			_delay->process();
        }
        
        if (unpackRight != nullptr) {
            unpackRight->process();
        }

		if (mse != nullptr) {
			mse->process();
		}
    }
    
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> sandProcessor::getBuffers() const {
		if (_g.type == generatorProcess::sbsUnpack) {
			return{ sand, unpackRight };
		} else if (_g.type == generatorProcess::mse) {
			return{ mse };
        } else {
            return {sand};
        }
    }
    
    std::vector<std::shared_ptr<mush::guiAccessible>> sandProcessor::getGuiBuffers() {
        const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
        _guiBuffers.clear();
        return buffs;
    }
    
    std::vector<std::shared_ptr<mush::frameStepper>> sandProcessor::getFrameSteppers() const {
        return steppers;
    }
    
    void sandProcessor::go() {
        SetThreadName("sand");
        
        while (!quitter->getQuit()) {
            process();
        }
        
        if (sand != nullptr) {
            sand->release();
        }
        
        if (motionReproj != nullptr) {
            motionReproj->release();
        }
        
        if (motionCopy != nullptr) {
            motionCopy->release();
        }

		if (_delay != nullptr) {
			_delay->release();
        }
        
        if (unpackRight != nullptr) {
            unpackRight->release();
        }

		if (mse != nullptr) {
			mse->release();
		}
        
        if (steppers[0] != nullptr) {
            steppers[0]->release();
        }
        
        for (auto& a : _inputs) {
            if (a != nullptr) {
                a->kill();
            }
        }
    }
    
    void sandProcessor::destroy() {
        running = false;
    }
    
}
