//
//  integerMapBuffer.cpp
//  video-mush
//
//  Created by Josh McNamee on 23/09/2015.
//
//
#include <mutex>

#include "opencl.hpp"
#include "tagInGui.hpp"
#include "integerMapBuffer.hpp"

namespace mush {
    void integerMapBuffer::inUnlock() {
        if (getTagInGuiMember() && tagGui != nullptr) {
            
            intBufferToImage->setArg(0, _getMem(next).get_buffer());
            cl::Event event;
            queue->enqueueNDRangeKernel(*intBufferToImage, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();

			mush::buffer temp{ *temp_image };
			if (_getMem(next).has_camera_position()) {
				temp.set_camera_position(_getMem(next).get_camera_position(), _getMem(next).get_theta_phi_fov());
			}

            tagGui->copyImageIntoGuiBuffer(getTagInGuiIndex(), temp);
        }
        ringBuffer::inUnlock();
    }

    void integerMapBuffer::guiTag(int i, tagInGui * tag) {
        if (tag != nullptr) {
            if (temp_image == nullptr) {
				create_temp_image(tag->getContext());
            }
        }
        guiAccessible::guiTag(i, tag);
    }

	void integerMapBuffer::create_temp_image(std::shared_ptr<opencl> context) {
		std::lock_guard<std::mutex> lock(_lock);
		if (temp_image == nullptr) {
			queue = context->getQueue();
			temp_image = context->floatImage(_width, _height);
			if (_bitDepth > 8) {
				intBufferToImage = context->getKernel("shortBufferToImage");
			}
			else {
				intBufferToImage = context->getKernel("charBufferToImage");
			}
			intBufferToImage->setArg(1, *temp_image);
			intBufferToImage->setArg(2, _width);
			intBufferToImage->setArg(3, _bitDepth);
		}
	}

}
