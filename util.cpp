#include "util.hpp"
#include <sys/time.h>


std::mutex Util::lock;

void Util::log_msg(const LOG_LEVEL lvl, const std::string& msg){
    if (lvl == LOG_ERROR || lvl == LOG_FATAL){
        lock.lock();
        std::cerr << msg << std::endl;
        lock.unlock();
    } else {
        lock.lock();
        std::cout << msg << std::endl;
        lock.unlock();
    }
}

void Util::get_videotime(struct timeval &time){
    gettimeofday(&time, NULL);
    
}

void Util::get_time_parts(struct timeval &time, struct VideoDate &date){
    struct tm out;
    date.ms = time.tv_usec / 1000;
    localtime_r(&time.tv_sec, &out);
    
    //copy it into our format
    date.year = out.tm_year + 1900;
    date.month = out.tm_mon + 1;
    date.day = out.tm_mday;
    date.hour = out.tm_hour;
    date.min = out.tm_min;
    date.sec = out.tm_sec;
}

unsigned long long int Util::pack_time(const struct timeval &time){
    unsigned long long int out;
    out = time.tv_sec;
    out *= 1000;
    out += time.tv_usec / 1000;
    return out;
}

void Util::unpack_time(const unsigned long long int packed_time, struct timeval &time){
    time.tv_usec = (packed_time % 1000) * 1000;
    time.tv_sec = packed_time / 1000;
}
