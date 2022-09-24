//
//  imguiDrawer.hpp
//  space
//
//  Created by Josh McNamee on 07/10/2015.
//  Copyright © 2015 josh04. All rights reserved.
//

#ifndef imguiDrawer_hpp
#define imguiDrawer_hpp

#include <array>

#include <azure/Eventable.hpp>
#include <azure/Events.hpp>
#include <azure/Eventkey.hpp>

#include <azure/engine.hpp>

#include "imgui/imgui.h"

//#include "pngReader.hpp"

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplGlfwGL3_RenderDrawLists(ImDrawData* draw_data);
void ImGui_ImplGlfwGL3_CreateFontsTexture();
bool ImGui_ImplGlfwGL3_CreateDeviceObjects();

class imguiDrawer {
public:
    imguiDrawer(unsigned int width, unsigned int height) : _width(width), _height(height) {
		mouseButtons = { false, false, false };
        ImGuiIO& io = ImGui::GetIO();
        // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
        io.KeyMap[ImGuiKey_Tab] = (int)azure::Key::Tab;
        io.KeyMap[ImGuiKey_LeftArrow] = (int)azure::Key::Left;
        io.KeyMap[ImGuiKey_RightArrow] = (int)azure::Key::Right;
        io.KeyMap[ImGuiKey_UpArrow] = (int)azure::Key::Up;
        io.KeyMap[ImGuiKey_DownArrow] = (int)azure::Key::Down;
        io.KeyMap[ImGuiKey_PageUp] = (int)azure::Key::PageUp;
        io.KeyMap[ImGuiKey_PageDown] = (int)azure::Key::PageDown;
        io.KeyMap[ImGuiKey_Home] = (int)azure::Key::Home;
        io.KeyMap[ImGuiKey_End] = (int)azure::Key::End;
        io.KeyMap[ImGuiKey_Delete] = (int)azure::Key::Delete;
        io.KeyMap[ImGuiKey_Backspace] = (int)azure::Key::Backspace;
        io.KeyMap[ImGuiKey_Enter] = (int)azure::Key::Return;
        io.KeyMap[ImGuiKey_Escape] = (int)azure::Key::Escape;
        /*io.KeyMap[ImGuiKey_A] = (int)azure::Key::a;
        io.KeyMap[ImGuiKey_C] = (int)azure::Key::c;
        io.KeyMap[ImGuiKey_V] = (int)azure::Key::v;
        io.KeyMap[ImGuiKey_X] = (int)azure::Key::x;
        io.KeyMap[ImGuiKey_Y] = (int)azure::Key::y;
        io.KeyMap[ImGuiKey_Z] = (int)azure::Key::z;*/
        
        io.RenderDrawListsFn = ImGui_ImplGlfwGL3_RenderDrawLists;
        // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
        
        /*io.SetClipboardTextFn = ImGui_ImplGlfwGL3_SetClipboardText;
        io.GetClipboardTextFn = ImGui_ImplGlfwGL3_GetClipboardText;*/
        
    }
    
    ~imguiDrawer();
    
    void preRender();
    void render();
    
    void keyChange(azure::Key k, bool down) {
        ImGuiIO& io = ImGui::GetIO();
        io.KeysDown[(int)k] = down;
        
        io.KeyCtrl = io.KeysDown[(int)azure::Key::LCtrl] || io.KeysDown[(int)azure::Key::RCtrl];
        io.KeyShift = io.KeysDown[(int)azure::Key::LShift] || io.KeysDown[(int)azure::Key::RShift];
        io.KeyAlt = io.KeysDown[(int)azure::Key::LAlt] || io.KeysDown[(int)azure::Key::RAlt];
    }
    
    void mouseDown(azure::MouseButton k) {
        mouseButtons[(int)k - 1] = true;
    }
    
    void mouseUp(azure::MouseButton k) {
        mouseButtons[(int)k - 1] = false;
    }
    
    void mouseScroll(int s) {
#ifdef __APPLE__
        scroll += s / 5.0f; // magic number
#endif
#ifdef _WIN32
        scroll += s / 5.0f; // magic number
#endif
    }
    
    void mouseMove(int x, int y) {
        
		m_x = x * (_width / (float)_s_width);
		m_y = y * (_height / (float)_s_height);
        
    }
    
    void textEntry(std::string text) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharactersUTF8(text.c_str());
    }
    
    void resize(int width, int height, int s_width, int s_height);
    
private:
    unsigned int _width = 0, _height = 0;
    unsigned int _s_width = 0, _s_height = 0;
    
    int m_x = -1, m_y = -1;
    float scroll = 0;
    std::array<bool, 3> mouseButtons;
    
};

#endif /* imguiDrawer_hpp */
