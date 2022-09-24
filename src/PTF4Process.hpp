#ifndef MUSH_GAMMA8PROCESS_HPP
#define MUSH_GAMMA8PROCESS_HPP

namespace mush {
class PTF4Process : public mush::imageProcess {
public:
	PTF4Process(int yuvMax) : mush::imageProcess(), _yuvMax(yuvMax) {
		
	}
	
	~PTF4Process() {}
	
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        
		this->context = context;
        
		PTF4 = context->getKernel("encodePTF4");
		
		inputBuffer = castToImage(buffers.begin()[0]);
		inputBuffer->getParams(_width, _height, _size);
		
		addItem(context->floatImage(_width, _height));
		queue = context->getQueue();
		
        PTF4->setArg(1, _getImageMem(0));
        PTF4->setArg(2, _yuvMax);
	}
	
	void process() {
		inLock();
		auto ptr = inputBuffer->outLock();
		
		if (ptr == nullptr) {
			return;
		}
		
		PTF4->setArg(0, ptr.get_image());
		
		cl::Event event;
		queue->enqueueNDRangeKernel(*PTF4, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
		event.wait();
		
		inputBuffer->outUnlock();
		inUnlock();
	}
	
private:
	std::shared_ptr<mush::opencl> context = nullptr;
	cl::CommandQueue * queue = nullptr;
	cl::Kernel * PTF4 = nullptr;
    
    const float _yuvMax = 1000.0f;
	
	std::shared_ptr<mush::imageBuffer> inputBuffer = nullptr;
};
}

#endif
