//
//  factoryProcessor.hpp
//  factory
//
//  Created by Josh McNamee on 24/03/2015.
//
//

#ifndef factory_factoryProcessor_hpp
#define factory_factoryProcessor_hpp

#include <Mush Core/imageProcessor.hpp>
#include <Mush Core/opencl.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Video Mush/quitEventHandler.hpp>

#include "gpuToMemory.hpp"
#include "memoryToGpu.hpp"
#include "factoryProcess.hpp"
#include "freeImageLoader.hpp"

#include "factoryHexGrid.hpp"

namespace factory {
class processor : public mush::imageProcessor, azure::Eventable {
public:
    processor(const char * localResourceDir) : mush::imageProcessor(), _localResourceDir(localResourceDir) {
        
    }
    
    ~processor() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) {
        this->context = context;
        quitHandler = std::make_shared<mush::quitEventHandler>();
        videoMushAddEventHandler(quitHandler);
        
        _background = std::make_shared<freeImageLoader>();
        std::string backgroundPath = std::string(_localResourceDir)+std::string("/background-1.png");
        _background->init(context, backgroundPath.c_str());
        
        output = std::make_shared<factoryProcess>();
        output->init(context, {});
        
        
        hex = std::make_shared<factoryHexGrid>(1280, 720);
        hex->init(context, {});
        
        _guiBuffers.push_back(_background);
        _guiBuffers.push_back(output);
        _guiBuffers.push_back(hex);
    }
    
    void go() {
        
        std::thread hexThread(&factoryHexGrid::startThread, hex);
        
        while (!quitHandler->getQuit()) {
            process();
        }
        output->release();
        hex->release();
        hexThread.join();
    }
    
    void process() {
        _background->inUnlock();
        output->process();
    }
    
    virtual const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const {
        return {output};
    }
    
    virtual std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() {
        return _guiBuffers;
    }
    
    bool event(std::shared_ptr<azure::Event> event) {
        return false;
    }
    
private:
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
    
    std::shared_ptr<mush::opencl> context = nullptr;
    
    std::shared_ptr<mush::imageProcess> output = nullptr;
    std::shared_ptr<factoryHexGrid> hex = nullptr;
    
    bool quit = false;
    
    std::shared_ptr<mush::quitEventHandler> quitHandler = nullptr;
    
    std::shared_ptr<freeImageLoader> _background = nullptr;
        
    const char * _localResourceDir;
};
}

#endif
