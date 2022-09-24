#include "getDiscontinuities.hpp"
#include <Mush Core/opencl.hpp>
#include <Mush Core/imageProcess.hpp>

getDiscontinuities::getDiscontinuities() : mush::integerMapProcess(1) {

}

getDiscontinuities::~getDiscontinuities() {

}

void getDiscontinuities::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {

	assert(buffers.size() == 2);

	discontinuities = context->getKernel("discontinuities");
	copyImage = context->getKernel("copyImage");

	motionVectors = castToImage(buffers.begin()[0]);

	depth = castToImage(buffers.begin()[1]);

	depth->getParams(_width, _height, _size);

	addItem((unsigned char *)context->buffer(_width*_height * sizeof(cl_uchar)));

	discontinuities->setArg(0, _getMem(0).get_buffer());

	create_temp_image(context);

	queue = context->getQueue();
}

void getDiscontinuities::process() {
	inLock();
	cl::Event event;

	auto mot = motionVectors->outLock();
	if (mot == nullptr) {
		release();
		return;
	}

	auto inDepth = depth->outLock();
	if (inDepth == nullptr) {
		release();
		return;
	}

	discontinuities->setArg(1, mot.get_image());
	discontinuities->setArg(2, inDepth.get_image());
	//        discontinuities->setArg(3, *in);

	discontinuities->setArg(3, *temp_image);

	queue->enqueueNDRangeKernel(*discontinuities, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
	event.wait();

	//inUnlock();
	inUnlock(); // custom image shows discontinuities, rather than b/w integer map

	motionVectors->outUnlock();

	depth->outUnlock();

}

void getDiscontinuities::inUnlock() {
	if (getTagInGuiMember() && tagGui != nullptr) {
		mush::buffer temp{ temp_image };
		if (_getMem(next).has_camera_position()) {
			temp.set_camera_position(_getMem(next).get_camera_position(), _getMem(next).get_theta_phi_fov());
		}
		tagGui->copyImageIntoGuiBuffer(getTagInGuiIndex(), temp);
	}
	ringBuffer::inUnlock();
}
