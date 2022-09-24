//
//  imageBuffer.cpp
//  video-mush
//
//  Created by Josh McNamee on 19/08/2014.
//
//

#include "imageBuffer.hpp"
#include "gui.hpp"
#include "opencl.hpp"
#include "tagInGui.hpp"

using namespace mush;

void imageBuffer::inUnlock() {
    if (getTagInGuiMember() && tagGui != nullptr) {
        tagGui->copyImageIntoGuiBuffer(getTagInGuiIndex(), _getMem(next));
    }
    ringBuffer::inUnlock();
}


