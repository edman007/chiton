#ifndef __UTIL_HPP__
#define __UTIL_HPP__
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

#include <iostream>
#include <mutex>
#include <time.h>


enum LOG_LEVEL {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
};


//basic macros
#define LDEBUG(str) Util::log_msg(LOG_DEBUG, str)
#define LINFO(str) Util::log_msg(LOG_INFO, str)
#define LWARN(str) Util::log_msg(LOG_WARN, str)
#define LERROR(str) Util::log_msg(LOG_ERROR, str)
#define LFATAL(str) Util::log_msg(LOG_FATAL, str)



struct VideoDate {
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int min;
    unsigned int sec;
    unsigned int ms;
};

class Util {

public:
    /*
     * Log a message
     */
    static void log_msg(const LOG_LEVEL lvl, const std::string& msg);

    static void get_videotime(struct timeval &time);//write the current time out

    static void get_time_parts(struct timeval &time, struct VideoDate &date);//write the time out to date format

    //pack and unpack time for database storage, spits out a 64-bit unsigned int
    static unsigned long long int pack_time(const struct timeval &time);
    static void unpack_time(const unsigned long long int packed_time, struct timeval &time);
    
private:
    static std::mutex lock;//lock for actually printing messages
};

#endif
