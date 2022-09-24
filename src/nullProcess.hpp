//
//  nullProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 30/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_nullProcess_hpp
#define media_encoder_nullProcess_hpp

#include <Mush Core/imageProcess.hpp>

namespace mush {
    class nullProcess : public mush::processNode {
    public:
        nullProcess() : mush::processNode() {
            
        }
        
        ~nullProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> buffer) {
            init(context, {buffer});
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            this->buffers = buffers;
        }
        
        void reInit(std::shared_ptr<mush::ringBuffer> buffer) {
            std::vector<std::shared_ptr<mush::ringBuffer>> temp;
            temp.push_back(buffer);
            reInit(temp);
        }
        
        void reInit(std::vector<std::shared_ptr<mush::ringBuffer>> buffers) {
            this->buffers = buffers;
        }
        
        void process() {
            for (int i = 0; i < buffers.size(); ++i) {
                buffers[i]->outLock();
                buffers[i]->outUnlock();
            }
        }
        
        void release() {
            buffers.clear();
        }
        
    private:
        std::vector<std::shared_ptr<mush::ringBuffer>> buffers;
    };
}

#endif
