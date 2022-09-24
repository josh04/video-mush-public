//
//  tapeProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 16/04/2016.
//
//

#ifndef tapeProcess_h
#define tapeProcess_h

#include <random>

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class tapeProcess : public mush::imageProcess {
    public:
        tapeProcess() : mush::imageProcess(), rd(), gen(rd()) {
            
        }
        
        ~tapeProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
        
        void process();
        
    private:
        void generate_offsets();
        void adjust_alignment();
        
        cl::Kernel * _tape_interlace_on = nullptr;
        cl::Kernel * _tape_interlace_off = nullptr;
        cl::Kernel * _tape_convert = nullptr;
        cl::Kernel * _tape_read = nullptr;
        cl::Buffer * _line_offsets = nullptr;
        float * _line_offsets_host = nullptr;
        std::vector<int> _line_offset_linger;
        std::vector<int> _line_offset_linger_next;
        std::vector<int> _line_offset_rand;
        
        cl::Image2D * _temp_image = nullptr;
        cl::Image2D * _tape_image = nullptr;
        
        mush::registerContainer<mush::imageBuffer> _buffer;
        
        float tape_align_master = 0.0f;
        float tape_shift_master = 0.01f;
        
        float tape_align_red = 0.00f;
        float tape_align_green = 0.00f;
        float tape_align_blue = 0.00f;
        
        float tape_shift_red = 0.00f;
        float tape_shift_green = 0.00f;
        float tape_shift_blue = 0.00f;
        
        std::random_device rd;
        std::mt19937 gen;
        
        float offset_factor = 10.0f;
        
        float tape_shift_offset = 0.0f;
        
        int align_tick = 0;
    };
    
}

#endif /* tapeProcess_h */
