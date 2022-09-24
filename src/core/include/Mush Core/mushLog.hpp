//
//  mushLog.hpp
//  mush-core
//
//  Created by Josh McNamee on 04/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#ifndef mushLog_hpp
#define mushLog_hpp

#include <string>
#include <vector>

#include "metricConfig.hpp"

#include "mush-core-dll.hpp"

/* getLog
 * passed a 4096 char array, getLog will fill
 * that buffer with incoming log messages.
 * rinse, repeat.
 * sets bool set to true if log is entered,
 * returns after 20 milliseconds otherwise
 */

extern "C" {
    MUSHEXPORTS_API bool getLog(char * output, uint64_t size);
    MUSHEXPORTS_API void putLog(std::string log);

    MUSHEXPORTS_API void endLog(); // needed on windows to have global cond_var
    
    /* getLog
     * passed a 4096 char array, getLog will fill
     * that buffer with incoming log messages.
     * rinse, repeat.
     * sets bool set to true if log is entered,
     * returns after 20 milliseconds otherwise
     */
    
    MUSHEXPORTS_API uint32_t getRowCount();
    MUSHEXPORTS_API bool getRow(mush::metric_value * output, uint32_t size);
    MUSHEXPORTS_API void addRow(const std::vector<mush::metric_value>&);
    MUSHEXPORTS_API void endRow();
    
    
    MUSHEXPORTS_API bool newRowNames(uint32_t * count);
    MUSHEXPORTS_API void getRowNames(char ** names, uint32_t count, size_t max_string_length);
    MUSHEXPORTS_API void getRowName(char * name, uint32_t index, size_t max_string_length);
    MUSHEXPORTS_API void setRowNames(std::vector<std::string> names);
}

#endif /* mushLog_hpp */
