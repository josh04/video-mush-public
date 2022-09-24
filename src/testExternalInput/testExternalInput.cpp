// testExternalInput.cpp
//

#include <sstream>
#include <thread>
#include "exports.hpp"
#include "imageProcessor.hpp"
#include "imageProcess.hpp"
#include "opencl.hpp"

/*
This class takes an image from main memory (hostMemory) and pushes it up to OpenCL.
The cl::Image2D * used can be accessed with outLock() and outUnlock()
*/
class memoryToGpu : public mush::imageProcess {
public:
	memoryToGpu() : mush::imageProcess() {

	}

	~memoryToGpu() {}

	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		assert(buffers.size() == 0);
		this->context = context;
		_width = 1920;
		_height = 1080;
		hostMemory = (float *)context->hostWriteBuffer(_width*_height*sizeof(float) * 4);
		addItem(context->floatImage(_width, _height));
		queue = context->getQueue();
	}

	void process() {
		inLock();

		cl::Event event;
		cl::size_t<3> origin, region;
		origin[0] = 0; origin[1] = 0; origin[2] = 0;
		region[0] = _width; region[1] = _height; region[2] = 1;

		queue->enqueueWriteImage(*(cl::Image2D *)_getMem(0), CL_TRUE, origin, region, 0, 0, hostMemory, NULL, &event);
		event.wait();

		inUnlock();
	}

private:
	std::shared_ptr<mush::opencl> context = nullptr;
	cl::CommandQueue * queue = nullptr;
	float * hostMemory = nullptr;

	std::shared_ptr<mush::imageBuffer> inputBuffer = nullptr;

};

/*
This class takes an image from OpenCL and brings it down to main memory.
The main memory pointer can be accessed with outLock() and outUnlock()
*/
class gpuToMemory : public mush::imageProcess {
public:
	gpuToMemory() : mush::imageProcess() {

	}

	~gpuToMemory() {}

	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		assert(buffers.size() == 2);
		this->context = context;

		inputBuffer = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[0]);
		inputBuffer->getParams(_width, _height, _size);

		addItem(context->hostReadBuffer(_width*_height*sizeof(float)*4));
		queue = context->getQueue();
	}

	void process() {
		inLock();
		cl::Image2D * ptr = (cl::Image2D *)inputBuffer->outLock();

		if (ptr == nullptr) {
			return;
		}

		cl::Event event;
		cl::size_t<3> origin, region;
		origin[0] = 0; origin[1] = 0; origin[2] = 0;
		region[0] = _width; region[1] = _height; region[2] = 1;

		queue->enqueueReadImage(*ptr, CL_TRUE, origin, region, 0, 0, _getMem(0), NULL, &event);
		event.wait();

		inputBuffer->outUnlock();
		inUnlock();
	}

private:
	std::shared_ptr<mush::opencl> context = nullptr;
	cl::CommandQueue * queue = nullptr;

	std::shared_ptr<mush::imageBuffer> inputBuffer = nullptr;
};

/*
This class builds the processing flow. Two input buffers arrived, and
are both assigned a process which brings them down to main memory. As
these classes aren't use for anything in this example outLock() and 
outUnlock() is run on them to simulate flow.

A seperate class which uploads a blank image to OpenCL is also created
and tagged for output.

All three classes are tagged for display.

*/
class testProcessor : public mush::imageProcessor {
public:
	testProcessor() : mush::imageProcessor() {
	
	}

	~testProcessor() {

	}

	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		assert(buffers.size() == 2);
		this->context = context;


		fisheyeInput = mush::castToImage(buffers.begin()[0]);
		backplateInput = mush::castToImage(buffers.begin()[1]);

		fisheyeProcess = std::make_shared<gpuToMemory>();
		fisheyeProcess->init(context, fisheyeInput);
		backplateProcess = std::make_shared<gpuToMemory>();
		backplateProcess->init(context, backplateInput);

		output = std::make_shared<memoryToGpu>();
		output->init(context, nullptr);

		_guiBuffers.push_back(output);
		_guiBuffers.push_back(fisheyeInput);
		_guiBuffers.push_back(backplateInput);
	}

	void go() {
		while (fisheyeInput->good()) {
			process();
		}
		fisheyeProcess->release();
		backplateProcess->release();
		output->release();
	}

	void process() {
		if (fisheyeInput->good()) {
			fisheyeProcess->process();
			fisheyeProcess->outLock();
			fisheyeProcess->outUnlock();
		}
		if (backplateProcess->good()) {
			backplateProcess->process();
			backplateProcess->outLock();
			backplateProcess->outUnlock();
		}
		
		output->process();
	}

	virtual const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const {
		return { output };
	}

	virtual std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() {
		return _guiBuffers;
	}

private:
	std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;

	std::shared_ptr<mush::opencl> context = nullptr;

	std::shared_ptr<mush::imageBuffer> fisheyeInput = nullptr;
	std::shared_ptr<mush::imageBuffer> backplateInput = nullptr;

	std::shared_ptr<mush::imageProcess> fisheyeProcess = nullptr, backplateProcess = nullptr, output = nullptr;
};

void doLog(std::atomic<bool> * quit) {
	while (!(*quit)) {
		char msg[4096];
		if (getLog(msg, 4096)) {
			std::cout << msg << std::endl;
		}
	}

	char msg[4096];
	while (getLog(msg, 4096) != false) {
		std::cout << msg << std::endl;
	}
	endLog();
}

cl::Context makeContext() {
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

#if defined(_WIN32)
	HGLRC glCtx = wglGetCurrentContext();
	HDC glHDC2 = wglGetCurrentDC();
#elif defined(__linux__)
	GLXContext glCtx = glXGetCurrentContext();
#elif defined(__APPLE__)
	CGLContextObj cgl_context = CGLGetCurrentContext();
	CGLShareGroupObj sharegroup = CGLGetShareGroup(cgl_context);
	gcl_gl_set_sharegroup(sharegroup);

	CGLError err;
	err = CGLEnable(cgl_context, kCGLCEMPEngine);

	if (err != kCGLNoError) {
		abort();
	}
#endif

	putLog("--- Selecting Platform");
	int platform_id = -1;
	for (int i = 0; i < platforms.size(); ++i) {
		std::vector<cl::Device> devices;
		try {
			platforms[i].getDevices(CL_DEVICE_TYPE_GPU, &devices);
		} catch (cl::Error e) {

		}
		if (devices.size() > 0) {
			platform_id = i;
		}
	}

	std::stringstream strm;
	strm << "--- Selected Platform: " << platform_id;
	putLog(strm.str());

	if (platform_id == -1) {
		throw std::runtime_error("Failed to find platform with GPU.");
	}

	putLog("--- Init CL");
	cl_context_properties props[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[platform_id])(),
#if defined(_WIN32)
		CL_GL_CONTEXT_KHR, (cl_context_properties)glCtx,
		CL_WGL_HDC_KHR, (cl_context_properties)glHDC2,
#elif defined(__linux__)
		CL_GL_CONTEXT_KHR, (cl_context_properties)glCtx,
		CL_GLX_DISPLAY_KHR, (intptr_t)glXGetCurrentDisplay(),
#elif defined(__APPLE__)
		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)sharegroup,
		//CL_CGL_SHAREGROUP_KHR, (cl_context_properties)sharegroup,
#endif
		0
	};

	return cl::Context(CL_DEVICE_TYPE_GPU, props);
}

/*
The main function sets up a small config struct and calls a
function in the DLL which sets up our two inputs.
*/
int main(int argc, char * argv[])
{
	// initialise config struct
	mush::config config;
	config.defaults();
	
	// vlc output
	config.outputConfig.encodeEngine = mush::encodeEngine::none;
	config.outputEngine = mush::outputEngine::noOutput;

	config.show_gui = true;
	// Sim2 preview window
	config.sim2preview = false;

	config.resourceDir = ".\\";
	config.inputConfig.resourceDir = ".\\resources\\";


	std::atomic<bool> quit;
	quit = false;
	std::shared_ptr<std::thread> logThread = nullptr;
	
	try {
		logThread = std::make_shared<std::thread>(&doLog, &quit);
		//videoMushInit(&config, true);
		//cl::Context ctx = makeContext();

		testProcessor hg = testProcessor();
		videoMushPresetAMD(&config, &hg);
//		videoMushDestroy();

	} catch (std::exception e) {
		std::cout << e.what() << std::endl;
		std::cout << "Exception thrown." << std::endl;
	}

	quit = true;
	if (logThread->joinable()) {
		logThread->join();
	}
	logThread = nullptr;
	// done!
	return 0;
}

