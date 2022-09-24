//
//  metricReporter.hpp
//  video-mush
//
//  Created by Josh McNamee on 16/07/2015.
//
//

#ifndef video_mush_metricReporter_hpp
#define video_mush_metricReporter_hpp

#include "metricConfig.hpp"
namespace mush {
class metricReporter {
public:
    metricReporter() {
        
    }
    
    ~metricReporter() {
        
    }
    
    //virtual void report() = 0;
    
    //virtual void finalReport() = 0;

	virtual std::string get_title_format_string() const = 0;
	virtual std::string get_format_string() const = 0;
	/*virtual std::string get_final_format_string() const {
		return get_format_string();
	}*/
	virtual mush::metric_value get_last() const = 0;
	//virtual float get_final() const = 0;
    
private:
};
}

#endif
