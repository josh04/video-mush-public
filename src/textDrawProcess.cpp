//
//  textDrawProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 05/09/2016.
//
//

#include <sstream>


#include <boost/algorithm/string.hpp>

#include <azure/eventtypes.hpp>

#include "exports.hpp"
#include "imguiDrawer.hpp"
#include "textDrawProcess.hpp"
#include "imguiEventHandler.hpp"
#include "imgui/imgui_internal.h"

namespace mush {
    textDrawProcess::textDrawProcess(std::string text, unsigned int width, unsigned int height, const char * resource_dir, std::array<float, 4> bg_colour, std::array<float, 4> text_colour) : mush::openglProcess(width, height), _text(text), _resource_dir(resource_dir), _text_colour(text_colour) {
        _bg_colour = bg_colour;
    }
    
    void textDrawProcess::release() {
        release_local_gl_assets();
        imageProcess::release();
    }
    
    textDrawProcess::~textDrawProcess() {
    }
    
    void textDrawProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		opengl_init(context);

        assert(buffers.size() == 0 || buffers.size() == 1);
        
        if (buffers.size() == 1 && buffers.begin()[0] != nullptr) {
            _input = castToImage(buffers.begin()[0]);
        }
        
        _imgui_drawer = std::make_shared<imguiDrawer>(_width, _height);
        _imgui_event_handler = std::make_shared<imguiEventHandler>(_imgui_drawer);
        
        _imgui_drawer->resize(_width, _height, _width, _height);
        
        videoMushAddEventHandler(_imgui_event_handler);
        
        ImGuiIO& io = ImGui::GetIO();

        //auto path = boost::filesystem::path(_resource_dir) / "assets" / "PTS55F.ttf";
        auto path = boost::filesystem::path("/Users/josh04/Documents/LucidaConsole.ttf");
        //ImFont * font = io.Fonts->AddFontFromFileTTF(path.generic_string().c_str(), 10.0f);
        //auto path = boost::filesystem::path("/Users/josh04/Development/imgui/misc/fonts/ProggyClean.ttf");
        ImFont * font = io.Fonts->AddFontFromFileTTF(path.generic_string().c_str(), 14.0f);

		addItem(context->floatImage(_width, _height));
    }
    
    void textDrawProcess::process() {
        
        
        _tick++;
		bind_gl();
		unbind_gl();
        
        if (_input.get() != nullptr) {
        
            auto ptr = _input->outLock();
            if (ptr == nullptr) {
                _input = nullptr;
            } else {
                cl::Event eventn;
        
                _flip->setArg(0, ptr.get_image());
                _flip->setArg(1, *_frameCL);

                queue->enqueueNDRangeKernel(*_flip, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &eventn);
                eventn.wait();
            
                _input->outUnlock();
            }
        }
        
		bind_gl(false);
        
        _imgui_drawer->preRender();
               
        //ImGuiIO& io = ImGui::GetIO();
                
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;
        
        ImGui::SetNextWindowCollapsed(false, ImGuiSetCond_Always);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Always);
        ImGui::SetNextWindowSize(ImVec2(_width, _height), ImGuiSetCond_Always);
        ImGui::Begin("main_menu", NULL, ImVec2(0, 0), 0.0f, window_flags);
        
        ImGui::PushStyleColor(ImGuiCol_Text, {_text_colour[0], _text_colour[1], _text_colour[2], _text_colour[3]});
        //ImGui::SetWindowFontScale(5.0f);
        std::string output_text = "";
        
        int num_chars = std::floor(_tick * _speed / _fps);
        
        std::string sub = _text.substr(0, num_chars);
        output_text += sub;
        
        auto next_space_pos = _text.find(" ", num_chars);
        bool did_cursor = false;
        if (_tick % (int)(_fps) < _fps/2 /*&& sub.length() != _text.length()*/) {
            output_text += _cursor;
            did_cursor = true;
        }
        
        if (next_space_pos != std::string::npos) {
            
            int did = 0; // did_cursor ? 1 : 0;
            
            int lim = next_space_pos - num_chars - did;
            
            auto temp_text = output_text;
            
            temp_text = _text.substr(0, num_chars + lim + 2) + " ";
            
            
            if (did_cursor) {
                temp_text += _cursor;
            }
            /*
            for (int i = 0; i < lim; ++i) {
                temp_text +="a";
            }
             */
            
            /*
            auto a = temp_text.c_str();
            auto c = font->CalcWordWrapPositionA(font->Scale, temp_text.c_str(), temp_text.c_str() + temp_text.size(), _width);
            
            usleep(1);
             */
			auto win = ImGui::GetCurrentWindow();
			const float wrap_width = ImGui::CalcWrapWidthForPos(win->DC.CursorPos, 0.0f) / 2.0f;

            auto font = ImGui::GetWindowFont();
            int i = 0;
            const char * wrap = output_text.c_str();
            while (wrap != output_text.c_str() + output_text.size()) {
                i++;
                wrap = font->CalcWordWrapPositionA(font->Scale, wrap+1, output_text.c_str() + output_text.size(), wrap_width);
            }
            const char * wrap2 = font->CalcWordWrapPositionA(font->Scale, temp_text.c_str(), temp_text.c_str() + temp_text.size(), wrap_width);
            for (int j = 1; j < i; j++) {
                wrap2 = font->CalcWordWrapPositionA(font->Scale, wrap2+1, temp_text.c_str() + temp_text.size(), wrap_width);
            }
            
            if (wrap2 != temp_text.c_str() + temp_text.size()) {
                size_t dist = wrap2 - temp_text.c_str();
                
                auto p1 = output_text.substr(0, dist);
				std::string p2 = "";
				if (dist < output_text.size()) {
					p2 = output_text.substr(dist + 1, output_text.size() - dist);
				}
                output_text = p1 + (char)27 + p2;
            }

        }
        
		std::vector<std::string> frags;
		boost::split(frags, output_text, boost::is_any_of(std::string{ '\n' }));
        
        
		for (auto& frag : frags) {


			boost::replace_all(frag, std::string{ 27 }, "\n");

			ImGui::TextWrapped(frag.c_str());
		}
        //ImGui::TextUnformatted(output_text.c_str());
        auto y = ImGui::GetScrollMaxY();
        ImGui::SetScrollY(y);
        //ImGui::SetScrollPosHere();
        ImGui::PopStyleColor();
        ImGui::End();

        
        
        _imgui_drawer->render();
        
        _glException();
		unbind_gl();
        
		process_gl();
    }
    
    
    void textDrawProcess::release_local_gl_assets() {

        _imgui_drawer = nullptr;
        
        if (_imgui_event_handler != nullptr) {
            videoMushRemoveEventHandler(_imgui_event_handler);
        }
        
        _imgui_event_handler = nullptr;
		release_gl_assets();
    }
}
