//
//  guiScreenSection.hpp
//  video-mush
//
//  Created by Josh McNamee on 26/08/2014.
//
//

#ifndef video_mush_guiScreenSection_hpp
#define video_mush_guiScreenSection_hpp

#include <azure/sprite.hpp>
#include <vector>
#include "mush-core-dll.hpp"

namespace Rocket {
    namespace Core {
        class ElementDocument;
    }
}

namespace azure {
    class Event;
    class Framebuffer;
    class Eventable;
}

namespace scarlet {
    class GUI;
}

namespace mush {
	namespace gui {
		class MUSHEXPORTS_API screen_section : public azure::Sprite {
		public:

			enum class screenSections : int {
				main = 0,
				sim2 = 1000
			};

			screen_section(int section, unsigned int w, unsigned int h, int subSectionRows);

			~screen_section();

			void addGui(const char * rml, int id);

			void createTexture(unsigned int width, unsigned int height, bool test_image = false);

			void setName(const std::string name) { _name = name; }

			std::string getName() { return _name; }

			void render(unsigned int width, unsigned int scissor_height, unsigned int height, int stereo_pass);

			float addExposure(const float a);

			float subtractExposure(const float a);

			float getExposure() const;

			void setExposure(const float a);

			bool getUseExposure() { return useExposure; }

			void setUseExposure(const bool a) { useExposure = a; }

			float getScroll() { return scroll; }

			void setScroll(const float a);

			enum class select {
				passthrough = 0,
				lum = 1,
				u = 2,
				v = 3,
				r = 4,
				g = 5,
				b = 6,
				rrr = 7,
				ggg = 8,
				bbb = 9,
				anaglyph = 10,
				left_channel = 11,
				right_channel = 12,
				stereo = 13
			};

			void setSelector(const select s);
			select getSelector() const { return (select)_selector; }

			static const char * getSelectName(select s) {
				switch (s) {
				case select::passthrough:
				default:
					return "";
				case select::lum:
					return "Luminance";
				case select::u:
					return "Chroma U";
				case select::v:
					return "Chroma V";
				case select::r:
					return "Red";
				case select::g:
					return "Green";
				case select::b:
					return "Blue";
				case select::rrr:
					return "Red Channel";
				case select::ggg:
					return "Green Channel";
				case select::bbb:
					return "Blue Channel";
				case select::anaglyph:
					return "Anaglyph";
				case select::left_channel:
					return "Left Channel";
				case select::right_channel:
					return "Right Channel";
				case select::stereo:
					return "Stereo";
				}
			}

			void mipmap();

			std::vector<GLfloat> getData() const { return _data; }

			void setData(const std::vector<GLfloat> newData);

			bool doEvent(std::shared_ptr<azure::Event> event);

			void resize(unsigned int width, unsigned int height) {

				//_ratio = 2.0f /( height / ( ( (width / (_subSectionRows + 1)) / 16.0f ) * 9.0f ) );
				//float aspect = height / width;

				float box = 9.0f * (width / ((_subSectionRows + 1) * 16.0f));

				float a = height / box;

				_ratio = 2.0f / a;

				setSubVertex(1);
				_width = width;
				_height = height;

				float div = 1.0f;
				if (_subSectionRows) {
					div = (float)_subSectionRows / 4;
				}
				float widthadd = 0.2f / div; // from above but 0 - 1 rather than -1 - 1
				add_x = (int)(widthadd * width);
				multratio_h = tex_height / (float)height;
				multratio_w = tex_width / (float)(width - add_x);
			}

			void pokeMainVertex(unsigned int width, unsigned int height) {
				_width = width;
				_height = height;
				setMainVertex(true);
				buffer();
				attach();
			}

			void set_fps_pointer(float * fps) {
				_fps = fps;
			}

			float * get_fps_pointer() const {
				return _fps;
			}

			float get_ratio() const {
				return _ratio;
			}

			void flip();


			glm::ivec2 get_size() const;

			void set_eventables(std::vector<std::shared_ptr<azure::Eventable>> evs) {
				_evs = evs;
			}

			std::vector<std::shared_ptr<azure::Eventable>> get_eventables() const {
				return _evs;
			}

			void set_recording(bool recording) {
				_recording = recording;
			}

			bool get_recording() const {
				return _recording;
			}
			
			void set_stereo(bool stereo) {
				_stereo = stereo;
			}

			bool get_stereo() const {
				return _stereo;
			}

		private:
			unsigned int _width = 1920, _height = 1080;
			unsigned int tex_width = 1920, tex_height = 1080;

			void setMainVertex(bool doWide = false);
			void setSubVertex(const int index);
			void set_stereo_left();
			void set_stereo_right();

			float exposure = 0.0f;
			float scroll = 0.0f;
			bool useExposure = true;

			int _subSectionRows = 4;

			const static GLfloat sim2[];

			std::string _name = "";

			std::shared_ptr<scarlet::GUI> _gui = nullptr;

			Rocket::Core::ElementDocument * _document = nullptr;

			std::shared_ptr<azure::Framebuffer> _frame = nullptr; // <3 becca

			int add_x = 0;
			float multratio_w = 1.0f, multratio_h = 1.0f;

			GLint _selector = 0;

			int _tag = -1;

			float * _fps = nullptr;

			float _ratio = 1.0f;

			std::vector<std::shared_ptr<azure::Eventable>> _evs;

			bool _recording = false;

			bool _stereo = false;
		};
	}
}

#endif
