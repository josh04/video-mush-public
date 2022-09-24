//
//  integerMapBuffer.hpp
//  video-mush
//
//  Created by Josh McNamee on 21/08/2014.
//
//

#ifndef video_mush_integerMapBuffer_hpp
#define video_mush_integerMapBuffer_hpp

#include "imageBuffer.hpp"
#include "mush-core-dll.hpp"

namespace cl {
    class Kernel;
    class Image2D;
    class Buffer;
}

namespace mush {
	class opencl;
	class MUSHEXPORTS_API integerMapBuffer : public guiAccessible {
    public:
        integerMapBuffer(int bitDepth = 8) : guiAccessible(), _bitDepth(bitDepth) {
            
        }
        
        ~integerMapBuffer() {
            
        }
        
		virtual void inUnlock();
        
		virtual void guiTag(int i, tagInGui * tag);
        
		void create_temp_image(std::shared_ptr<opencl> context);

        int getBitDepth() {
            return _bitDepth;
        }
        
        int getMapSize() {
            if (_bitDepth > 8) {
                return _width * _height * sizeof(uint16_t);
            } else {
                return _width * _height * sizeof(uint8_t);
            }
        }
        
    protected:
        int _bitDepth = 8;
        cl::Image2D * temp_image = nullptr;
        cl::Kernel * intBufferToImage = nullptr;
    private:
        //std::vector<cl::Buffer *> mem;

		std::mutex _lock;
    };

}

#endif
