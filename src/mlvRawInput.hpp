//
//  mlvRawInput.h
//  video-mush
//
//  Created by Josh McNamee on 04/08/2015.
//
//

#ifndef __video_mush__mlvRawInput__
#define __video_mush__mlvRawInput__

#include <array>
#include <azure/Eventable.hpp>

#include <Mush Core/scrubbableFrameGrabber.hpp>
#include <OpenEXR/ImathMatrix.h>


namespace mush {
    class mlvRawInput : public mush::scrubbableFrameGrabber, public azure::Eventable {
    public:
        mlvRawInput(int black_point, int white_point, unsigned int frame_skip);
        
        ~mlvRawInput() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        void gather() override;
        
        int getBlackPoint() { return _black; }
		int getWhitePoint() { return _white; }

		void moveToFrame(int frame) override;

        int getFrameCount() override;
        int getCurrentFrame() override;

		bool event(std::shared_ptr<azure::Event> event) override;

        rawCameraType get_camera_type() const {
            return _camera_type;
        }
        
    private:

		cl::Kernel * _mlv_raw_map = nullptr;
		cl::Buffer * _input_buffer = nullptr;
		uint16_t * _upload_buffer = nullptr;
		std::mutex _scrub_mutex;
		bool _scrub_set = false;
		int _scrub_target = 0;

		int _frame_count = -1;
        std::atomic_int _currentFrame{0};
        
        unsigned int _frame_skip;
        bool checkId(const char * ptr, const char * id);
        std::vector<std::shared_ptr<std::ifstream>> _inputs;
        
        unsigned int _depth = 14;
        
        int _black = 0;
		int _white = 0;
        
        int _file_count = 0;
        
        std::string _input_path;
        
        size_t read_header(std::shared_ptr<std::ifstream> input);
        
        std::vector<std::vector<std::pair<uint64_t, std::streampos>>> _files_positions;
        
        
        std::vector<std::pair<std::pair<uint64_t, uint32_t>, std::streampos>> _reg_files_positions;
        
        rawCameraType _camera_type;
        
        bool _dual_iso = false;
    };
}

#endif /* defined(__video_mush__mlvRawInput__) */
