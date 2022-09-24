//
//  tagInGui.cpp
//  video-mush
//
//  Created by Josh McNamee on 26/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#define __CL_ENABLE_EXCEPTIONS

#include <string>
#include <vector>

#include <boost/date_time.hpp>

#include <azure/events.hpp>
#include <azure/eventtypes.hpp>
#include <azure/framebuffer.hpp>

#include "gui.hpp"

#include "tagInGui.hpp"
#include "guiAccessible.hpp"
#include "opencl.hpp"
//#include "hdrControl.hpp"

#include "guiScreenSection.hpp"
#include "mushEngine.hpp"

//#include <SFML/Window/Keyboard.hpp>

#include "hdrEXR.hpp"
#include "clToTIFF.hpp"

#include "integerMapBuffer.hpp"

void tagInGui::addFakeSubScreen(unsigned int width, unsigned int height) {

	auto subScreen = gui->addScreenSection();

	gui->setSubTextureName(0, "Loading");
	gui->setSubTextureStereo(0, false);

	//widths.push_back(width);
	//heights.push_back(height);

	subScreen->createTexture(width, height, true);

	gui->update_window_texts();
}

void tagInGui::addSubScreen(std::shared_ptr<mush::guiAccessible> buffer) {
    if (buffer == nullptr) {
        return;
    }

	if (_screen_count == 0) {
		gui->clear_sidebar();
	}
    
    auto subScreen = gui->addScreenSection();
    if (buffer->get_eventable() != nullptr) {
        subScreen->set_eventables({buffer->get_eventable()});
    }
    unsigned int width = 0, height = 0, size = 0;
    size_t i = _screen_count++;//gui->getSubScreenCount() - 1;
    
    buffer->getParams(width, height, size);
    names.push_back(buffer->getTagInGuiName());
    gui->setSubTextureName(i, buffer->getTagInGuiName());
	gui->setSubTextureStereo(i, buffer->getTagInGuiStereo());

	if (buffer->getTagInGuiStereo()) {
		if (gui->has_stereo()) {
			subScreen->setSelector(mush::gui::screen_section::select::stereo);
		} else {
			subScreen->setSelector(mush::gui::screen_section::select::anaglyph);
		}
	}
    
    widths.push_back(width);
    heights.push_back(height);
    
    subScreen->createTexture(width, height);
    
    //gui->mipmaps();
    GLuint tex = subScreen->getTexture()->getTexture();
    
    cl::ImageGL * surface = this->context->glImage(tex, CL_MEM_READ_WRITE, GL_TEXTURE_2D, false);
    /*
    std::vector<cl::Memory> glObjects;
    glObjects.push_back(*surface);
    cl::Event event;
    queue->enqueueAcquireGLObjects(&glObjects, NULL, &event);
    event.wait();
    */
    surface_images.push_back(*surface);
    surfaces.push_back(mush::buffer(*surface));
    last_frame_times.push_back(azure::Clock::now());
    fps_counters.push_back(0);
    fps_s.push_back(0.0f);
    
    subScreen->set_fps_pointer(&fps_s[fps_s.size()-1]);
    
    buffer->guiTag(i, this);
    
    if (auto buffer_int = std::dynamic_pointer_cast<mush::integerMapBuffer>(buffer)) {
        tiff_outputs.push_back(true);
    } else {
        tiff_outputs.push_back(false);
    }
    
    if (i == 0) {
        initEXROut();
        gui->update_window_texts();
        gui->initRocketGuiList();
    }
    _count++;
    
    delete surface;
}

void tagInGui::init(shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::guiAccessible>>& buffers, const char * exrDir, unsigned int subScreenRows) {
    if (context == nullptr) {
		throw std::runtime_error("OpenCL context creation failed.");
    }

	this->exrDir = exrDir;
    
    /*
    if (_count == 0) {
        return;
    }
    */
    queue = context->getQueue();
    cl::Event event;
    
//    GLenum err;
    
    gui->addEventHandler(shared_from_this());
    registeredEventHandler = true;
    

    std::vector<mush::guiAccessible *> buf_done;
    
	if (buffers.size() > 0) {
		for (int i = 0; i < buffers.size(); ++i) {
            bool cont = false;
            for (auto b : buf_done) {
                if (b == buffers[i].get()){
                    cont = true;
                    break;
                }
            }
            if (cont) {
                continue;
            }
            addSubScreen(buffers[i]);
            buf_done.push_back(buffers[i].get());
        }
        initEXROut();
        gui->update_window_texts();
        gui->initRocketGuiList();
	}
    
    _working = true;
}

void tagInGui::createGLContext(bool sim2preview, bool fullscreen, const char *resourceDir) {

    unsigned int width = 1280;
    unsigned int height = 720;

    putLog("--- Init GL");
    gui = std::make_shared<hdrGui>((unsigned int)(width), (height), resourceDir, fullscreen, sim2preview);
    gui->createGLContext();


	gui->init(8);
	addFakeSubScreen(width, height);
}

std::shared_ptr<mush::opencl> tagInGui::createInteropContext(const char *resourceDir, bool openclCPU) {
    
    context = std::make_shared<mush::opencl>(resourceDir, openclCPU);
    context->init(true);
    
	postContextSetup();
    return context;
}

void tagInGui::postContextSetup() {
	putLog("---Tag Getting Queue");
	_copy = context->getKernel("copyImage");
	this->queue = context->getQueue();
}

void tagInGui::initEXROut() {
    exrOut = std::make_shared<hdrEXR::clToEXR>();
    exrOut->init(context);
    tiffOut = std::make_shared<mush::clToTIFF>();
    tiffOut->init(context);
}

void tagInGui::update() {
	/*
    if (_count == 0) {
        return;
    }
	*/
    
    if (_working) {
		/*
        std::vector<cl::Memory> glMem;
        for (int i = 0; i < surface_images.size(); ++i) {
			glMem.push_back(surface_images[i]);
        }
		*/
		gui->events();

		//if (glMem.size() > 0) {
			gui->engine->getEvents();
			gui->engine->doUpdate();
          /*  
            cl::Event eventa;
            std::lock_guard<std::mutex> lock(tagMutex);
            
            queue->enqueueReleaseGLObjects(&glMem, NULL, &eventa);
			eventa.wait();
			*/
			//glFinish();
            //gui->mipmaps();
            
            gui->smallDisplay();
            /*
			//glFinish();
			cl::Event eventb;
            queue->enqueueAcquireGLObjects(&glMem, NULL, &eventb);
            eventb.wait();
			*/
        //}
    }
}

void tagInGui::copyImageIntoGuiBuffer(int index, const mush::buffer& image) {
    if (_working) {
        if (index < surfaces.size()) {
            std::lock_guard<std::mutex> tagLock(tagMutex);

			if (image.has_camera_position()) {
				surfaces[index].set_camera_position(image.get_camera_position(), image.get_theta_phi_fov());
            } else {
                surfaces[index].set_no_camera_position();
            }
            
            if (image.get_type() == mush::buffer::type::cl_image) {
                cl::Event event;
                _copy->setArg(0, image.get_image());
                
                acquireCL(surface_images[index]);

                _copy->setArg(1, surfaces[index].get_image());
                queue->enqueueNDRangeKernel(*_copy, cl::NullRange, cl::NDRange(widths[index], heights[index]), cl::NullRange, NULL, &event);
                event.wait();
                
                releaseCL(surface_images[index]);
            } else if (image.get_type() == mush::buffer::type::gl_framebuffer) {
                auto fb = image.get_gl_framebuffer();
                fb->bind(false);
                
                auto tex = gui->getSubScreen(index)->getTexture();
                tex->bind();
                glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                                    0, 0,
                                    0, 0, widths[index], heights[index]);
                _glException();
                
                tex->unbind();
                //fb->unbind(); // causes flicker
            }

			fps_tick(index);
            
            //surfaces[index]->setNewFrame();
        }
        
    }
}

void tagInGui::fps_tick(int index) {

	fps_counters[index]++;

	if (fps_counters[index] % 5 == 0) {
		azure::TimePoint newTime = azure::Clock::now();
		azure::Duration delta = newTime - last_frame_times[index];
		last_frame_times[index] = newTime;
		fps_s[index] = 5.0f / (float)(delta.count());
	}

}

void tagInGui::writeEXR() {
    std::lock_guard<std::mutex> lock(tagMutex);
    try {
        char buffer[80];
        
        std::tm t = boost::posix_time::to_tm(boost::posix_time::second_clock::local_time());
        
        strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S", &t);
        
        std::stringstream strmLog, strm;
        int swapped = gui->getCurrentMainScreen();
        
        strm << exrDir << buffer << " - " << swapped << " " << names[swapped];

		acquireCL(surface_images[swapped]);
        if (tiff_outputs[swapped]) {
            strm << ".tif";
            strmLog << "Saving TIFF frame as " << strm.str();
            putLog(strmLog.str().c_str());
            tiffOut->write(surfaces[swapped].get_image(), strm.str(), widths[swapped], heights[swapped]);
        } else {
            strm << ".exr";
            strmLog << "Saving EXR frame as " << strm.str();
            putLog(strmLog.str().c_str());
            exrOut->write(surfaces[swapped], strm.str(), widths[swapped], heights[swapped]);
        }
		releaseCL(surface_images[swapped]);
    } catch (std::exception& e) {
        putLog("Error writing EXR.");
        putLog(e.what());
    }
}

void tagInGui::writeEXRs() {
    try {
        std::lock_guard<std::mutex> lock(tagMutex);
        char buffer[80];
        
        std::tm t = boost::posix_time::to_tm(boost::posix_time::second_clock::local_time());
        
        strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S", &t);

		cl::Event eventa;
		queue->enqueueAcquireGLObjects(&surface_images, NULL, &eventa);
		eventa.wait();

        for (int i = 0; i < surfaces.size(); ++i) {
            std::stringstream strmNm, strmLog;
            strmNm << exrDir << buffer << " - " << i << " " << names[i];
            if (tiff_outputs[i]) {
                strmNm << ".tif";
                strmLog << "Saving TIFF frame as " << strmNm.str();
                putLog(strmLog.str().c_str());
                tiffOut->write(surfaces[i].get_image(), strmNm.str(), widths[i], heights[i]);
            } else {
                strmNm << ".exr";
                strmLog << "Saving EXR frame as " << strmNm.str();
                putLog(strmLog.str().c_str());
                exrOut->write(surfaces[i], strmNm.str(), widths[i], heights[i]);
            }
        }

		cl::Event eventb;
		queue->enqueueReleaseGLObjects(&surface_images, NULL, &eventb);
		eventa.wait();

    } catch (std::exception& e) {
        putLog("Error writing EXR.");
        putLog(e.what());
    }
}

void tagInGui::fireQuitEvent() {
    if (registeredEventHandler) {
        registeredEventHandler = false;
        gui->removeEventHandler(shared_from_this());
    }
    azure::Events::Push(std::unique_ptr<azure::Event>(new azure::QuitEvent()));
    update();
}

bool tagInGui::event(std::shared_ptr<azure::Event> event) {
    if (event->isType("quit")) {
        registeredEventHandler = false;
        gui->removeEventHandler(shared_from_this());
        azure::Events::Push(std::unique_ptr<azure::Event>(new azure::QuitEvent()));
        return true;
    } else if (event->isType("keyDown")) {
        switch (event->getAttribute<azure::Key>("key")) {
            case azure::Key::z:
                if (gui->lshiftpressed) {
                    writeEXRs();
                } else {
                    writeEXR();
                }
                return true;
                break;
            default:
                break;
                
        }
    } else if (event->isType("saveSnapshots")) {
        writeEXRs();
    }
    return false;

}

void tagInGui::addEventHandler(std::shared_ptr<azure::Eventable> hand) {
    return gui->addEventHandler(hand);
}
void tagInGui::removeEventHandler(std::shared_ptr<azure::Eventable> hand) {
    return gui->removeEventHandler(hand);
}

void tagInGui::drop_gl_cl_interop() {
	
	if (surface_images.size() > 0) {
		cl::Event eventa;
		std::lock_guard<std::mutex> lock(tagMutex);

		//queue->enqueueReleaseGLObjects(&surface_images, NULL, &eventa);
		//eventa.wait();
	}

    surface_images.clear();
    
	_working = false;
}

void tagInGui::set_gui_catch_escape(bool c) {
    gui->catch_escape = c;
}

void tagInGui::acquireCL(const cl::Memory& buf) {
	std::vector<cl::Memory> glMem = { buf };
	cl::Event eventa;
	queue->enqueueAcquireGLObjects(&glMem, NULL, &eventa);
	eventa.wait();
}

void tagInGui::releaseCL(const cl::Memory& buf) {
	std::vector<cl::Memory> glMem = { buf };
	cl::Event eventa;
	queue->enqueueReleaseGLObjects(&glMem, NULL, &eventa);
	eventa.wait();
}
