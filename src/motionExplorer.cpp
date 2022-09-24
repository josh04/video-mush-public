//
//  motionExplorer.cpp
//  video-mush
//
//  Created by Josh McNamee on 05/09/2016.
//
//

#ifndef __APPLE__
#include "oculusVideoPlayer.hpp"
#include "oculusDraw.hpp"
#endif

#include <sstream>
#include <Mush Core/opencl.hpp>
#include <Mush Core/frameStepper.hpp>
#include "motionExplorer.hpp"
#include "remote-render/motionReprojection.hpp"
#include "singleMotionGenerator.hpp"
#include "remote-render/edgeEncodingProcessor.hpp"
#include "delayProcess.hpp"
#include "motionGenerator.hpp"
#include <Mush Core/camera_base.hpp>
#include <Mush Core/camera_event_handler.hpp>
#include <Mush Core/quitEventHandler.hpp>
#include <Mush Core/timerWrapper.hpp>
#include <Mush Core/fixedExposureProcess.hpp>
#include <Mush Core/rasteriserEngine.hpp>
#include "ffmpegEncodeDecode.hpp"
#include "repeaterProcess.hpp"

#ifdef _WIN32
#include <PARtner/parProcessor.hpp>
#include <PARtner/parRasteriser2.hpp>
#endif
#ifdef __APPLE__
#include <ParFramework/parProcessor.hpp>
#include <ParFramework/parRasteriser2.hpp>
#endif

extern void SetThreadName(const char * threadName);

namespace mush {

    motionExplorer::motionExplorer(const parConfigStruct& par_config, const config::motionExplorerStruct& mot_config) : _par_config(par_config), _mot_config(mot_config) {
	}

	motionExplorer::~motionExplorer() {}

	void motionExplorer::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() >= 1);
        
		steppers.push_back(make_shared<mush::frameStepper>());

        _original = castToImage(buffers.begin()[0]);
        _original_r = _original;
        //_depth = castToImage(buffers.begin()[1]);
        if (buffers.size() > 1) {
            _small = castToImage(buffers.begin()[1]);
        }
        
		quitter = std::make_shared<mush::quitEventHandler>();
		//videoMushAddEventHandler(quitter);

        
        _timer = std::make_shared<mush::timerWrapper>("");
        
        /*
        _edge = std::make_shared<edgeEncodingProcessor>(5.1f, _timer, "");
        
        _edge->init(context, {_depth});
        
        auto ebuf = _edge->getBuffers();
        
        
        _ffmpeg_depth = std::make_shared<mush::ffmpegEncodeDecode>(avcodec_codec::x265, mush::transfer::pq, 24);
        
        _ffmpeg_depth->init(context, {_depth});
        
        _timer->register_metric(_ffmpeg_depth->get_encoder(), "Packet Size (depth)");
        */
        
        
        _par_config.opengl_only = true;
        unsigned int width, height, size;
        _original->getParams(width, height, size);
        
        _par_config.gl_width = width;
        _par_config.gl_height = height;
        _par_config.camera_type = par_camera_type::perspective;
        

        parConfigStruct _par_config2 = _par_config;
        
        if (!_mot_config.spherical) {
            _par_config2.camera_config.equirectangular = false;
        }
        _par_config2.camera_config.autocam = false;
        _par = std::make_shared<parProcessor>(_par_config2, false);
        _par->init(context, {});
		
        auto par_b = _par->getBuffers();
        
        _gl = std::dynamic_pointer_cast<par::rasteriser>(par_b[0]);
		_ev = std::dynamic_pointer_cast<mush::camera::camera_event_handler>(_gl->get_eventable());

        //_gl->get_camera()->move_camera({0.0, 0.0, 0.0}, 0.0, 0.0);
        //_gl->get_camera()->set_fov(75.0);
		_gl->get_camera()->move_camera({_par_config.camera_config.position_x, _par_config.camera_config.position_y, _par_config.camera_config.position_z}, _par_config.camera_config.position_theta, _par_config.camera_config.position_phi);
		_gl->get_camera()->set_fov(_par_config.camera_config.fov);
        _gl_d = _par->get_depth();

		_repeater = std::make_shared<repeaterProcess>();
        _repeater->init(context, {_gl_d});
        _repeater->setTagInGuiName("In Depth");
		
        _generator = std::make_shared<singleMotionGenerator>(_par_config.camera_config.equirectangular, _par_config.camera_config.position_x, _par_config.camera_config.position_y, _par_config.camera_config.position_z, _par_config.camera_config.position_theta, _par_config.camera_config.position_phi, _par_config.camera_config.fov, (_mot_config.spherical && !_par_config.camera_config.equirectangular));
        _generator->init(context, { _gl_d });
        _generator->setTagInGuiName("Motion Gen");
        
        _reproject = make_shared<motionReprojection>();
        _reproject->set_update_previous(true);
        if (_small == nullptr) {
            _reproject->init(context, {_original, _generator, _gl, _repeater, _gl });
        } else {
            _reproject->init(context, {_original, _generator, _small, _repeater, _gl });
        }
        
        _timer->register_node(_gl, "GL");
        _timer->register_node(_gl_d, "GL Depth");
		_timer->register_node(_generator, "Generator");
        _timer->register_node(_reproject, "Reprojection");

        //_timer->register_node(_edge, "Edge Perf");
        
        if (_par_config.camera_config.autocam) {
            _path_camera = std::make_shared<mush::camera::base>();
            _path_event_handler = std::make_shared<mush::camera::camera_event_handler>(_path_camera);
            _path_event_handler->load_camera_path(_par_config.camera_config.load_path, _par_config.camera_config.speed, false, _par_config.camera_config.quit_at_camera_path_end);
            //_path_event_handler->frame_tick();
        }
        
        _reproject->setTagInGuiName("Reprojection");
        //_ffmpeg_depth->setTagInGuiName("FFmpeg Depth");
        _guiBuffers.push_back(_reproject);
        _guiBuffers.push_back(_generator);
        _guiBuffers.push_back(_repeater);
        //_guiBuffers.push_back(_ffmpeg_depth);
        
        //auto buf = _edge->getGuiBuffers();
        //_guiBuffers.insert(_guiBuffers.end(), buf.begin(), buf.end());
        
        auto buf = _par->getGuiBuffers();
        _guiBuffers.insert(_guiBuffers.end(), buf.begin(), buf.end());
    }


	void motionExplorer::process() {
        
        
		steppers[0]->process();
        
        //if (_first_run) {
        //    usleep(10000000);
        //}
		bool camera_trigger = false;
		if (_first_run) {
            _gl_d->removeRepeat();
            if (_small == nullptr) {
            _gl->removeRepeat();
            }
            _timer->process(_gl);
			_gl->process_depth();
            _timer->process(_gl_d);
            _gl_d->addRepeat();
            if (_small == nullptr) {
            _gl->addRepeat();
            }
            
			_repeater->process();
			_first_run = false;
			camera_trigger = true;
		}

        auto buf = _original_r->outLock();
        if (buf.has_camera_position()) {
            auto loc = buf.get_camera_position();
            auto tpf = buf.get_theta_phi_fov();
			if (camera_trigger) {
				camera_trigger = false;
				_gl->get_camera()->move_camera({ loc.s[0], loc.s[1], loc.s[2] }, tpf.s[0], tpf.s[1]); 
				_gl->get_camera()->set_fov(tpf.s[2]);
			}

            _generator->set_camera_orig({loc.s[0], loc.s[1], loc.s[2]}, tpf.s[0], tpf.s[1], tpf.s[2]);
            
            _gl->draw_depth_image({loc.s[0], loc.s[1], loc.s[2]}, tpf.s[0], tpf.s[1], tpf.s[2], _par_config.camera_config.equirectangular || _mot_config.spherical);
            _gl_d->process();
            _repeater->set_take_next();
            
            _repeater->process();
            
            if (_mot_config.follow_stereo) {
                _gl->get_camera()->move_camera({ loc.s[0], loc.s[1], loc.s[2] }, tpf.s[0], tpf.s[1]);
                _gl->get_camera()->set_fov(tpf.s[2]);
                
                _gl->get_engine()->add_stereo_shift();
                
            }
        } else if (_path_event_handler != nullptr) {
            auto loc = _path_camera->get_location();
            auto theta = _path_camera->get_theta();
            auto phi = _path_camera->get_phi();
            auto fov = _path_camera->get_fov();
            
            if (camera_trigger) {
                camera_trigger = false;
                _gl->get_camera()->move_camera({ loc.x, loc.y, loc.z }, theta, phi);
                _gl->get_camera()->set_fov(fov);
            }
            
            _generator->set_camera_orig({ loc.x, loc.y, loc.z }, theta, phi, fov);
            
            _gl->draw_depth_image({ loc.x, loc.y, loc.z }, theta, phi, fov, _par_config.camera_config.equirectangular || _mot_config.spherical);
            _gl_d->process();
            _repeater->set_take_next();
            
            _repeater->process();
            
            if (_mot_config.follow_stereo) {
                _gl->get_camera()->move_camera({ loc.x, loc.y, loc.z }, theta, phi);
                _gl->get_camera()->set_fov(fov);
                
                _gl->get_engine()->add_stereo_shift();
                
            }
        }
        _original_r->outUnlock();
        
        _ev->move_camera();
        _timer->process(_gl);
		_gl->process_depth();
        _timer->process(_gl_d);
        
        _generator->set_camera(_par->get_camera());

        _timer->process(_generator);
        _timer->process(_reproject);
		
        
        //_timer->process(_edge);
        
        //_timer->metric(_ffmpeg_depth->get_encoder());

        if (_path_event_handler) {
            _path_event_handler->frame_tick();
        }
        
		_ev->frame_tick();
        _timer->print_metered_report();
	}


	const std::vector<std::shared_ptr<mush::ringBuffer>> motionExplorer::getBuffers() const {
        
        //auto ebuf = _edge->getBuffers();
		//return{ ebuf };
        
        return {_reproject};
	}

	std::vector<std::shared_ptr<mush::guiAccessible>> motionExplorer::getGuiBuffers() {
		const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
		_guiBuffers.clear();
		return buffs;
	}

	std::vector<std::shared_ptr<mush::frameStepper>> motionExplorer::getFrameSteppers() const {
		return steppers;
	}

	std::vector<std::shared_ptr<azure::Eventable>> motionExplorer::getEventables() const {
		//return{ _event_handler };
        return { _par->getEventables() };
	}

	void motionExplorer::go() {
		SetThreadName("Motion Explorer");
        
        //std::thread ffmpeg(&mush::ffmpegEncodeDecode::go, _ffmpeg_depth);
        
        //std::thread par(&parProcessor::go, _par);
        
        _timer->print_header();
/*        int tick = 0;

        while(_original->good()) {
            
            tick++;
            
            _original->outLock();
            _original->outUnlock();
            //_original->outLock();
            //_original->outUnlock();
            
            
            std::stringstream strm;
            strm << tick;
            putLog(strm.str());
            
        }*/
        
		while (_original->good()) {
			process();
		}
        
        _timer->print_final_report();

		_gl->release();

		if (_par != nullptr) {
			_par->release();
		}

		if (_reproject != nullptr) {
			_reproject->release();
		}

		if (steppers[0] != nullptr) {
			steppers[0]->release();
        }
        
        if (_ffmpeg_depth != nullptr) {
            _ffmpeg_depth->release();
        }
        
        
        if (_original != nullptr) {
            _original->release();
        }
        
        if (_motion != nullptr) {
            _motion->release();
        }
        
        if (_small != nullptr) {
            _small->release();
        }
        
        if (_depth != nullptr) {
            _depth->release();
        }
        /*
        while (_ffmpeg_depth->outLock() != nullptr) {
            _ffmpeg_depth->outUnlock();
        }
        
        try {
            if (ffmpeg.joinable()) {
                ffmpeg.join();
            }
        } catch (std::exception& e) {
            putLog(e.what());
        }
         */
        /*
        try {
            if (par.joinable()) {
                par.join();
            }
        } catch (std::exception& e) {
            putLog(e.what());
        }*/
	}

	void motionExplorer::destroy() {
		running = false;
	}

}
