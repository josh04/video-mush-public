//
//  process.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_process_hpp
#define media_encoder_process_hpp

namespace mush {

    class processNode {
    public:
        processNode() {
            
        }
        
        ~processNode() {
            
        }
        
        virtual void process() = 0;
    private:
    };

}

#endif
