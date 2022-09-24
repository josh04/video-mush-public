//
//  guiScreenSections.cpp
//  video-mush
//
//  Created by Josh McNamee on 26/08/2014.
//
//

#include "guiScreenSection.hpp"
#include <azure/sprite.hpp>
#include <azure/framebuffer.hpp>
#include <scarlet/gui.hpp>
#include <sstream>

#include "hdrEXR.hpp"

extern "C" void putLog(std::string s);
namespace mush {
	namespace gui {

		const GLfloat screen_section::sim2[] = {    // second window

			-1.0f,	-1.0f,	0.0f,	0.0f, 2.0f,
			-1.0f,	3.0f,	0.0f,	0.0f, 0.0f,
			3.0f,	3.0f,	0.0f,	2.0f, 0.0f

		/*	1.0f,	1.0f,	0.0f,	1.0f, 0.0f,
			1.0f,	-1.0f,	0.0f,	1.0f, 1.0f,
			-1.0f,	-1.0f,	0.0f,	0.0f, 1.0f*/
		};

		screen_section::screen_section(int section, unsigned int w, unsigned int h, int subSectionRows) : azure::Sprite(nullptr, nullptr), _width(w), _height(h), _subSectionRows(subSectionRows) {


			_ratio = 0.25f;

			_program = std::make_shared<azure::Program>();
			_program->addShader(azure::ShaderFactory::Get("shaders/shader.vert", GL_VERTEX_SHADER));
			if (section == (int)screenSections::sim2) {

				_program->addShader(azure::ShaderFactory::Get("shaders/ldr.frag", GL_FRAGMENT_SHADER));
				std::string cs = "rec709";

				_program->addShader(azure::ShaderFactory::Get("shaders/colorspaces/" + cs + ".frag", GL_FRAGMENT_SHADER));
			} else {
				_program->addShader(azure::ShaderFactory::Get("shaders/hdr.frag", GL_FRAGMENT_SHADER));
			}
			std::string transfer = "linear";

			_program->addShader(azure::ShaderFactory::Get("shaders/transferfunctions/" + transfer + ".frag", GL_FRAGMENT_SHADER));
			_program->link();

			_frame = std::make_shared<azure::Framebuffer>();
			_frame->setTarget(GL_TEXTURE_2D);
			_frame->createTexture(GL_RGBA32F, 1, 1, GL_RGBA, GL_FLOAT, NULL);
			_texture = _frame;//std::make_shared<azure::Texture>();

			_texture->setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			_texture->setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			//_texture->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			_texture->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//_texture->setParam(GL_GENERATE_MIPMAP, GL_TRUE);
			_texture->setParam(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			_texture->applyParams();

			_program->use();
			_program->uniform("exposure", powf(2.0f, exposure));
			//_program->uniform("width", (int)(0.8*width));
			_program->uniform("tex", (GLint)0);
			_program->uniform("scroll", (GLfloat)0.0f);
			_program->uniform("selector", (GLint)_selector);
			_program->discard();

			_mode = GL_TRIANGLES;

			_layout.push_back(azure::GLObject::Layout("vert", 3, GL_FLOAT, GL_TRUE));
			_layout.push_back(azure::GLObject::Layout("vertTexCoord", 2, GL_FLOAT, GL_FALSE));

			_first = 0;
			_count = 3;
			switch (section) {
			case (int)screenSections::main:
				setMainVertex();
				break;
			case (int)screenSections::sim2:
				_data.assign(sim2, sim2 + 15);
				break;
			default:
				setSubVertex((int)section);
				break;
			}
		}

		screen_section::~screen_section() {
			if (_document != nullptr) { _document->RemoveReference(); }
		}

		void screen_section::addGui(const char * rml, int id) {
			if (rml != nullptr) {
				_gui = std::make_shared<scarlet::GUI>(nullptr, nullptr);

				//addEventHandler(_gui);
				//addUpdateHandler(_gui);

				_gui->setViewport(glm::uvec4(0, 0, tex_width, tex_height), true);

				_document = _gui->addFile(rml);
				_document->AddReference();

				_tag = id;
				auto e = std::make_shared<azure::Event>("guiTag", nullptr);
				e->setAttribute<float>("tag", (float)id);
				scarlet::GUI::PythonEvent(_document, e);
			}
		}

		void screen_section::createTexture(unsigned int width, unsigned int height, bool test_image) {
			

			void * image = NULL;

			if (test_image) {
#ifdef _WIN32
				const char * path = "resources/TestGrid.exr";
				//unsigned int width = 0, height = 0;
				hdrEXR::ReadSize(path, width, height);
				image = malloc(width * height * 2 * sizeof(float));
				mush::buffer buf((unsigned char *)image);
				hdrEXR::ReadEXR(buf, path);
#endif
			}

			tex_width = width;
			tex_height = height;

			float div = 1.0f;
			if (_subSectionRows) {
				div = (float)_subSectionRows / 4;
			}
			float widthadd = 0.2f / div; // from above but 0 - 1 rather than -1 - 1
			add_x = (int)(widthadd * _width);
			multratio_h = tex_height / (float)_height;
			multratio_w = tex_width / (float)(_width - add_x);


			_frame->createTexture(GL_RGBA32F, width, height, GL_RGBA, GL_HALF_FLOAT, image);
			_frame->attach();

			if (image != NULL) {
				free(image);
			} else {
				_frame->clear();
			}

			if (_gui) {
				//_frame->bind();
				//_gui->getContext()->SetDimensions(Rocket::Core::Vector2i(_width, _height));
				_gui->setViewport(glm::uvec4(0, 0, width, height));
				//_frame->unbind();
			}
			//_frame->clear();
		}

		glm::ivec2 screen_section::get_size() const {
			auto ret = glm::ivec2(tex_width, tex_height);
			switch ((select)_selector) {
			case select::anaglyph:
			case select::left_channel:
			case select::right_channel:
			case select::stereo:
				ret.x = ret.x / 2;
			default:
				break;
			}
			return ret;
		}

		void screen_section::render(unsigned int width, unsigned int scissor_height, unsigned int height, int stereo_pass) {
			if (_stereo) {
				//if (stereo_pass != 0) {
					if (stereo_pass == 2) {
						set_stereo_left();
					} else {
						set_stereo_right();
					}
				//}
			}
			_program->use();
			if (_gui != nullptr) {
				_frame->bind();
				_gui->update(azure::Duration(1));
				_gui->render();
				_frame->unbind();
			}

			glEnable(GL_SCISSOR_TEST);
			glScissor(0, scissor_height, width, _height);
			_glException();
			glViewport(0, 0, width, height);
			_glException();

			azure::Sprite::render();
			_program->discard();
			glDisable(GL_SCISSOR_TEST);

		}

		float screen_section::addExposure(const float a) {
			if (useExposure) {
				exposure += a;
				_program->use();
				_program->uniform("exposure", powf(2.0f, exposure));
				_program->discard();
			}
			return exposure;
		}

		float screen_section::subtractExposure(const float a) {
			if (useExposure) {
				exposure -= a;
				_program->use();
				_program->uniform("exposure", powf(2.0f, exposure));
				_program->discard();
			}
			return exposure;
		}

		float screen_section::getExposure() const {
			return exposure;
		}

		void screen_section::setExposure(const float a) {
			if (useExposure) {
				exposure = a;
				_program->use();
				_program->uniform("exposure", powf(2.0f, exposure));
				_program->discard();
			}
		}

		void screen_section::setScroll(const float a) {
			scroll = a;
			_program->use();
			_program->uniform("scroll", scroll);
			_program->discard();
		}

		void screen_section::setSelector(const select s) {
			_selector = (GLint)s;

			_program->use();
			_program->uniform("selector", _selector);
			_program->discard();
		}

		void screen_section::mipmap() {
			_texture->bind();
			glGenerateMipmap(GL_TEXTURE_2D);
			_glException();
			_texture->unbind();
		}

		void screen_section::setData(const std::vector<GLfloat> newData) {
			_data = newData;

			buffer();
			attach();
		}

		void screen_section::setMainVertex(bool doWide) {
			/*
			float div = 1.0f;
			if (_subSectionRows) {
				if (_subSectionRows > 7) {
					div = (float)_subSectionRows / 4.444444444f;
				} else if (_subSectionRows > 3) {
					div = (float)_subSectionRows / 4.0f;
				}
			}
			float heightadd = 0.0f, widthadd = 0.4f / div;
			*/
			float heightadd = 0.0f, widthadd = 2.0f / (float)(_subSectionRows + 1);

			if (doWide) {
				//                heightadd = 0.4f;
			} else {
				widthadd = 0.0f;
			}

			GLfloat temp[] = {
				-1.0f + widthadd,          -1.0f + (heightadd),	0.0f,	0.0f, 2.0f,
				-1.0f + widthadd,           3.0f,               0.0f,	0.0f, 0.0f,
				3.0f,                       3.0f,               0.0f,	2.0f, 0.0f
/*
				1.0f,                      1.0f,               0.0f,	1.0f, 0.0f,
				1.0f,                     -1.0f + (heightadd),	0.0f,	1.0f, 1.0f,
				-1.0f + widthadd,          -1.0f + (heightadd),	0.0f,	0.0f, 1.0f*/
			};

			_data.assign(temp, temp + 15);
			buffer();
			attach();
		}

		void screen_section::setSubVertex(const int index) {
			/*float div = 1.0f;
			if (_subSectionRows) {
				if (_subSectionRows > 7) {
					div = (float)_subSectionRows / 4.444444444f;
				}
				else if (_subSectionRows > 3) {
					div = (float)_subSectionRows / 4.0f;
				}
			}
			*/
			//float heightadd = 0.0f, widthadd = 0.4f / div;
			float heightadd = 0.0f, widthadd = 4.0f;// / (float)(_subSectionRows + 1);
			float widthpos = 0.0f, heightpos = 0.0f;
			//            if (count > 5) {
			//                heightadd = 0.4f;
			//            } else {
			heightadd = 0.5;
			if (_subSectionRows) {
				heightadd = 2.0f / (float)_subSectionRows;
				heightadd = _ratio;
				//heightadd = ( widthadd / 16.0f ) * 9.0f;
			}
			//            }
			heightpos = 0;//_subSectionRows-index;
			//            if (count > 5) {
			//                heightpos++;
			//            }

			GLfloat temp[] = {
				-3.0f + widthadd*(widthpos + 1),          1.0f - 2.0f*heightadd*(heightpos + 1),      0.0f,	0.0f, 2.0f,
				-3.0f + widthadd*(widthpos + 1),          1.0f,	         0.0f,	0.0f, 0.0f,
				-3.0f + widthadd*(widthpos),              1.0f,	         0.0f,	2.0f, 0.0f

				/*-1.0f + widthadd*(widthpos),              1.0f - heightadd*(heightpos),	         0.0f,	1.0f, 0.0f,
				-1.0f + widthadd*(widthpos),              1.0f - heightadd*(heightpos + 1),      0.0f,	1.0f, 1.0f,
				-1.0f + widthadd*(widthpos + 1),          1.0f - heightadd*(heightpos + 1),      0.0f,	0.0f, 1.0f*/
			};

			_data.assign(temp, temp + 15);
			buffer();
			attach();
		}

		bool screen_section::doEvent(std::shared_ptr<azure::Event> event) {
			if (_gui != nullptr) {
				/*
				if (event->isType("currentFrame")) {
					int x = event->getAttribute<int>("tag");
					if (x == _tag) {

						int y = event->getAttribute<int>("frame");
						int t = event->getAttribute<int>("total_frames");
						auto label = _document->GetElementById("framecount");
						std::stringstream strm;
						strm << y << " / " << t;
						label->SetInnerRML(strm.str().c_str());
					}
				*/

				if (event->isType("setGuiText")) {
					int x = event->getAttribute<int>("tag");
					if (x == _tag) {
						std::string y = event->getAttribute<std::string>("text");
						std::string t = event->getAttribute<std::string>("label");
						auto label = _document->GetElementById(t.c_str());
						if (label != NULL) {
							label->SetInnerRML(y.c_str());
						}
					}
					return false;
				} else if (event->isType("mouseMove")) {
					int x = event->getAttribute<int>("x");
					int y = event->getAttribute<int>("y");

					auto event2 = std::make_shared<azure::Event>(*event);
					event2->setAttribute<int>("x", (x - add_x)*multratio_w);
					event2->setAttribute<int>("y", y*multratio_h);

					if (scarlet::GUI::PythonEvent(_document, event2)) {
						return true;
					}

					return _gui->event(event2);
				} else if (event->isType("windowResize")) {
					return false;
				} else {
					if (scarlet::GUI::PythonEvent(_document, event)) {
						return true;
					}

					return _gui->event(event);
				}
			} else {
				return false;
			}
		}

		void screen_section::flip() {
			_data[1] = -_data[1];
			_data[6] = -_data[6];
			_data[11] = -_data[11];
/*
			_data[16] = -_data[16];
			_data[21] = -_data[21];
			_data[26] = -_data[26];
*/
			buffer();
			attach();
		}

		void screen_section::set_stereo_left() {
			_data[3] = 0.0f;
			_data[8] = 0.0f;
			_data[13] = 1.0f;
/*
			_data[18] = 0.5f;
			_data[23] = 0.5f;
			_data[28] = 0.0f;
*/
			buffer();
			attach();
		}

		void screen_section::set_stereo_right() {
			_data[3] = 1.0f;
			_data[8] = 1.0f;
			_data[13] = 2.0f;
/*
			_data[18] = 1.0f;
			_data[23] = 1.0f;
			_data[28] = 0.5f;
*/
			buffer();
			attach();
		}
	}
}
