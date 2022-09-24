//
//  metricConfig.hpp
//  mush-core
//
//  Created by Josh McNamee on 12/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#ifndef metricConfig_h
#define metricConfig_h

namespace mush {
    
    
    enum class metrics {
        psnr,
        log_psnr,
        pupsnr,
        ssim,
        pussim
    };
    
    enum class metric_value_type {
        i,
        f,
        null
    };
    
    union metric_value_union {
        int integer;
        float floating_point;
    };
    
    struct metric_value {
        metric_value() {}
        metric_value(metric_value_type type,
                     metric_value_union value,
                     uint32_t frame,
                     bool is_average,
                     bool is_null) :
        type(type),
        value(value),
        frame(frame),
        is_average(is_average),
        is_null(is_null) {
        }
        
        metric_value_type type = metric_value_type::f;
        metric_value_union value;
        uint32_t frame = 0;
        bool is_average = false;
        bool is_null = false;
    };
    
}

#endif /* metricConfig_h */
