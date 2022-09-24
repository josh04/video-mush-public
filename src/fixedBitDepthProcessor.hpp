//
//  fixedBitDepthProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 19/01/2015.
//
//

#ifndef video_mush_fixedBitDepthProcessor_hpp
#define video_mush_fixedBitDepthProcessor_hpp

#include "profile.hpp"
#include "ConfigStruct.hpp"
#include <Mush Core/imageProcessor.hpp>
#include <thread>

namespace mush {
    class yuvDepthClampProcess;
    class shortFalseColour;
    class pqEncodeProcess;
    class pqDecodeProcess;
    class psnrProcess;
    class integerMapProcess;
	class PTF4DecodeProcess;
	class PTF4Process;
    class ffmpegEncodeDecode;
}
class hdrProcessor;


class fixedBitDepthProcessor : public mush::imageProcessor {
public:
    fixedBitDepthProcessor(float yuvMax, int bitDepth, bool pqLegacy, mush::transfer func, mush::fbdOutputs output = mush::fbdOutputs::switcher) :
    yuvMax(yuvMax), bitDepth(bitDepth), pqLegacy(pqLegacy), func(func), _output(output) {
        
    }
    
    ~fixedBitDepthProcessor() {}
	
	
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
    
    void process();
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers();
    
    void go();
    
private:
    shared_ptr<mush::opencl> context = nullptr;
    shared_ptr<mush::ringBuffer> imageBuf = nullptr;
    
    std::shared_ptr<mush::imageProcess> _pq = nullptr;
    
	std::shared_ptr<mush::pqDecodeProcess> _pqD = nullptr;
	std::shared_ptr<mush::PTF4DecodeProcess> _gamD = nullptr;
	
    std::shared_ptr<mush::yuvDepthClampProcess> _clamp = nullptr;
    
    std::shared_ptr<mush::imageProcess> _fc = nullptr;
    std::shared_ptr<mush::shortFalseColour> _sfc = nullptr;
    
    std::shared_ptr<mush::psnrProcess> _psnr = nullptr;
    
    std::shared_ptr<mush::imageProcess> _tone = nullptr;
    std::shared_ptr<mush::imageProcess> _switcher = nullptr;
    
    std::shared_ptr<mush::imageProcess> _out = nullptr;
    std::shared_ptr<mush::imageProcess> _chromaSwap = nullptr;
    
    
    std::shared_ptr<mush::imageProcess> _pqD_test = nullptr;
    std::shared_ptr<mush::imageProcess> _g8D_test = nullptr;
    
    
    std::shared_ptr<mush::ffmpegEncodeDecode> _ff1 = nullptr;
    std::shared_ptr<mush::ffmpegEncodeDecode> _ff2 = nullptr;
    std::shared_ptr<mush::ffmpegEncodeDecode> _ff3 = nullptr;
    std::shared_ptr<mush::ffmpegEncodeDecode> _ff4 = nullptr;
    std::shared_ptr<mush::ffmpegEncodeDecode> _ff5 = nullptr;
    
    float yuvMax = 10.0f;
    int bitDepth = 10;
    
    bool pqLegacy = false;
    
    cl::Event event;
    cl::CommandQueue * queue;
	
	mush::transfer func = mush::transfer::pq;
    mush::fbdOutputs _output = mush::fbdOutputs::switcher;
    std::vector<std::thread> ffTh;
    bool threadsSpawned = false;
    
    Profile _profile;
};


#endif
