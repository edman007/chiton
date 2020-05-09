#ifndef __UTIL_HPP__
#define __UTIL_HPP__


#include <iostream>


enum LOG_LEVEL {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
};

class Util {

public:
    /*
     * Log a message
     */
    static void log_msg(const LOG_LEVEL lvl, const std::string& msg);

    
    
};

#endif
