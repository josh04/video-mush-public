//
//  sandProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_sandProcessor_hpp
#define video_mush_sandProcessor_hpp

#include "exports.hpp"
#include <Mush Core/quitEventHandler.hpp>
#include "nullProcess.hpp"
#include <Mush Core/frameStepper.hpp>
#include "sandProcess.hpp"
#include "textDrawProcess.hpp"
#include <Mush Core/sphereMapProcess.hpp>
#include <Mush Core/imageProcessor.hpp>
#include <thread>
#include <vector>

namespace mush {
class sandProcessor : public mush::imageProcessor {
public:
    sandProcessor(config::generatorProcessStruct g, unsigned int width, unsigned int height, const char * resource_dir, parConfigStruct par_config, mush::config config);
    
    ~sandProcessor();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;
    
    std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override;
    
    void go() override;
    
    void destroy();
    
private:
    bool running = true;
    
    std::shared_ptr <mush::imageProcess> sand = nullptr;
    
    std::shared_ptr <mush::imageProcess> motionReproj = nullptr;
    std::shared_ptr <mush::imageProcess> motionCopy = nullptr;
    
    std::shared_ptr <mush::imageProcess> unpackRight = nullptr;
	std::shared_ptr <mush::imageProcess> mse = nullptr;
   
    std::vector<std::shared_ptr<mush::frameStepper>> steppers;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
    std::vector<std::shared_ptr<mush::ringBuffer>> _inputs;
    
    unsigned int _width = 1280, _height = 720;
    
	std::shared_ptr<mush::quitEventHandler> quitter = nullptr;

	std::shared_ptr<mush::imageProcess> _delay = nullptr;

    const char * _resource_dir;
    config::generatorProcessStruct _g;
    
    parConfigStruct _par_config;
	mush::config _config;
};
}

#endif
