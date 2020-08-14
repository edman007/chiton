/**************************************************************************
 *
 *     This file is part of Chiton.
 *
 *   Chiton is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Chiton is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Chiton.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   Copyright 2020 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */
#include "util.hpp"
#include <sys/time.h>


std::mutex Util::lock;
#ifdef DEBUG
unsigned int Util::log_level = 5;
#else
unsigned int Util::log_level = 3;
#endif

void Util::log_msg(const LOG_LEVEL lvl, const std::string& msg){
    if (lvl > log_level){
        return;//drop any message above our current logging level
    }
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

void Util::get_time_parts(const struct timeval &time, struct VideoDate &date){
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

void Util::compute_timestamp(const struct timeval &connect_time, struct timeval &out_time, long pts, AVRational &time_base){
    double delta = av_q2d(time_base) * pts;
    double usec = delta - ((long)delta);
    usec *= 1000000;
    out_time.tv_sec = connect_time.tv_sec + delta;
    out_time.tv_usec = connect_time.tv_usec + usec;
    if (out_time.tv_usec >= 1000000){
        out_time.tv_usec -= 1000000;
        out_time.tv_sec++;
    }
    
    
}

void Util::set_log_level(unsigned int level){
    log_level = level;
}
