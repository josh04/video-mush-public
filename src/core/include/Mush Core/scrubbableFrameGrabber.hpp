//
//  scrubbableFrameGrabber.hpp
//  video-mush
//
//  Created by Josh McNamee on 01/04/2016.
//
//

#ifndef scrubbableFrameGrabber_h
#define scrubbableFrameGrabber_h

#include "frameGrabber.hpp"

namespace mush {
    class scrubbableFrameGrabber : public mush::frameGrabber {
    public:
        scrubbableFrameGrabber(const mush::inputEngine inputEngine) : mush::frameGrabber(inputEngine) {
            
        }
        
        ~scrubbableFrameGrabber() {
            
        }
        
        virtual void moveToFrame(int frame) = 0;
        
        virtual int getFrameCount() = 0;
        virtual int getCurrentFrame() = 0;
        
    protected:
    };
    
}

#endif /* scrubbableFrameGrabber_h */
