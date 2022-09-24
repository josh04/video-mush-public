//
//  tagInGui.hpp
//  video-mush
//
//  Created by Josh McNamee on 26/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_tagInGui_hpp
#define media_encoder_tagInGui_hpp

#include <azure/eventable.hpp>

#include "mush-core-dll.hpp"
#include "mushBuffer.hpp"

#include <mutex>
#include <string>
#include <vector>
#include <atomic>

#include <azure/chrono.hpp>
struct _cl_mem;
typedef struct _cl_mem * cl_mem;
class hdrGui;
namespace mush {
    class opencl;
    class imageBuffer;
    class guiAccessible;
    class clToTIFF;
	class buffer;
}

namespace cl {
    class ImageGL;
    class Kernel;
    class Image2D;
	class CommandQueue;
	class Memory;
}

namespace azure {
	class Texture;
}

typedef unsigned int GLuint;

namespace hdrEXR {
    class clToEXR;
}
//#include "hdrEXR.hpp"

class MUSHEXPORTS_API tagInGui : public azure::Eventable, public std::enable_shared_from_this<tagInGui> {
public:
	tagInGui() : _working(false) {
        
    }
    
	~tagInGui() {
		_working = false;
    }
    
	void init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::guiAccessible>>& buffers, const char * exrDir, unsigned int subScreenRows);
    
	void createGLContext(bool sim2preview, bool fullscreen, const char * resourceDir);
    
	std::shared_ptr<mush::opencl> createInteropContext(const char *resourceDir, bool openclCPU);
	
	void postContextSetup();

	void initEXROut();
	
	void update();
    
	void copyImageIntoGuiBuffer(int index, const mush::buffer& image);
    
	std::unique_lock<std::mutex> getGuiLock() { std::unique_lock<std::mutex> lock(tagMutex); return lock; }
    
	std::shared_ptr<mush::opencl> getContext() const {
        return context;
    }
    
    // in most situations this would be an /incredibly/ bad idea to use
	void setContext(std::shared_ptr<mush::opencl> ctx) {
        context = ctx;
    }
    
	bool event(std::shared_ptr<azure::Event> event);
    
	void fireQuitEvent();

	void addFakeSubScreen(unsigned int width, unsigned int height);
	void addSubScreen(std::shared_ptr<mush::guiAccessible> buffer);
    
	void addEventHandler(std::shared_ptr<azure::Eventable> hand);
	void removeEventHandler(std::shared_ptr<azure::Eventable> hand);
	
	void drop_gl_cl_interop();

    void set_gui_catch_escape(bool c);
    
	std::shared_ptr<hdrGui> gui;
protected:

	void fps_tick(int index);
	void acquireCL(const cl::Memory& buf);
	void releaseCL(const cl::Memory& buf);
    
    void writeEXR();
    
    void writeEXRs();
    
    cl::CommandQueue * queue;
    size_t _count = 0;
	std::shared_ptr<mush::opencl> context = nullptr;
    
    std::vector<std::string> names;
    std::vector<mush::buffer> surfaces;
    std::vector<cl::Memory> surface_images;
    std::vector<unsigned int> widths, heights;
    std::vector<azure::TimePoint> last_frame_times;
    std::vector<int> fps_counters;
    std::deque<float> fps_s; // consistent memory addresses
    
    std::vector<bool> tiff_outputs;
    
    std::mutex tagMutex;
	cl::Kernel * _copy = nullptr;
    std::shared_ptr<hdrEXR::clToEXR> exrOut = nullptr;
    std::shared_ptr<mush::clToTIFF> tiffOut = nullptr;
    
#ifdef _WIN32
//	HGLRC glCtx;
//	HDC glHDC2;
#endif
#ifdef __APPLE__
//	CGLContextObj cgl_context;
#endif
    
    std::atomic<bool> _working;
    
	const char * exrDir = nullptr;
    
    bool registeredEventHandler = false;
    
    int _screen_count = 0;
private:
};

#endif
