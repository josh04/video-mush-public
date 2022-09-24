//
//  factoryHexGrid.hpp
//  factory
//
//  Created by Josh McNamee on 12/07/2015.
//
//

#ifndef factory_factoryHexGrid_hpp
#define factory_factoryHexGrid_hpp

#include <Mush Core/opencl.hpp>
#include <azure/Framebuffer.hpp>
#include <azure/Program.hpp>

#include "hexGrid.hpp"

class factoryHexGrid : public mush::imageProcess {
public:
    factoryHexGrid(unsigned int width, unsigned int height) : mush::imageProcess() {
        _width = width;
        _height = height;
    }
    
    ~factoryHexGrid() {}
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) {

        assert(buffers.size() == 0);
        queue = context->getQueue();
        
        parFlip = context->getKernel("copyImage");
        
        hex = std::make_shared<hexGrid>(_width, _height);
        hex->init();
        
#if defined(_WIN32)
        glCtx = wglGetCurrentContext();
        glHDC2 = wglGetCurrentDC();
        
        glCtx4 = wglCreateContext(glHDC2);
        _glException();
        
        if (wglShareLists(glCtx, glCtx4)) {
            putLog("Shared GL Textures.");
        } else {
            putLog("Failed to share GL Textures.");
        }
        _glException();
        
        bool test = wglMakeCurrent(glHDC2, glCtx4);
        
#elif defined(__linux__)
        glCtx = glXGetCurrentContext();
#elif defined(__APPLE__)
        cgl_context = CGLGetCurrentContext();
        auto pix_fmt = CGLGetPixelFormat(cgl_context);
        
        /*CGLPixelFormatAttribute attributes[4] = {
         kCGLPFAAccelerated,   // no software rendering
         kCGLPFAOpenGLProfile, // core profile with the version stated below
         (CGLPixelFormatAttribute) kCGLOGLPVersion_Legacy,
         (CGLPixelFormatAttribute) 0
         };
         
         CGLPixelFormatObj pix_fmt;
         GLint npix;
         CGLChoosePixelFormat(attributes, &pix_fmt, &npix);*/
        
        if (auto err = CGLCreateContext(pix_fmt, cgl_context, &cgl_context_new)) {
            putLog(CGLErrorString(err));
            throw std::runtime_error("Failed to share GL Textures.");
        } else {
            putLog("Shared GL Textures.");
        }
        _glException();
        CGLSetCurrentContext(cgl_context_new);
#endif
        //GLint i, j;
        //glGetIntegerv(GL_MAJOR_VERSION, &i);
        //glGetIntegerv(GL_MINOR_VERSION, &j);
        
        //std::stringstream strm;
        //strm << "GL Version " << i <<"." << j;
        //putLog(strm.str());
        
        _frame = std::make_shared<azure::Framebuffer>();
        _frame->setTarget(GL_TEXTURE_RECTANGLE);
        _frame->createTexture(GL_RGBA, _width, _height, GL_RGBA, GL_FLOAT, NULL);
        _frame->attach();
        _frame->clear();
        
        _frame->bind();
        _frame->attach();
        
        GLuint _depthTex; // Store and delete
        glGenTextures(1, &_depthTex);
        _glException();
        glBindTexture(GL_TEXTURE_2D, _depthTex);
        _glException();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        _glException();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        _glException();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        _glException();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        _glException();
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        //_glException();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        _glException();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, _width, _height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
        _glException();
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTex, 0);
        _glException();
        glBindTexture(GL_TEXTURE_2D, 0);
        _glException();
        
        glEnable(GL_DEPTH_TEST);
        _glException();
        glDepthMask(GL_TRUE);
        _glException();
        glDepthFunc(GL_LEQUAL);
        _glException();
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        _glException();
        
        _frame->unbind();
        
        GLuint texID = _frame->getTexture();
        _frameCL = context->glImage(texID, CL_MEM_READ_WRITE, GL_TEXTURE_RECTANGLE);
        
        std::vector<cl::Memory> glObjects;
        glObjects.push_back(*_frameCL);
        cl::Event event;
        
        _glException();
        
#if defined(_WIN32)
        test = wglMakeCurrent(glHDC2, glCtx);
#else if defined(__APPLE__)
        CGLSetCurrentContext(cgl_context);
#endif
        glFlush();
        queue->enqueueAcquireGLObjects(&glObjects, NULL, &event);
        event.wait();
        glFlush();
        _glException();
        addItem(context->floatImage(_width, _height));
        
        parFlip->setArg(0, *_frameCL);
        parFlip->setArg(1, *_getImageMem(0));

        
    }
    
    void process() {
        
        static std::once_flag initFlag;
        std::call_once(initFlag, [&]() {
            
#if defined(_WIN32)
            bool test = wglMakeCurrent(glHDC2, glCtx4);
            auto word = GetLastError();
            
            if (!test) {
                std::stringstream strm2;
                strm2 << "Err: " << word;
                putLog(strm2.str());
            }
#else if defined(__APPLE__)
            auto err = CGLSetCurrentContext(cgl_context_new);
            if (err < 0) {
                throw std::runtime_error(CGLErrorString(err));
            }
#endif
            
            
        });
        
        inLock();
        
        std::vector<cl::Memory> glObjects;
        glObjects.push_back(*_frameCL);
        cl::Event event2;
        queue->enqueueReleaseGLObjects(&glObjects, NULL, &event2);
        event2.wait();
        
        glFinish();
        _glException();
        
        _frame->bind();
        
        hex->draw();
        _glException();
        
        glFinish();
        
        _frame->unbind();
        
        glFinish();
        
        cl::Event event3;
        queue->enqueueAcquireGLObjects(&glObjects, NULL, &event3);
        event3.wait();
        
        cl::Event event;
        queue->enqueueNDRangeKernel(*parFlip, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        inUnlock();

        
    }
    
    void startThread() {
        while(good()) {
            process();
            outLock();
            outUnlock();
        }
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    float * hostMemory = nullptr;
    
    cl::ImageGL * _frameCL = nullptr;
    std::shared_ptr<azure::Framebuffer> _frame = nullptr;
    std::shared_ptr<hexGrid> hex = nullptr;
    
    
    cl::Kernel * parFlip = nullptr;
#ifdef _WIN32
    HGLRC glCtx;
    HGLRC glCtx4;
    HDC glHDC2;
#endif
#ifdef __APPLE__
    CGLContextObj cgl_context;
    CGLContextObj cgl_context_new;
#endif
    
    GLuint depthbuffer = 0;
    float count = 0.0f;
    
};

#endif
