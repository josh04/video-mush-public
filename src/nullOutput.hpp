//
//  nullOutput.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_nullOutput_hpp
#define media_encoder_nullOutput_hpp

#include <Mush Core/encoderEngine.hpp>
#include <Mush Core/outputEngine.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/SetThreadName.hpp>

class nullOutput : public outputEngine {
public:
    nullOutput() : outputEngine() {
        
    }
    
    ~nullOutput() {
        
    }
    
    virtual void init(std::vector<std::shared_ptr<encoderEngine>> encoderEngines, mush::core::outputConfigStruct config) {
        for (auto eng : encoderEngines) {
            _encoders.push_back(mush::registerContainer<encoderEngine>(eng));
        }
    }
    
    virtual void gather() {
        
        std::vector<bool> runnings;
        for (auto encoder : _encoders) {
            runnings.push_back(true);
        }
        
        for (int j = 0; j < _encoders.size(); ++j) {
            threads.push_back(std::thread(&nullOutput::checkCode, _encoders[j].get()));
        }
        
        for (int j = 0; j < threads.size(); ++j) {
            if (threads[j].joinable()) {
                threads[j].join();
            }
        }
        /*
        while (running(runnings)) {
            for (int j = 0; j < _encoders.size(); ++j) {
                if (!_encoders[j]->good()) {
                    runnings[j] = false;
                } else {
                    unsigned char * enc = (unsigned char *)_encoders[j]->outLock(30); // 30 nanosecond
                    if (enc != nullptr) {
                        _encoders[j]->outUnlock();
                    }
                }
            }
        }*/

    }
private:
    static void checkCode(std::shared_ptr<encoderEngine> encoder) {
		SetThreadName("Null output check code");
        bool end = false;
        while (!end) {
            if (!encoder->good()) {
                end = true;
            } else {
				auto buf = encoder->outLock(); // 30 nanosecond
                if (buf != nullptr) {
                    encoder->outUnlock();
                }
            }
        }
    };
    
    bool running(const std::vector<bool> &runnings) {
        bool ret = false;
        for (auto run : runnings) {
            ret = ret || run;
        }
        return ret;
    }
    
    std::vector<std::thread> threads;
    
    std::vector<mush::registerContainer<encoderEngine>> _encoders;
    
};

#endif
