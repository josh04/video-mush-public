#include <stdexcept>

#include <azure/glbits.h>
#include <azure/glexception.hpp>
#include <azure/events.hpp>
#include <azure/eventtypes.hpp>

#include "mushLog.hpp"

#include "sdlinterface.hpp"

using namespace azure;
using namespace std;

int SDLInterface::_instances = 0;
SDL_GLContext SDLInterface::_context;

SDLInterface::SDLInterface() {
    
    static std::once_flag initFlag;
    std::call_once(initFlag, [&]() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw runtime_error(SDL_GetError());
    }
    });
	
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}
#ifdef _WIN32
int test_stereo(unsigned int width, unsigned int height, uint32_t flags) {

	SDL_Window * test_window;
	if ((test_window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags)) == NULL) {
		throw runtime_error(SDL_GetError());
	}
	SDL_GLContext test_context = SDL_GL_CreateContext(test_window);
	if (test_context == NULL) {
		throw runtime_error(SDL_GetError());
	}

	auto hdc = wglGetCurrentDC();

	PIXELFORMATDESCRIPTOR pfd;
	auto iPixelFormat = DescribePixelFormat(hdc, 1, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	int stereo_formats = 0;

	while (iPixelFormat) {
		DescribePixelFormat(hdc, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
		if (pfd.dwFlags & PFD_STEREO) {
			stereo_formats++;
		}
		iPixelFormat--;
	}

	SDL_GL_DeleteContext(test_context);
	SDL_DestroyWindow(test_window);
	return stereo_formats;
}
#endif

WindowHandle SDLInterface::createWindow(const char * title, unsigned int width, unsigned int height, bool fullscreen, Uint32 flags) {
	

	if (fullscreen) {
		static int stereo_formats = 0;
		static bool stereo_once = false;

		putLog("--- Enabling GL Fullscreen.");
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#ifdef _WIN32
		if (!stereo_once) {
			stereo_formats = test_stereo(width, height, flags);
			stereo_once = true;
		}
#endif

        if (stereo_formats > 0) {
			_stereo = true;
            putLog("--- Enabling GL Stereo.");
            SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
        } else {
            putLog("--- GL Stereo not supported.");
            SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
        }
	} else {
		SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
	}

	SDL_Window * window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
	if (window == NULL) {
		throw runtime_error(SDL_GetError());
	}
	
	if (flags & SDL_WINDOW_ALLOW_HIGHDPI) {
		SDL_GL_GetDrawableSize(window, (int *)&width, (int *)&height); // Override the width and height with correct ones from the hidpi window.
	}
	Events::Push(unique_ptr<azure::Event>(new WindowResizeEvent((WindowHandle)window, width, height)));
	
	// Add entry to fullscreen map.
	_fullscreen[SDL_GetWindowID(window)] = false;
	
	return (WindowHandle)window;
}

ContextHandle SDLInterface::createContext(WindowHandle window) {
	SDL_Window * w = (SDL_Window *)window;
	_instances++;
	
	if (_instances <= 1) {
		if ((_context = SDL_GL_CreateContext(w)) == NULL) {
			throw runtime_error(SDL_GetError());
		}
		
#ifndef __APPLE__ // #ifdef HAVE_GLEW?
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK) {
			throw runtime_error("GLEW init failed.");
		}
		glGetError(); // GLEW causes an error. Throw it away.
#endif
		/*
		if (SDL_GL_SetSwapInterval(-1) == -1) {
			if (SDL_GL_SetSwapInterval(1) == -1) {
				//throw runtime_error();
				cerr << "Neither late swap tearing or VSync supported. No biggie." << endl;
			}
		}
		*/
		
	}
	
	return (ContextHandle)_context;
}

void SDLInterface::destroyContext(ContextHandle context) {
	_instances--;
	
	if (_instances < 1) {
		SDL_GL_DeleteContext(_context);
        _context = nullptr;
        _instances = 0;
	}
}

void SDLInterface::destroyWindow(WindowHandle window) {
	SDL_Window * w = (SDL_Window *)window;
	SDL_DestroyWindow(w);
}

void SDLInterface::makeCurrent(WindowHandle window, ContextHandle context) {
	SDL_Window * w = (SDL_Window *)window;
	SDL_GLContext c = (SDL_GLContext)context;
	
	//glFinish();
	//_glException();
	
	SDL_GL_MakeCurrent(w, c);
}

void SDLInterface::swapWindow(WindowHandle window) {
	SDL_Window * w = (SDL_Window *)window;
	SDL_GL_SwapWindow(w);
}

void SDLInterface::processEvents(bool wait) {
	SDL_Event event;
	if (wait) {
		if (SDL_WaitEvent(&event)) {
			Events::Push(unique_ptr<Event>(sdlEvent(event)));
		} else {
			throw runtime_error(SDL_GetError());
		}
	}
	
	while (SDL_PollEvent(&event)) {
        auto azure_event = sdlEvent(event);
        if (azure_event != nullptr) {
            Events::Push(unique_ptr<Event>(azure_event));
        }
	}
}

Event * SDLInterface::sdlEvent(SDL_Event & event) {
	switch (event.type) {
		case SDL_KEYDOWN: {
			return new KeyDownEvent((WindowHandle)SDL_GetWindowFromID(event.key.windowID), SDLKeyToKey(event.key.keysym.sym));
		} break;
		case SDL_KEYUP: {
			return new KeyUpEvent((WindowHandle)SDL_GetWindowFromID(event.key.windowID), SDLKeyToKey(event.key.keysym.sym));
        } break;
        case SDL_TEXTINPUT: {
            {
                auto ev = new Event("textEntry", 0);
                ev->setAttribute<std::string>("text", event.text.text);
                return ev;
            }
        } break;
		case SDL_MOUSEMOTION: {
			SDL_Window * w = SDL_GetWindowFromID(event.window.windowID);
			int x, y, h_x, h_y;
            SDL_GetWindowSize(w, &x, &y);
            SDL_GL_GetDrawableSize(w, &h_x, &h_y);
            float x_diff = h_x/(float)x;
            float y_diff = h_y/(float)y;
			return new MouseMoveEvent((WindowHandle)w, (int)(event.motion.x*x_diff), h_y - (int)(event.motion.y*y_diff));
		} break;
		case SDL_MOUSEBUTTONDOWN: {
			return new MouseDownEvent((WindowHandle)SDL_GetWindowFromID(event.button.windowID), SDLButtonToButton(event.button.button));
		} break;
		case SDL_MOUSEBUTTONUP: {
			return new MouseUpEvent((WindowHandle)SDL_GetWindowFromID(event.button.windowID), SDLButtonToButton(event.button.button));
		} break;
		case SDL_MOUSEWHEEL: {
			return new MouseScrollEvent((WindowHandle)SDL_GetWindowFromID(event.wheel.windowID), event.wheel.x, event.wheel.y);
		} break;
		case SDL_WINDOWEVENT: {
			switch (event.window.event) {
				case SDL_WINDOWEVENT_RESIZED: {
					int width, height;
					SDL_GL_GetDrawableSize(SDL_GetWindowFromID(event.window.windowID), &width, &height);
					return new WindowResizeEvent((WindowHandle)SDL_GetWindowFromID(event.window.windowID), width, height);
				} break;
				case SDL_WINDOWEVENT_MOVED: {
					return new WindowMoveEvent((WindowHandle)SDL_GetWindowFromID(event.window.windowID), event.window.data1, event.window.data2);
				} break;
				case SDL_WINDOWEVENT_ENTER: {
					auto ev = new Event("windowEntered", (WindowHandle)SDL_GetWindowFromID(event.window.windowID));
					return ev; 
				} break;
				case SDL_WINDOWEVENT_LEAVE: {
					auto ev = new Event("windowLeft", (WindowHandle)SDL_GetWindowFromID(event.window.windowID));
					return ev;
				} break;
				case SDL_WINDOWEVENT_CLOSE: {
					auto ev = new Event("quit", (WindowHandle)SDL_GetWindowFromID(event.window.windowID));
					return ev;
				} break;
			}
		} break;
		case SDL_QUIT: {
			return new QuitEvent();
		} break;
	}
	
	//return new Event("", 0); // Just in case
    return nullptr; // JESUS CHRIST JON
}

uint32_t SDLInterface::getID(WindowHandle window) const {
	SDL_Window * w = (SDL_Window *)window;
	return SDL_GetWindowID(w);
}

bool SDLInterface::getFullscreen(WindowHandle window) const {
	SDL_Window * w = (SDL_Window *)window;
	return _fullscreen.at(SDL_GetWindowID(w));
}
void SDLInterface::setFullscreen(WindowHandle window, bool fullscreen) {
	SDL_Window * w = (SDL_Window *)window;
	_fullscreen[SDL_GetWindowID(w)] = fullscreen;
	SDL_SetWindowFullscreen(w, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
}

void SDLInterface::setTitle(WindowHandle window, string title) {
	SDL_Window * w = (SDL_Window *)window;
	SDL_SetWindowTitle(w, title.c_str());
}

Key SDLInterface::SDLKeyToKey(SDL_Keycode code) {
	switch (code) {
		case SDLK_RETURN: return Key::Return;
		case SDLK_ESCAPE: return Key::Escape;
		case SDLK_BACKSPACE: return Key::Backspace;
		case SDLK_TAB: return Key::Tab;
		case SDLK_SPACE: return Key::Space;
		case SDLK_EXCLAIM: return Key::Exclaim;
		case SDLK_QUOTEDBL: return Key::QuoteDouble;
		case SDLK_HASH: return Key::Hash;
		case SDLK_PERCENT: return Key::Percent;
		case SDLK_DOLLAR: return Key::Dollar;
		case SDLK_AMPERSAND: return Key::Ampersand;
		case SDLK_QUOTE: return Key::Quote;
		case SDLK_LEFTPAREN: return Key::LeftParen;
		case SDLK_RIGHTPAREN: return Key::RightParen;
		case SDLK_ASTERISK: return Key::Asterisk;
		case SDLK_PLUS: return Key::Plus;
		case SDLK_COMMA: return Key::Comma;
		case SDLK_MINUS: return Key::Minus;
		case SDLK_PERIOD: return Key::Period;
		case SDLK_SLASH: return Key::Slash;
		case SDLK_0: return Key::Num_0;
		case SDLK_1: return Key::Num_1;
		case SDLK_2: return Key::Num_2;
		case SDLK_3: return Key::Num_3;
		case SDLK_4: return Key::Num_4;
		case SDLK_5: return Key::Num_5;
		case SDLK_6: return Key::Num_6;
		case SDLK_7: return Key::Num_7;
		case SDLK_8: return Key::Num_8;
		case SDLK_9: return Key::Num_9;
		case SDLK_COLON: return Key::Colon;
		case SDLK_SEMICOLON: return Key::Semicolon;
		case SDLK_LESS: return Key::Less;
		case SDLK_EQUALS: return Key::Equals;
		case SDLK_GREATER: return Key::Greater;
		case SDLK_QUESTION: return Key::Question;
		case SDLK_AT: return Key::At;
		case SDLK_LEFTBRACKET: return Key::Leftbracket;
		case SDLK_BACKSLASH: return Key::Backslash;
		case SDLK_RIGHTBRACKET: return Key::Rightbracket;
		case SDLK_CARET: return Key::Caret;
		case SDLK_UNDERSCORE: return Key::Underscore;
		case SDLK_BACKQUOTE: return Key::Backquote;
		case SDLK_a: return Key::a;
		case SDLK_b: return Key::b;
		case SDLK_c: return Key::c;
		case SDLK_d: return Key::d;
		case SDLK_e: return Key::e;
		case SDLK_f: return Key::f;
		case SDLK_g: return Key::g;
		case SDLK_h: return Key::h;
		case SDLK_i: return Key::i;
		case SDLK_j: return Key::j;
		case SDLK_k: return Key::k;
		case SDLK_l: return Key::l;
		case SDLK_m: return Key::m;
		case SDLK_n: return Key::n;
		case SDLK_o: return Key::o;
		case SDLK_p: return Key::p;
		case SDLK_q: return Key::q;
		case SDLK_r: return Key::r;
		case SDLK_s: return Key::s;
		case SDLK_t: return Key::t;
		case SDLK_u: return Key::u;
		case SDLK_v: return Key::v;
		case SDLK_w: return Key::w;
		case SDLK_x: return Key::x;
		case SDLK_y: return Key::y;
		case SDLK_z: return Key::z;
		case SDLK_CAPSLOCK: return Key::Capslock;
			/*SDLK_F1 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F1), // TODO: Finish me
			 SDLK_F2 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F2),
			 SDLK_F3 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F3),
			 SDLK_F4 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F4),
			 SDLK_F5 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F5),
			 SDLK_F6 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F6),
			 SDLK_F7 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F7),
			 SDLK_F8 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F8),
			 SDLK_F9 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F9),
			 SDLK_F10 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F10),
			 SDLK_F11 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F11),
			 SDLK_F12 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F12),
			 
			 SDLK_PRINTSCREEN = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PRINTSCREEN),
			 SDLK_SCROLLLOCK = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SCROLLLOCK),
			 SDLK_PAUSE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAUSE),
			 SDLK_INSERT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_INSERT),
			 SDLK_HOME = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HOME),
			 SDLK_PAGEUP = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEUP),
			 SDLK_DELETE = '\177',
			 SDLK_END = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_END),
			 SDLK_PAGEDOWN = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEDOWN),*/
		case SDLK_RIGHT: return Key::Right;
		case SDLK_LEFT: return Key::Left;
		case SDLK_DOWN: return Key::Down;
        case SDLK_UP: return Key::Up;
        case SDLK_LSHIFT: return Key::LShift;
        case SDLK_RSHIFT: return Key::RShift;
        case SDLK_LCTRL: return Key::LCtrl;
        case SDLK_RCTRL: return Key::RCtrl;
			/*SDLK_NUMLOCKCLEAR = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_NUMLOCKCLEAR),
			 SDLK_KP_DIVIDE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DIVIDE),
			 SDLK_KP_MULTIPLY = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MULTIPLY),*/
		case SDLK_KP_PLUS: return Key::KP_Plus;
		case SDLK_KP_MINUS: return Key::KP_Minus;
			/*
			 SDLK_KP_MINUS = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MINUS),
			 SDLK_KP_PLUS = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PLUS),
			 SDLK_KP_ENTER = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_ENTER),
			 SDLK_KP_1 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_1),
			 SDLK_KP_2 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_2),
			 SDLK_KP_3 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_3),
			 SDLK_KP_4 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_4),
			 SDLK_KP_5 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_5),
			 SDLK_KP_6 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_6),
			 SDLK_KP_7 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_7),
			 SDLK_KP_8 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_8),
			 SDLK_KP_9 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_9),
			 SDLK_KP_0 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_0),
			 SDLK_KP_PERIOD = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PERIOD),
			 
			 SDLK_APPLICATION = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_APPLICATION),
			 SDLK_POWER = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_POWER),
			 SDLK_KP_EQUALS = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EQUALS),
			 SDLK_F13 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F13),
			 SDLK_F14 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F14),
			 SDLK_F15 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F15),
			 SDLK_F16 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F16),
			 SDLK_F17 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F17),
			 SDLK_F18 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F18),
			 SDLK_F19 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F19),
			 SDLK_F20 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F20),
			 SDLK_F21 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F21),
			 SDLK_F22 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F22),
			 SDLK_F23 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F23),
			 SDLK_F24 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F24),
			 SDLK_EXECUTE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EXECUTE),
			 SDLK_HELP = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HELP),
			 SDLK_MENU = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MENU),
			 SDLK_SELECT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SELECT),
			 SDLK_STOP = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_STOP),
			 SDLK_AGAIN = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AGAIN),
			 SDLK_UNDO = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UNDO),
			 SDLK_CUT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CUT),
			 SDLK_COPY = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_COPY),
			 SDLK_PASTE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PASTE),
			 SDLK_FIND = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_FIND),
			 SDLK_MUTE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MUTE),
			 SDLK_VOLUMEUP = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_VOLUMEUP),
			 SDLK_VOLUMEDOWN = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_VOLUMEDOWN),
			 SDLK_KP_COMMA = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_COMMA),
			 SDLK_KP_EQUALSAS400 =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EQUALSAS400),
			 
			 SDLK_ALTERASE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_ALTERASE),
			 SDLK_SYSREQ = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SYSREQ),
			 SDLK_CANCEL = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CANCEL),
			 SDLK_CLEAR = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CLEAR),
			 SDLK_PRIOR = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PRIOR),
			 SDLK_RETURN2 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RETURN2),
			 SDLK_SEPARATOR = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SEPARATOR),
			 SDLK_OUT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_OUT),
			 SDLK_OPER = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_OPER),
			 SDLK_CLEARAGAIN = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CLEARAGAIN),
			 SDLK_CRSEL = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CRSEL),
			 SDLK_EXSEL = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EXSEL),
			 
			 SDLK_KP_00 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_00),
			 SDLK_KP_000 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_000),
			 SDLK_THOUSANDSSEPARATOR =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_THOUSANDSSEPARATOR),
			 SDLK_DECIMALSEPARATOR =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DECIMALSEPARATOR),
			 SDLK_CURRENCYUNIT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CURRENCYUNIT),
			 SDLK_CURRENCYSUBUNIT =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CURRENCYSUBUNIT),
			 SDLK_KP_LEFTPAREN = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LEFTPAREN),
			 SDLK_KP_RIGHTPAREN = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_RIGHTPAREN),
			 SDLK_KP_LEFTBRACE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LEFTBRACE),
			 SDLK_KP_RIGHTBRACE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_RIGHTBRACE),
			 SDLK_KP_TAB = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_TAB),
			 SDLK_KP_BACKSPACE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_BACKSPACE),
			 SDLK_KP_A = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_A),
			 SDLK_KP_B = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_B),
			 SDLK_KP_C = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_C),
			 SDLK_KP_D = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_D),
			 SDLK_KP_E = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_E),
			 SDLK_KP_F = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_F),
			 SDLK_KP_XOR = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_XOR),
			 SDLK_KP_POWER = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_POWER),
			 SDLK_KP_PERCENT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PERCENT),
			 SDLK_KP_LESS = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LESS),
			 SDLK_KP_GREATER = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_GREATER),
			 SDLK_KP_AMPERSAND = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_AMPERSAND),
			 SDLK_KP_DBLAMPERSAND =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DBLAMPERSAND),
			 SDLK_KP_VERTICALBAR =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_VERTICALBAR),
			 SDLK_KP_DBLVERTICALBAR =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DBLVERTICALBAR),
			 SDLK_KP_COLON = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_COLON),
			 SDLK_KP_HASH = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_HASH),
			 SDLK_KP_SPACE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_SPACE),
			 SDLK_KP_AT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_AT),
			 SDLK_KP_EXCLAM = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EXCLAM),
			 SDLK_KP_MEMSTORE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMSTORE),
			 SDLK_KP_MEMRECALL = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMRECALL),
			 SDLK_KP_MEMCLEAR = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMCLEAR),
			 SDLK_KP_MEMADD = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMADD),
			 SDLK_KP_MEMSUBTRACT =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMSUBTRACT),
			 SDLK_KP_MEMMULTIPLY =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMMULTIPLY),
			 SDLK_KP_MEMDIVIDE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMDIVIDE),
			 SDLK_KP_PLUSMINUS = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PLUSMINUS),
			 SDLK_KP_CLEAR = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_CLEAR),
			 SDLK_KP_CLEARENTRY = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_CLEARENTRY),
			 SDLK_KP_BINARY = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_BINARY),
			 SDLK_KP_OCTAL = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_OCTAL),
			 SDLK_KP_DECIMAL = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DECIMAL),
			 SDLK_KP_HEXADECIMAL =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_HEXADECIMAL),
			 
			 SDLK_LCTRL = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LCTRL),
			 SDLK_LSHIFT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LSHIFT),
			 SDLK_LALT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LALT),
			 SDLK_LGUI = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LGUI),
			 SDLK_RCTRL = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RCTRL),
			 SDLK_RSHIFT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RSHIFT),
			 SDLK_RALT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RALT),
			 SDLK_RGUI = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RGUI),
			 
			 SDLK_MODE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MODE),
			 
			 SDLK_AUDIONEXT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIONEXT),
			 SDLK_AUDIOPREV = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIOPREV),
			 SDLK_AUDIOSTOP = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIOSTOP),
			 SDLK_AUDIOPLAY = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIOPLAY),
			 SDLK_AUDIOMUTE = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AUDIOMUTE),
			 SDLK_MEDIASELECT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIASELECT),
			 SDLK_WWW = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_WWW),
			 SDLK_MAIL = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MAIL),
			 SDLK_CALCULATOR = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CALCULATOR),
			 SDLK_COMPUTER = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_COMPUTER),
			 SDLK_AC_SEARCH = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_SEARCH),
			 SDLK_AC_HOME = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_HOME),
			 SDLK_AC_BACK = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_BACK),
			 SDLK_AC_FORWARD = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_FORWARD),
			 SDLK_AC_STOP = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_STOP),
			 SDLK_AC_REFRESH = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_REFRESH),
			 SDLK_AC_BOOKMARKS = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_BOOKMARKS),
			 
			 SDLK_BRIGHTNESSDOWN =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_BRIGHTNESSDOWN),
			 SDLK_BRIGHTNESSUP = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_BRIGHTNESSUP),
			 SDLK_DISPLAYSWITCH = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DISPLAYSWITCH),
			 SDLK_KBDILLUMTOGGLE =
			 SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KBDILLUMTOGGLE),
			 SDLK_KBDILLUMDOWN = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KBDILLUMDOWN),
			 SDLK_KBDILLUMUP = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KBDILLUMUP),
			 SDLK_EJECT = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EJECT),
			 SDLK_SLEEP = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SLEEP)*/
	}
	
	return Key::Unknown;
}

MouseButton SDLInterface::SDLButtonToButton(Uint8 button) {
	switch (button) {
		case SDL_BUTTON_LEFT: return MouseButton::Left; break;
		case SDL_BUTTON_MIDDLE: return MouseButton::Middle; break;
		case SDL_BUTTON_RIGHT: return MouseButton::Right; break;
		default: return MouseButton::Unknown; break;
	}
}


std::string SDLInterface::getPythonPath() const {
#ifdef _WIN32
	const char * pathsep = ";";
#else
	const char * pathsep = ":";
#endif
	return (getResourcesFolder() / "python.zip/lib").generic_string() + pathsep + (getResourcesFolder() / "python").generic_string() + pathsep;
}

/*boost::filesystem::path getResourcesFolder() {
char cwd[PATH_MAX];
getcwd(cwd, sizeof(cwd));
return boost::filesystem::path(cwd);
}*/
