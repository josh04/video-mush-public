#define __CL_ENABLE_EXCEPTIONS

#include <iostream>
#include <sstream>

#include <boost/filesystem.hpp>

#include "opencl.hpp"
//#include "hdrControl.hpp"

#ifdef _WIN32
#endif
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/GL3.h>
#include <OpenGL/CGLCurrent.h>
#endif

using namespace mush;

extern "C" void putLog(std::string s);

opencl::opencl(const char * resourceDir, bool openclCPU) :
	formatFloat(cl::ImageFormat(CL_RGBA, CL_FLOAT)),
	formatHalf(cl::ImageFormat(CL_RGBA, CL_HALF_FLOAT)),
	formatGrey(cl::ImageFormat(CL_R, CL_FLOAT)),
	formatRedGreen(cl::ImageFormat(CL_RG, CL_FLOAT)),
	formatInt(cl::ImageFormat(CL_RGBA, CL_UNORM_INT8)),
	formatBGRAInt(cl::ImageFormat(CL_BGRA, CL_UNORM_INT8)),
	formatIntNotNormalised(cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8)),
	format16bitInt(cl::ImageFormat(CL_RGBA, CL_UNORM_INT16)),
	resourceDir(resourceDir),
	openclCPU(openclCPU) {

	putLog("---- CL Wrapper Initialised");
}

void opencl::init(bool withGL) {
    
    putLog("--- Init CL");
    
    std::vector<cl_context_properties> props;
    if (withGL) {
        props = buildPropsWithGL(openclCPU);
    } else {
        props = buildProps(openclCPU);
    }
    
    putLog("--- Making context");
    try {
		auto type = CL_DEVICE_TYPE_GPU;
		if (openclCPU) {
			type = CL_DEVICE_TYPE_CPU;
		}
        context = new cl::Context(type, &props[0]);
    } catch (cl::Error& e) {
        putLog(e.what());
        std::stringstream strm;
        strm << e.err();
        
        putLog(strm.str());
        throw std::runtime_error("CL Context Making Failed");
    }
    initCommon();
}

void opencl::init(cl::Context * ctx) {
    context = new cl::Context(*ctx);
    initCommon();
}

void opencl::initCommon() {
    devices = context->getInfo<CL_CONTEXT_DEVICES>();
    
    putLog("--- Adding sources");
	addKernels({});
    
    putLog("--- Making program");
    makeProgram();
    cl_command_queue_properties cq_props;
    devices[0].getInfo(CL_DEVICE_QUEUE_PROPERTIES, &cq_props);
    
    std::string device_name = "";
    devices[0].getInfo(CL_DEVICE_NAME, &device_name);
    
    putLog("--- Using device: " + device_name);
    
    cl_command_queue_properties use_props = 0;
    if (cq_props & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) {
        use_props = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
    }
    queue = new cl::CommandQueue(*context, devices[0], use_props, NULL);
}

int opencl::getPlatformId(std::vector<cl::Platform> platforms, bool openclCPU) {
    
    putLog("--- Selecting Platform");
    
    int platform_id = -1;
    for (int i = 0; i < platforms.size(); ++i) {
        std::vector<cl::Device> devices;
        try {
			auto type = CL_DEVICE_TYPE_GPU;
			if (openclCPU) {
				type = CL_DEVICE_TYPE_CPU;
			}
            platforms[i].getDevices(type, &devices);
        } catch (cl::Error& e) {
			/*std::stringstream strm;
			strm << e.err();
			putLog(strm.str());*/

        }
        if (devices.size() > 0) {
            platform_id = i;
        }
    }
    std::stringstream strm;
    std::string platform_name = "";
    platforms[platform_id].getInfo(CL_PLATFORM_NAME, &platform_name);
    strm << "--- Selected Platform: " << platform_id << ": " << platform_name;
    putLog(strm.str());
    
    if (platform_id == -1) {

        throw std::runtime_error("Failed to find platform.");
    }
    return platform_id;
}

std::vector<cl_context_properties> opencl::buildProps(bool openclCPU) {
    
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    int platform_id = getPlatformId(platforms, openclCPU);
    
    std::vector<cl_context_properties> props = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[platform_id])(),
        0
    };
    
    return props;
}


std::vector<cl_context_properties> opencl::buildPropsWithGL(bool openclCPU) {
    
#if defined(_WIN32)
    HGLRC glCtx = wglGetCurrentContext();
    HDC glHDC2 = wglGetCurrentDC();
#elif defined(__linux__)
    GLXContext glCtx = glXGetCurrentContext();
#elif defined(__APPLE__)
    CGLContextObj cgl_context = CGLGetCurrentContext();
    CGLShareGroupObj sharegroup = CGLGetShareGroup(cgl_context);
    gcl_gl_set_sharegroup(sharegroup);
    
    CGLError err;
    err = CGLEnable( cgl_context, kCGLCEMPEngine);
    
    if (err != kCGLNoError) {
        abort();
    }
#endif
    
    
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    int platform_id = getPlatformId(platforms, openclCPU);
    
    std::vector<cl_context_properties> props = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[platform_id])(),
#if defined(_WIN32)
        CL_GL_CONTEXT_KHR, (cl_context_properties)glCtx,
        CL_WGL_HDC_KHR, (cl_context_properties)glHDC2,
#elif defined(__linux__)
        CL_GL_CONTEXT_KHR, (cl_context_properties)glCtx,
        CL_GLX_DISPLAY_KHR, (intptr_t)glXGetCurrentDisplay(),
#elif defined(__APPLE__)
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)sharegroup,
        //CL_CGL_SHAREGROUP_KHR, (cl_context_properties)sharegroup,
#endif
        0
    };
    
    return props;
}

opencl::~opencl() {
	size_t s = buffers.size();
	for (int i = 0; i < s; ++i) {
		cl::Memory * mem = buffers.back();
		buffers.pop_back();
        
        if (mem != nullptr) {
            delete mem;
        }
	}

	size_t k = kernels.size();
	for (int i = 0; i < k; ++i) {
		cl::Kernel * ker = kernels.back();
		kernels.pop_back();
        if (ker != nullptr) {
            delete ker;
        }
	}
	delete queue;
	delete program;
	delete context;
}

void opencl::addKernels(std::vector<std::string> kernels) {
/*	for (std::vector<std::string>::iterator it = kernels.begin(); it != kernels.end(); ++it) {
		addKernel(*it);
	}
  */  
    std::string path = std::string(resourceDir) + "/kernels/";

    boost::filesystem::directory_iterator end_itr;
    
    for ( boost::filesystem::directory_iterator itr( path ); itr != end_itr; ++itr ) {
        if (itr->path().extension().string() == ".cl") {
            std::string path = itr->path().string();
            addKernel(path);
        }
    }
    
}

void opencl::addKernel(std::string path) {
    std::ifstream file(path.c_str());
    std::string source(std::istreambuf_iterator<char>(file), (std::istreambuf_iterator<char>()));
    //			cout << source << endl;
    sources.push_back(source);
}

void opencl::makeProgram() {
	cl::Program::Sources pSources;

    // EXTRA SEGMENT FOR RE-MAKING PROGRAM
    // TERRIBLE IDEA, ALMOST NEVER APPROPRIATE
    std::vector<std::string> kernel_names;
    if (kernels.size() > 0) {
        for (auto k : kernels) {
            std::string name = "";
            if (k->getInfo(CL_KERNEL_FUNCTION_NAME, &name) == CL_SUCCESS) {
                kernel_names.push_back(name);
            }
        }
    }
    
    if (program != nullptr) {
        delete program;
    }
    
    // ENDS HERE
    
	for (std::vector<std::string>::iterator it = sources.begin(); it != sources.end(); ++it) {
		pSources.push_back(std::make_pair(it->c_str(), it->size()));
	}

	try {
		program = new cl::Program(*context, pSources);
		program->build(devices);
        
        // ALSO THIS BIT
        int i = 0;
        for (auto name : kernel_names) {
            cl::Kernel * new_kernel = getKernel(name.c_str(), false);
            *kernels[i] = *new_kernel;
            i++;
        }
        // BLEURGH
	} catch (cl::Error &e) {
		std::string str = program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
		putLog(str);
		return;
	} catch (std::exception &e) {
		putLog("oh shit");
		return;
    }
    
    std::string str = program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
    putLog(str);
    
}

unsigned char * opencl::hostReadBuffer(unsigned int size, bool store) {
	cl::Buffer * ptr = new cl::Buffer(*context, (CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR), size, NULL, NULL);
    if (store) {
        buffers.push_back(ptr);
    }
	return (unsigned char*)queue->enqueueMapBuffer(*ptr, CL_TRUE, CL_MAP_READ, 0, size, NULL, NULL, NULL);
}

unsigned char * opencl::hostWriteBuffer(unsigned int size, bool store) {
	cl::Buffer * ptr = new cl::Buffer(*context, (CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR), size, NULL, NULL);
    if (store) {
        buffers.push_back(ptr);
    }
	return (unsigned char*)queue->enqueueMapBuffer(*ptr, CL_TRUE, CL_MAP_WRITE, 0, size, NULL, NULL, NULL);
}

cl::Image3D * opencl::cubeImage(unsigned int width, unsigned int height, unsigned depth, cl_mem_flags mem, bool store) {
    cl::Image3D * ptr = new cl::Image3D(*context, mem, formatFloat, width, height, depth);
    if (store) {
        buffers.push_back(ptr);
    }
    return ptr;
}

cl::Image2D * opencl::floatImage(unsigned int width, unsigned int height, cl_mem_flags mem, bool store) {
	cl::Image2D * ptr = new cl::Image2D(*context, mem, formatFloat, width, height);
    if (store) {
        buffers.push_back(ptr);
    }
	return ptr;
}

cl::Image2D * opencl::halfImage(unsigned int width, unsigned int height, cl_mem_flags mem, bool store) {
	cl::Image2D * ptr = new cl::Image2D(*context, mem, formatHalf, width, height);
    if (store) {
        buffers.push_back(ptr);
    }
    return ptr;
}

cl::Image2D * opencl::greyImage(unsigned int width, unsigned int height, cl_mem_flags mem, bool store) {
	cl::Image2D * ptr = new cl::Image2D(*context, mem, formatGrey, width, height);
    if (store) {
        buffers.push_back(ptr);
    }
    return ptr;
}

cl::Image2D * opencl::redGreenImage(unsigned int width, unsigned int height, cl_mem_flags mem, bool store) {
    cl::Image2D * ptr = new cl::Image2D(*context, mem, formatRedGreen, width, height);
    if (store) {
        buffers.push_back(ptr);
    }
    return ptr;
}

cl::Image2D * opencl::intImage(unsigned int width, unsigned int height, cl_mem_flags mem, bool store) {
	cl::Image2D * ptr = new cl::Image2D(*context, mem, formatInt, width, height);
    if (store) {
        buffers.push_back(ptr);
    }
    return ptr;
}

cl::Image2D * opencl::intBGRAImage(unsigned int width, unsigned int height, cl_mem_flags mem, bool store) {
	cl::Image2D * ptr = new cl::Image2D(*context, mem, formatBGRAInt, width, height);
    if (store) {
        buffers.push_back(ptr);
    }
    return ptr;
}

cl::Image2D * opencl::intNotNormalisedImage(unsigned int width, unsigned int height, cl_mem_flags mem, bool store) {
	cl::Image2D * ptr = new cl::Image2D(*context, mem, formatIntNotNormalised, width, height);
    if (store) {
        buffers.push_back(ptr);
    }
    return ptr;
}

cl::Image2D * opencl::int16bitImage(unsigned int width, unsigned int height, cl_mem_flags mem, bool store) {
	cl::Image2D * ptr = new cl::Image2D(*context, mem, format16bitInt, width, height);
    if (store) {
        buffers.push_back(ptr);
    }
    return ptr;
}

cl::Buffer * opencl::buffer(unsigned int size, cl_mem_flags mem, bool store) {
	cl::Buffer * ptr = new cl::Buffer(*context, mem, size, NULL, NULL);
    if (store) {
        buffers.push_back(ptr);
    }
    return ptr;
}

cl::ImageGL * opencl::glImage(GLuint textureID, cl_mem_flags mem, GLenum target, bool store) {
	cl_int err = 0;
	cl::ImageGL * ptr = new cl::ImageGL(*context, mem, target, 0, textureID, &err);
    if (store) {
        buffers.push_back(ptr);
    }
    return ptr;
}

cl::Kernel * opencl::getKernel(const char * kernel, bool store) {
	cl::Kernel * ptr = new cl::Kernel(*program, kernel, NULL);
    if (store) {
        kernels.push_back(ptr);
    }
    return ptr;
}

cl::CommandQueue * opencl::getQueue() {
	return queue;
}

void opencl::deleteCL(void * ptr) {
    for (auto it = buffers.begin(); it != buffers.end(); ++it) {
        if ((cl::Memory *)ptr == *it) {
            delete (cl::Memory *)ptr;
            buffers.erase(it);
            break;
        }
    }
    
    for (auto it = kernels.begin(); it != kernels.end(); ++it) {
        if ((cl::Kernel *)ptr == *it) {
            delete (cl::Kernel *)ptr;
            kernels.erase(it);
            break;
        }
    }
}

void opencl::rekernel() {
    
    
    sources.clear();
    addKernels({});
    //queue->_josh_lock_queue();
    try {
        putLog("--- !!! Remaking program");
        makeProgram();
    } catch (cl::Error& e) {
        std::stringstream strm;
        strm << "OpenCL Exception: " << e.err() << " : " << e.what();
        putLog(strm.str());
    } catch (std::exception& e) {
        putLog("Unknown exception: ");
        putLog(e.what());
    }
    //queue->_josh_unlock_queue();
}

cl_context opencl::get_cl_context() const {
    return (*context)();
}

cl_device_id opencl::get_cl_device() const {
    return devices[0]();
}
