//
//  frameStepper.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_frameStepper_hpp
#define video_mush_frameStepper_hpp

#include <atomic>
#include <mutex>
#include <condition_variable>

#include "processNode.hpp"
#include "mush-core-dll.hpp"

namespace mush {
    class MUSHEXPORTS_API frameStepper : public mush::processNode {
    public:
		frameStepper();
		~frameStepper();
        
		void process();
		void throw_switch();
		void toggle();
		void release();
        
    protected:
        std::atomic<bool> enabled;
    private:
        std::mutex mut;
        std::condition_variable cond;
        std::atomic<bool> sw;
    };
}

#endif
