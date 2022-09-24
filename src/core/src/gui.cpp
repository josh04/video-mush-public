#include <fstream>
#include <cmath>
#include <vector>

//#include <Windows.h>
//#include <Shlwapi.h>

#include "mush-core-dll.hpp"

extern "C" MUSHEXPORTS_API void putLog(std::string);

/*
#include <SFML/Graphics.hpp> // SFML must be included first
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
*/

#include "sdlbits.h"
 
#ifdef _WIN32
#include <GL/glew.h>
#endif
#ifdef __APPLE__
#include <OpenGL/GL3.h>
#include <OpenGL/OpenGL.h>
#endif

//#include <ImfRgbaFile.h>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <tiffio.h>

#include <azure/program.hpp>
#include <azure/texture.hpp>
#include <azure/engine.hpp>
#include <azure/events.hpp>
#include <azure/eventtypes.hpp>
#include <scarlet/sim2Display.hpp>
#include <scarlet/filter.hpp>
#include <scarlet/display.hpp>
#include "mushEngine.hpp"
//#include <scarlet/LDRDecoder.hpp>
#include <scarlet/Configuration.hpp>

#include "guiScreenSection.hpp"
#include "mushWindowInterface.hpp"
#include "mushEngine.hpp"
#include "mushSidebar.hpp"
#include "mushMainScreen.hpp"
#include "mushWindow.hpp"
#include "dummyEventable.hpp"
#include "fakeWindow.hpp"

#include "gui.hpp"

//using namespace std;

hdrGui::hdrGui(unsigned int width, unsigned int height, const char * resourceDir, bool fullscreen, int sim2preview = 0)
: width(width), height(height), sim2preview(sim2preview), _fullscreen(fullscreen), _stereo(fullscreen), resourceDir(resourceDir), azure::Eventable(), in_height(height) {
	hasFocus = true;
    sim2HasFocus = false;
    _new_title_set = false;
    
    
}

hdrGui::~hdrGui() {
    
    if (engine != nullptr) {
        //engine->unload();
		engine = nullptr;
    }
    
    if (sim2preview) {
        if (window2 != NULL) {
            scarlet_interface->destroyWindow(window2);
        }
    }

	scarlet_window = nullptr;
	scarlet_sim2_window = nullptr;
	scarlet_display = nullptr;
	scarlet_sim2_display = nullptr;
    
    scarlet::PlatformInterface::SetPlatformInterface(nullptr);
    //scarlet_interface = nullptr;

	sim2Screen = nullptr;
    _sidebar = nullptr;
	_main_screen = nullptr;
	//subScreens.clear();

	azure::ShaderFactory::Reset();

	if (window != NULL) {
		//window->close();
		//delete window;
		scarlet_interface->destroyWindow(window);
		window = NULL;
		scarlet_interface->destroyContext(context);
		context = NULL;
	}

	scarlet::PlatformInterface::SetPlatformInterface(nullptr);
	scarlet_interface = nullptr;
    
}

void hdrGui::createGLContext() {
	_fake_window = std::make_shared<mush::gui::fake_window>();

    scarlet_interface = std::make_shared<mush::gui::window_interface>(resourceDir);
    scarlet::PlatformInterface::SetPlatformInterface(scarlet_interface);


	uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    window = scarlet_interface->createWindow(_title.c_str(), width, height, _fullscreen, flags);

	_stereo = scarlet_interface->is_stereo();

	if (!_stereo) {
		glDrawBuffer(GL_BACK);
	}

    if (sim2preview) {
        window2 = scarlet_interface->createWindow(_title.c_str(), 1920, 1080, false, flags);
        SDL_SetWindowPosition((SDL_Window*)window2, 1680, 0);
        
    }
	//HGLRC curr = wglGetCurrentContext();
	//_glException();
    context = scarlet_interface->createContext(window);

	//wglShareLists((HGLRC)context, (HGLRC)_fake_window->get_context());
	//_glException();

    scarlet_interface->makeCurrent(window, context);
    
    scarlet_interface->processEvents(false);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	if (_stereo) {
		glDrawBuffer(GL_BACK_LEFT);
	}
    glClear(GL_COLOR_BUFFER_BIT);

	if (_stereo) {
		glDrawBuffer(GL_BACK_RIGHT);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawBuffer(GL_BACK_LEFT);
	}

    scarlet_interface->swapWindow(window);

}

void hdrGui::smallDisplay() {
    
    {
        if (_new_title_set) {
            _new_title_set = false;
            SDL_SetWindowTitle((SDL_Window*)window2, _title.c_str());
        }
    }
	//window->setActive(true);
    //scarlet_interface->makeCurrent(window, context);
    
	if (_stereo) {
		glDrawBuffer(GL_BACK_LEFT);
		for (int i = 1; i < 3; ++i) {
			engine->getFrame(_sidebar, _main_screen, i);
			scarlet_window->doRender(_wide, i);
			glDrawBuffer(GL_BACK_RIGHT);
		}
	} else {
		engine->getFrame(_sidebar, _main_screen, 0);
		scarlet_window->doRender(_wide, 0);
	}

    if (sim2preview) {
		if (_stereo) {
			glDrawBuffer(GL_BACK);
		}
        scarlet_sim2_window->doRender(_wide, 0);
    }
}

bool hdrGui::event(std::shared_ptr<azure::Event> event) {
    
    if (hasFocus) {
        
        if (lbracketpressed) {
            if (lshiftpressed && sim2Screen != nullptr) {
					sim2Screen->subtractExposure(0.1f);
            } else {
                if (_sidebar->get_current_main()->getUseExposure()) {
                    _sidebar->get_current_main()->subtractExposure(0.1f);
                }
                else {
                    //subScreens[0]->subtractExposure(0.1f);
                }
            }
        }
        
        if (rbracketpressed) {
            if (lshiftpressed && sim2Screen != nullptr) {
                sim2Screen->addExposure(0.1f);
            } else {
                if (_sidebar->get_current_main()->getUseExposure()) {
                    _sidebar->get_current_main()->addExposure(0.1f);
                } else {
                    //subScreens[0]->addExposure(0.1f);
                }
            }
        }
        
    }

    
    if(_sidebar->get_current_main() != nullptr) {
        if (_sidebar->get_current_main()->doEvent(event)) {
            //return true;
        }
    }
    
	bool trigger_input_lock = false;


    if (event->isType("quit")) {
        engine->removeEventHandler(shared_from_this());
        azure::Events::Push(std::unique_ptr<azure::Event>(new azure::QuitEvent()));
        return true;
    } else if (event->isType("mouseMove")) {
        
        m_x = event->getAttribute<int>("x");
        m_y = highres_height - event->getAttribute<int>("y");
		trigger_input_lock = true;
    } else if (event->isType("mouseDown")) {
        
        if (event->getAttribute<azure::MouseButton>("button") == azure::MouseButton::Left) {
            if (subSectionRows != 0) {
					if (m_x < (highres_width / ((float)subSectionRows + 1))) {
                        if (_wide) {
                            _sidebar->click(m_y, lshiftpressed);
                            update_window_texts();
                            return true;
                        }
					}
            }
        }
		trigger_input_lock = true;
    } else if (event->isType("mouseScroll")) {
        
#ifdef __APPLE__
        float scroll = - event->getAttribute<int>("y") / 75.0f; // magic number
#endif
#ifdef _WIN32
        float scroll = - event->getAttribute<int>("y") / 5.0f; // magic number
#endif
        _sidebar->set_scroll(scroll);
		trigger_input_lock = true;
    } else if (event->isType("keyDown")) {
        
		trigger_input_lock = true;
        
        auto screen_to_change = _sidebar->get_current_main(); //subScreens[swapped];
        auto w = scarlet_window;
        if (lshiftpressed) {
			if (sim2Screen != nullptr) {
				screen_to_change = sim2Screen;
				w = scarlet_sim2_window;
			}
        }
        
        switch (event->getAttribute<azure::Key>("key")) {
            case azure::Key::Escape:
            {
                if (catch_escape) {
                    azure::Events::Push(std::unique_ptr<azure::Event>(new azure::QuitEvent()));
                    return true;
                }
            }
                break;
            case azure::Key::l:
                _input_event_lock = !_input_event_lock;
                scarlet_window->setLock(_input_event_lock);
                break;
            case azure::Key::Backspace:
                screen_to_change->setExposure(0.0);
                return false;
                break;
            case azure::Key::Tab:
                _sidebar->toggle();

				update_window_texts();
                return false;
                break;
            case azure::Key::Hash:
            case azure::Key::Quote:
				reshape(!_wide);
				break;
            case azure::Key::LShift:
                lshiftpressed = true;
                return false;
                break;
            case azure::Key::Leftbracket:
                lbracketpressed = true;
                return false;
                break;
            case azure::Key::Rightbracket:
                rbracketpressed = true;
                return false;
                break;
                
                
            case azure::Key::Num_0:
				screen_to_change->setSelector(mush::gui::screen_section::select::bbb);
				w->setMode(mush::gui::screen_section::getSelectName(screen_to_change->getSelector()));
				return false;
				break;
            case azure::Key::Num_1:
				if (lshiftpressed && screen_to_change->get_stereo() && !_stereo) {
					screen_to_change->setSelector(mush::gui::screen_section::select::anaglyph);
				} else {
					screen_to_change->setSelector(mush::gui::screen_section::select::passthrough);
				}
				w->setMode(mush::gui::screen_section::getSelectName(screen_to_change->getSelector()));
				return false;
				break;
            case azure::Key::Num_2:
				if (lshiftpressed && screen_to_change->get_stereo() && !_stereo) {
					screen_to_change->setSelector(mush::gui::screen_section::select::left_channel);
				} else {
					screen_to_change->setSelector(mush::gui::screen_section::select::lum);
				}
				w->setMode(mush::gui::screen_section::getSelectName(screen_to_change->getSelector()));
				return false;
				break;
            case azure::Key::Num_3:
				if (lshiftpressed && screen_to_change->get_stereo() && !_stereo) {
					screen_to_change->setSelector(mush::gui::screen_section::select::right_channel);
				} else {
					screen_to_change->setSelector(mush::gui::screen_section::select::u);
				}
				w->setMode(mush::gui::screen_section::getSelectName(screen_to_change->getSelector()));
				return false;
				break;
            case azure::Key::Num_4:
				screen_to_change->setSelector(mush::gui::screen_section::select::v);
				w->setMode(mush::gui::screen_section::getSelectName(screen_to_change->getSelector()));
				return false;
				break;
            case azure::Key::Num_5:
				screen_to_change->setSelector(mush::gui::screen_section::select::r);
				w->setMode(mush::gui::screen_section::getSelectName(screen_to_change->getSelector()));
				return false;
				break;
            case azure::Key::Num_6:
				screen_to_change->setSelector(mush::gui::screen_section::select::g);
				w->setMode(mush::gui::screen_section::getSelectName(screen_to_change->getSelector()));
				return false;
				break;
            case azure::Key::Num_7:
				screen_to_change->setSelector(mush::gui::screen_section::select::b);
				w->setMode(mush::gui::screen_section::getSelectName(screen_to_change->getSelector()));
				return false;
				break;
            case azure::Key::Num_8:
				screen_to_change->setSelector(mush::gui::screen_section::select::rrr);
				w->setMode(mush::gui::screen_section::getSelectName(screen_to_change->getSelector()));
				return false;
				break;
            case azure::Key::Num_9:
				screen_to_change->setSelector(mush::gui::screen_section::select::ggg);
				w->setMode(mush::gui::screen_section::getSelectName(screen_to_change->getSelector()));
				return false;
				break;
                
            default:
                break;
                
        }
    } else if (event->isType("keyUp")) {
		trigger_input_lock = true;
        switch (event->getAttribute<azure::Key>("key")) {
            case azure::Key::LShift:
                lshiftpressed = false;
                return false;
                break;
            case azure::Key::Leftbracket:
                lbracketpressed = false;
                return false;
                break;
            case azure::Key::Rightbracket:
                rbracketpressed = false;
                return false;
                break;
        }
    } else if (event->isType("windowResize")) {
        SDL_GL_GetDrawableSize((SDL_Window*)window, &highres_width, &highres_height);
        /*
		unsigned int newwidth = event->getAttribute<int>("x");
		unsigned int newheight = event->getAttribute<int>("y");

		float w_ratio = 16.0;

		if (subSectionRows) {
			w_ratio = 16.0 + 4.0 * (4.0 / (float)subSectionRows);
		}
        */
        _sidebar->resize(highres_width, highres_height);

		return false;
	}
	if (trigger_input_lock) {
		if (_input_event_lock) {
			//if (event->getAttribute<azure::Key>("key") != azure::Key::l) {
				return true;
			//}
		}
	}

    return false;
}
    
char hdrGui::events() {
	scarlet_interface->processEvents(false);

	return true;
}

std::shared_ptr<mush::gui::screen_section> hdrGui::getSubScreen(int i) {
    return _sidebar->get_screen(i);
}

void hdrGui::setSubTextureName(int i, std::string name) {
    _sidebar->get_screen(i)->setName(name);
}

void hdrGui::setSubTextureStereo(int i, bool stereo) {
	_sidebar->get_screen(i)->set_stereo(stereo);
}

void hdrGui::initRocketGuiList() {
    _sidebar->init_rocket_guis();
}

void hdrGui::reshape(bool wide) {
	//_main_screen->set_vertex(wide);


    SDL_GetWindowSize((SDL_Window*)window, (int *)&width, (int *)&height);
    SDL_GL_GetDrawableSize((SDL_Window*)window, &highres_width, &highres_height);

	unsigned int w;
	if (_wide) {
		width = width / (1.0f + 1.0f / subSectionRows);
	} else {
		width = width;
	}
	_wide = wide;

	int diff = 0;

	if (wide) {
		diff = width*(1.0f / (subSectionRows));
		width = width*(1.0f + 1.0f / subSectionRows);
		//highres_width = highres_width*(1.0f + 1.0f/subSectionRows);
	} else {
		diff = width*(1.0f / (subSectionRows));
	}


	// seamless launch path
	if (_sidebar->get_screens_count() == 1) {
		diff = diff / 2;
	}

	int pos_x = 0, pos_y = 0;
	SDL_GetWindowPosition((SDL_Window*)window, &pos_x, &pos_y);

	if (_wide) {
		SDL_SetWindowPosition((SDL_Window*)window, pos_x - diff, pos_y);
	}

    SDL_SetWindowSize((SDL_Window*)window, width, height);
    SDL_GL_GetDrawableSize((SDL_Window*)window, &highres_width, &highres_height);
    
	if (!_wide) {
		SDL_SetWindowPosition((SDL_Window*)window, pos_x + diff, pos_y);
	}
    
    azure::Events::Push(std::unique_ptr<azure::Event>(new azure::WindowResizeEvent((azure::WindowHandle) window, highres_width, highres_height)));
}

std::shared_ptr<mush::gui::screen_section> hdrGui::addScreenSection() {
    
    if (_sidebar->get_screens_count() == 1) {
        reshape(true);
		//_decoder->setSize(highres_width, highres_height);
    }
    
    auto s = _sidebar->add_screen();
    
    if (_sidebar->get_screens_count() == 1 && sim2preview) {
        sim2Screen = std::make_shared<mush::gui::screen_section>((int)mush::gui::screen_section::screenSections::sim2, 1920, 1080, subSectionRows);
        sim2Screen->setTexture(s->getTexture());
        //std::vector<std::shared_ptr<mush::gui::screen_section>> temp;
        scarlet_sim2_window = std::dynamic_pointer_cast<mush::gui::window>(engine->addWindow(window2, context, nullptr, nullptr, sim2Screen));
        scarlet_sim2_display = engine->getDisplay("Sim2");
        scarlet_sim2_window->setDisplay(scarlet_sim2_display);
        azure::Events::Push(std::unique_ptr<azure::Event>(new azure::WindowResizeEvent(window2, 1920, 1080)));
        scarlet_sim2_window->setFPS("");
        scarlet_sim2_window->setLabel("");
        //scarlet_sim2_window->setInnerFPS(0.0f);
    }
    
    return s;

}

void hdrGui::init(unsigned int rows) {
    
    engine = std::make_shared<mush::gui::engine>();
	scarlet::PlatformInterface::SetPlatformInterface(scarlet_interface);
    
	glClearColor(0.0, 0.0, 0.0, 1.0);
    
    SDL_GetWindowSize((SDL_Window*)window, (int *)&width, (int *)&height);
    //sf::Vector2u size = window->getSize();
    
    SDL_GL_GetDrawableSize((SDL_Window*)window, &highres_width, &highres_height);

    subSectionRows = rows;

    
    azure::Events::Push(std::unique_ptr<azure::Event>(new azure::WindowResizeEvent((azure::WindowHandle) window, highres_width, highres_height)));
	//const char * err = _glError();
	auto err = glGetError();
	engine->mush_init();

//	scarlet::Configuration::SetAttribute<string>("tfdecode", "rec709");
	//scarlet::Configuration::SetAttribute<string>("tfdecode", "PTF4");
	//scarlet::Configuration::SetAttribute<string>("tfdecode", "pq");
	//scarlet::Configuration::SetAttribute<string>("tfdecode", "logc");
    
    //scarlet::Configuration::SetAttribute<std::string>("tfencode", "rec709");
    scarlet::Configuration::SetAttribute<std::string>("csencode", "identity");
    scarlet::Configuration::SetAttribute<std::string>("csdecode", "identity");
    scarlet::Configuration::SetAttribute<std::string>("tfencode", "srgb");
    scarlet::Configuration::SetAttribute<std::string>("defaultfilter", "shaders/mush.filter");

    engine->setFPS(999999.0f);
    
    //registering
	for (auto & i : boost::filesystem::directory_iterator(scarlet_interface->getResourcesFolder() / boost::filesystem::path("shaders"))) {
        auto & s = i.path().extension().generic_string();
        if (s == ".filter") {
            engine->registerFilter(std::make_shared<scarlet::Filter>("shaders/" + i.path().filename().generic_string()));
        } else if (s == ".display") {
            engine->registerDisplay(std::make_shared<scarlet::Display>("shaders/" + i.path().filename().generic_string()));
        }
    }
    engine->registerDisplay(std::make_shared<scarlet::Sim2Display>());
    // /registering
    
    
    
    _sidebar = std::make_shared<mush::gui::sidebar>(subSectionRows, highres_width, highres_height);
    _main_screen = std::make_shared<mush::gui::main_screen>(subSectionRows);
    
    
    scarlet_window = std::dynamic_pointer_cast<mush::gui::window>(engine->addWindow(window, context, _sidebar, _main_screen/*, subScreens*/));
    scarlet_display = engine->getDisplay("LDR");
    scarlet_window->setDisplay(scarlet_display);
   
    
    scarlet::Configuration::SetAttribute<std::string>("defaultfilter", "shaders/mushSub.filter");
    /*
    if (sim2preview) {
        sim2Screen = std::make_shared<mush::gui::screen_section>((int)mush::gui::screen_section::screenSections::sim2, 1920, 1080, subSectionRows);
        //sim2Screen->setTexture(subScreens[0]->getTexture());
        //std::vector<std::shared_ptr<mush::gui::screen_section>> temp;
        scarlet_sim2_window = std::dynamic_pointer_cast<mush::scarletWindow>(engine->addWindow(window2, context, sim2Screen));
        scarlet_sim2_display = engine->getDisplay("Sim2");
        scarlet_sim2_window->setDisplay(scarlet_sim2_display);
        azure::Events::Push(std::unique_ptr<azure::Event>(new azure::WindowResizeEvent(window2, 1920, 1080)));
    }
    */
    
    //engine->unload();
    engine->addEventHandler(std::dynamic_pointer_cast<azure::Eventable>(shared_from_this()));
    
    engine->addEventHandler(std::dynamic_pointer_cast<azure::Eventable>(_main_screen->get_eventable()));
}

float hdrGui::getExposure(int i) {
    return _sidebar->get_screen(i)->getExposure();
}

void hdrGui::mipmaps() {
    if (sim2Screen != nullptr) {
        sim2Screen->mipmap();
    }
    
    for (int i = 0; i < _sidebar->get_screens_count(); ++i) {
        _sidebar->get_screen(i)->mipmap();
    }
}

void hdrGui::setRecording(int i, bool recording) { 
	if (i > -1) {
		_sidebar->get_screen(i)->set_recording(recording);
	}
}

void hdrGui::setUseExposure(int i, bool recording) {
	if (i > -1) {
		_sidebar->get_screen(i)->setUseExposure(recording);
	}
}

void hdrGui::addEventHandler(std::shared_ptr<azure::Eventable> hand) {
    if (engine != nullptr) {
        engine->addEventHandler(hand);
    }
}

void hdrGui::removeEventHandler(std::shared_ptr<azure::Eventable> hand) {
    if (engine != nullptr) {
        engine->removeEventHandler(hand);
    }
}

void hdrGui::addRocketGui(int i, std::string pathToRML) {
    _sidebar->add_rocket_gui(i, pathToRML);
}

void hdrGui::update_window_texts() {
    scarlet_window->setMode(mush::gui::screen_section::getSelectName(_sidebar->get_current_main()->getSelector()));
    
    scarlet_window->setLabel(_sidebar->get_current_main()->getName().c_str());
    scarlet_window->setInnerFPS(_sidebar->get_current_main()->get_fps_pointer());
    
    if (scarlet_sim2_window != nullptr) {
        scarlet_sim2_window->setMode(mush::gui::screen_section::getSelectName(_sidebar->get_current_sim2()->getSelector()));
    }
}

int hdrGui::getCurrentMainScreen() const { 
	return _sidebar->get_current_main_int_legacy();
}

void hdrGui::clear_sidebar() {
	_sidebar->clear();
}