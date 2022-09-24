//
//  trayraceRedraw.hpp
//  video-mush
//
//  Created by Josh McNamee on 19/08/2014.
//
//

#ifndef video_mush_trayraceRedraw_hpp
#define video_mush_trayraceRedraw_hpp

#include <Mush Core/registerContainer.hpp>
#include <Mush Core/integerMapProcess.hpp>

#include <sstream>

class trayraceRedraw : public mush::integerMapProcess {
public:
    trayraceRedraw(int scale = 4) : mush::integerMapProcess(1), mapBuffer(), _scale(scale) {
        
    }
    
    ~trayraceRedraw() {
        
    }
    
    void release() {
        mapBuffer.kill();
        integerMapProcess::release();
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        redraw = context->getKernel("redraw");
        
        discontinuities = castToIntegerMap(buffers.begin()[0]);
        
        discontinuities->getParams(_width, _height, _size);
        
        getMap = context->hostReadBuffer(_width*_height*sizeof(cl_uchar));
        memset(getMap, 1, _width*_height*sizeof(cl_uchar));
        mapBuffer.addItem(getMap);
        
        addItem(context->buffer(_width*_height*sizeof(cl_uchar)));
        
        redraw->setArg(0, _getMem(0).get_buffer());
        redraw->setArg(2, _width);
        redraw->setArg(4, _scale);
        
        queue = context->getQueue();
        
        cl::Event event;
#ifdef CL_VERSION_1_2
        queue->enqueueFillBuffer<unsigned char>(_getMem(0).get_buffer(), 0, 0, _width*_height*sizeof(cl_uchar), NULL, &event);
        event.wait();
#endif
        
        //mapBuffer.null(); // DISABLE IT
        
        //mapBuffer.addRepeat();
        mapBuffer.inLock(); // prep with payload for first draw
        mapBuffer.inUnlock();
        //mapBuffer.removeRepeat();
        addRepeat();
        inLock();
        inUnlock();
        removeRepeat();
    }
    
    void process() {
        cl::Event event;
        
        auto discont = discontinuities->outLock();
        if (discont == nullptr) {
            release();
            mapBuffer.kill();
            return;
        }

		inLock();

        count = (count+1) % 16;
        
        /*
        unsigned char grid[] =  { 6,  5,  8,  12,
            15,  1,  9, 13,
            0, 10,  7,  4,
            14,  3, 11,  2 };
        
        float xdiff2 =  (float)((grid[count] % 4) % _scale);
        float ydiff2 =  (float)(((grid[count] - (grid[count] % 4))/4 ) % _scale);
        std::stringstream strm;
        strm << "Redraw - xdiff: " << xdiff2 <<" ydiff: " << ydiff2;
        putLog(strm.str());
        */
        redraw->setArg(1, discont.get_buffer());
        redraw->setArg(3, count);
        queue->enqueueNDRangeKernel(*redraw, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        
        mapBuffer.inLock();
        queue->enqueueReadBuffer(_getMem(0).get_buffer(), CL_TRUE, 0, _width*_height*sizeof(cl_uchar), getMap, NULL, &event);
        event.wait();
        mapBuffer.inUnlock();
        
        discontinuities->outUnlock();
        
        inUnlock();
    }
    
    mush::ringBuffer& getRedrawMap() {
        return std::ref(mapBuffer);
    }
    
    // non-thread safe!

private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * redraw = nullptr;

    uint8_t * getMap = nullptr;
    
    mush::registerContainer<mush::integerMapBuffer> discontinuities;
    
    int count = 0;
    
    std::mutex mapMutex;
    mush::ringBuffer mapBuffer;
    
    int _scale = 4;
};
#endif
