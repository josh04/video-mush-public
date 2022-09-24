#ifndef HDREXPORTS_HPP
#define HDREXPORTS_HPP

#include "dll.hpp"

#include <string>
#include <memory>
#include "ConfigStruct.hpp"

namespace azure {
    class Eventable;
}

namespace mush {
    class ringBuffer;
    class imageProcessor;
    class opencl;
    
}

namespace cl {
    class Context;
}

extern "C" {

	// important functions
	//---------------------------------------------------

	/* hdrRun
	 * main encoder entry point for GUI applications.
	 * passed a filled out config struct it will
	 * start whatever work it's been given.
	 */
    VIDEOMUSH_EXPORTS void videoMushRunAll(mush::config * config);
    
    VIDEOMUSH_EXPORTS void videoMushInit(mush::config * config, bool useOwnContext = false);

	VIDEOMUSH_EXPORTS void videoMushCreateProcessor(mush::config * config, std::initializer_list<std::shared_ptr<mush::ringBuffer>> list);
	VIDEOMUSH_EXPORTS void videoMushUseExistingProcessor(std::shared_ptr<mush::imageProcessor> processor, std::initializer_list<std::shared_ptr<mush::ringBuffer>> list);

	VIDEOMUSH_EXPORTS void videoMushExecute(int ar_size, bool ** use_outputs);

	VIDEOMUSH_EXPORTS void videoMushDestroy();
    
    VIDEOMUSH_EXPORTS void videoMushUpdateCLKernels();
    
    VIDEOMUSH_EXPORTS void videoMushAddEventHandler(std::shared_ptr<azure::Eventable> ev);
    
    VIDEOMUSH_EXPORTS void videoMushRemoveEventHandler(std::shared_ptr<azure::Eventable> ev);

	VIDEOMUSH_EXPORTS uint32_t videoMushGetOutputCount();
	VIDEOMUSH_EXPORTS bool videoMushGetOutputName(char * name, uint64_t size);
	VIDEOMUSH_EXPORTS void videoMushResetOutputNameCount();

	//---------------------------------------------------

	/* this is a bad thing
	 */

	VIDEOMUSH_EXPORTS unsigned int getStats(mush::config* Config,
		unsigned int raw_width, unsigned int raw_height,
		const char * inputfolder,
		unsigned int &width, unsigned int &height, mush::filetype &filetype);

}

VIDEOMUSH_EXPORTS std::shared_ptr<mush::opencl> videoMushGetContext();


#endif
