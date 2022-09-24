//
//  ffmpegWrapper.cpp
//  video-mush
//
//  Created by Josh McNamee on 24/06/2015.
//
//


#include <mutex>
#include <string>

extern "C" void putLog(std::string s);

extern "C" {
#include <libavformat/avformat.h>
}

#include "ffmpegWrapper.hpp"

namespace mush {
    ffmpegWrapper::ffmpegWrapper()  {
        static std::once_flag initFlag;
        std::call_once(initFlag, []() {
            av_register_all();
            avformat_network_init();
            av_log_set_callback([](void * ptr, int level, const char* szFmt, va_list varg){ if (level < av_log_get_level()) { int pre = 1; char line[1024]; av_log_format_line(ptr, level, szFmt, varg, line, 1024, &pre); putLog(std::string(line)); }});
        });
        
    }
}
