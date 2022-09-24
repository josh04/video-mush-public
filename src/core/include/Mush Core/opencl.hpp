#ifndef HDRCONTEXT_HPP
#define HDRCONTEXT_HPP

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define __CL_ENABLE_EXCEPTIONS

#ifdef _WIN32
#include <windows.h>
#include <GL/glew.h>
#endif

#include <vector>
#include <fstream>
#include <iostream>
#include <memory>
#ifndef _MUSH_OPENCL_FRAMEWORK

#include "cl-1.2.8.hpp"
#endif
#include "mush-core-dll.hpp"

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#endif


namespace mush {
	class MUSHEXPORTS_API opencl {
        public:
			opencl(const char * resourceDir, const bool openclCPU);

			~opencl();
        
			void init(bool withGL = false);
			void init(cl::Context * ctx);
        
			static int getPlatformId(std::vector<cl::Platform> platforms, bool openclCPU);
        
			static std::vector<cl_context_properties> buildProps(bool openclCPU = false);
			static std::vector<cl_context_properties> buildPropsWithGL(bool openclCPU = false);
            

            void addKernels(std::vector<std::string> kernels);
            void makeProgram();

			unsigned char * hostReadBuffer(unsigned int size, bool store = true);
			unsigned char * hostWriteBuffer(unsigned int size, bool store = true);
        
			cl::Image3D * cubeImage(unsigned int width, unsigned int height, unsigned depth, cl_mem_flags mem = CL_MEM_READ_WRITE, bool store = true);
        
			cl::Image2D * floatImage(unsigned int width, unsigned int height, cl_mem_flags mem = CL_MEM_READ_WRITE, bool store = true);
			cl::Image2D * halfImage(unsigned int width, unsigned int height, cl_mem_flags mem = CL_MEM_READ_WRITE, bool store = true);
        cl::Image2D * greyImage(unsigned int width, unsigned int height, cl_mem_flags mem = CL_MEM_READ_WRITE, bool store = true);
        cl::Image2D * redGreenImage(unsigned int width, unsigned int height, cl_mem_flags mem = CL_MEM_READ_WRITE, bool store = true);
			cl::Image2D * intImage(unsigned int width, unsigned int height, cl_mem_flags mem = CL_MEM_READ_WRITE, bool store = true);
			cl::Image2D * intBGRAImage(unsigned int width, unsigned int height, cl_mem_flags mem = CL_MEM_READ_WRITE, bool store = true);
			cl::Image2D * intNotNormalisedImage(unsigned int width, unsigned int height, cl_mem_flags mem = CL_MEM_READ_WRITE, bool store = true);
			cl::Image2D * int16bitImage(unsigned int width, unsigned int height, cl_mem_flags mem = CL_MEM_READ_WRITE, bool store = true);
        
			cl::Buffer * buffer(unsigned int size, cl_mem_flags mem = CL_MEM_READ_WRITE, bool store = true);

			cl::ImageGL * glImage(GLuint textureID, cl_mem_flags mem = CL_MEM_READ_WRITE, GLenum target = GL_TEXTURE_2D, bool store = true);

			cl::Kernel * getKernel(const char * kernel, bool store = true);

			cl::CommandQueue * getQueue();
        
            void deleteCL(void *);
        
            void rekernel();
        
        cl_context get_cl_context() const;
        cl_device_id get_cl_device() const;
        
        private:
            void initCommon();
        
            void addKernel(std::string kernel);

            const cl::ImageFormat formatFloat;
            const cl::ImageFormat formatHalf;
        const cl::ImageFormat formatGrey;
        const cl::ImageFormat formatRedGreen;
            const cl::ImageFormat formatInt;
            const cl::ImageFormat formatBGRAInt;
            const cl::ImageFormat formatIntNotNormalised;
            const cl::ImageFormat format16bitInt;

            cl::Context * context = nullptr;
            cl::Program * program = nullptr;
            cl::CommandQueue * queue = nullptr;

            std::vector<std::string> sources;

            std::vector<cl::Memory *> buffers;
            std::vector<cl::Kernel *> kernels;

            std::vector<cl::Device> devices;
        
            const char * resourceDir = nullptr;
			bool openclCPU = false;
    };
    
}
#endif
