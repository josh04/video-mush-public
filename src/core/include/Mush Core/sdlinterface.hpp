#ifndef AZURE_SDLINTERFACE_HPP
#define AZURE_SDLINTERFACE_HPP

#include <iostream>
#include <map>
#include <stdexcept>

#include <azure/eventkey.hpp>
#include <azure/windowinterface.hpp>

#include <scarlet/platforminterface.hpp>

#include "mush-core-dll.hpp"

#include "sdlbits.h"

class MUSHEXPORTS_API SDLInterface : public scarlet::PlatformInterface {
	static int _instances;
	static SDL_GLContext _context;
	std::map<uint32_t, bool> _fullscreen;
	bool _stereo = false;


	public:
	SDLInterface();
	~SDLInterface() {
		//SDL_Quit();
	}

	azure::WindowHandle createWindow(const char * title, unsigned int width, unsigned int height, bool fullscreen, Uint32 flags);

	azure::ContextHandle createContext(azure::WindowHandle window);

	void destroyContext(azure::ContextHandle context);

	void destroyWindow(azure::WindowHandle window);

	void makeCurrent(azure::WindowHandle window, azure::ContextHandle context) override;

	void swapWindow(azure::WindowHandle window) override;

	void processEvents(bool wait = false) override;

	azure::Event * sdlEvent(SDL_Event & event);

	uint32_t getID(azure::WindowHandle window) const;

	bool getFullscreen(azure::WindowHandle window) const;
	void setFullscreen(azure::WindowHandle window, bool fullscreen);

	void setTitle(azure::WindowHandle window, std::string title);

	static azure::Key SDLKeyToKey(SDL_Keycode code);

	static azure::MouseButton SDLButtonToButton(Uint8 button);

	bool is_stereo() const {
		return _stereo;
	}

	std::string getPythonPath() const;
	/*boost::filesystem::path getResourcesFolder() {
		char cwd[PATH_MAX];
		getcwd(cwd, sizeof(cwd));
		return boost::filesystem::path(cwd);
	}*/
};

#endif

