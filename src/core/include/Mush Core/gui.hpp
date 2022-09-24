#ifndef HDRGUI_HPP
#define HDRGUI_HPP

#include <atomic>

#ifdef _WIN32
#include <GL/glew.h>
#endif
#ifdef __APPLE__
#include <OpenGL/GL3.h>
#endif

namespace azure {
    class Shader;
    class Program;
}

#include <vector>

#include <azure/Eventable.hpp>
#include <azure/Texture.hpp>
#include <azure/windowinterface.hpp>

namespace scarlet {
    class Display;
}

namespace mush {
    namespace gui {
        class dummy_eventable;
		class engine;
		class window;
		class screen_section;
		class window_interface;
		class sidebar;
		class main_screen;
		class fake_window;
    }
}

#include "mush-core-dll.hpp"

class MUSHEXPORTS_API hdrGui : public azure::Eventable, public std::enable_shared_from_this<hdrGui> {
	public:
		hdrGui(unsigned int width, unsigned int height, const char * resourceDir, bool fullscreen, int sim2preview);
    ~hdrGui();
    void createGLContext();
    void init(unsigned int rows);
    void smallDisplay();
    char events();
    
    std::shared_ptr<mush::gui::screen_section> getSubScreen(int i);
    void setSubTextureName(int i, std::string name);
	void setSubTextureStereo(int i, bool stereo);

	
    void initRocketGuiList();
    void mipmaps();
	float getExposure(int i);
	void setRecording(int, bool);
	void setUseExposure(int i, bool recording);
	int getCurrentMainScreen() const;
    void addEventHandler(std::shared_ptr<azure::Eventable> hand);
    void removeEventHandler(std::shared_ptr<azure::Eventable> hand);
	bool event(std::shared_ptr<azure::Event> event);
    void addRocketGui(int i, std::string pathToRML);
    
    std::shared_ptr<mush::gui::screen_section> addScreenSection();

	std::shared_ptr<mush::gui::engine> engine = nullptr;
    
    bool lshiftpressed = false, lbracketpressed = false, rbracketpressed = false;
    
    void setTitle(std::string new_title) {
        _title = new_title;
        _new_title_set = true;
    }
    
    void update_window_texts();
    
    bool catch_escape = true;
    
	bool has_stereo() const {
		return _stereo;
	}

	void clear_sidebar();

private:
    void reshape(bool wide);

    std::string _title = "Video Mush Monitor";
    std::atomic_bool _new_title_set;

    bool hasFocus, sim2HasFocus;

    std::shared_ptr<mush::gui::screen_section> sim2Screen = nullptr;

    //sf::RenderWindow * window = NULL, * window2 = NULL;
    azure::WindowHandle window = NULL, window2 = NULL;
    azure::ContextHandle context = NULL;
    void * surface;
    unsigned int width, height;
    int highres_width, highres_height;
    int subSectionRows = 0;
    float scroll = 0.0;
    int sim2preview;
    int swapped = 0;
    int tabbed = 0;
    const char * resourceDir;
    
    std::shared_ptr<mush::gui::window_interface> scarlet_interface = nullptr;
    
    std::shared_ptr<mush::gui::window> scarlet_window = nullptr, scarlet_sim2_window = nullptr;
    std::shared_ptr<scarlet::Display> scarlet_display = nullptr, scarlet_sim2_display = nullptr;
    
    int m_x = 0, m_y = 0;

	int clickoffset = 0;
	unsigned int in_height;
    
    std::shared_ptr<mush::gui::sidebar> _sidebar = nullptr;
    std::shared_ptr<mush::gui::main_screen> _main_screen = nullptr;
    
    bool _input_event_lock = false;
	bool _wide = false;

	bool _fullscreen = false;
	bool _stereo = false;

	std::shared_ptr<mush::gui::fake_window> _fake_window = nullptr;
};

#endif

