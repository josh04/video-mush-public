//
//  imageProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 03/05/2015.
//
//

#include "imageProcess.hpp"

namespace mush {

MUSHEXPORTS_API std::shared_ptr<mush::imageBuffer> castToImage(const std::shared_ptr<mush::ringBuffer> in) {
    auto out = std::dynamic_pointer_cast<mush::imageBuffer>(in);
    if (out == nullptr) {
        throw std::runtime_error("Error: A non-image buffer was passed to an image process.");
    }
    return out;
}

}
