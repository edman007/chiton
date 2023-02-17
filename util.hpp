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
 *   Copyright 2020-2022 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

#include <iostream>
#include <mutex>
#include <time.h>
#include <deque>
#include "config_build.hpp"
#include "chiton_ffmpeg.hpp"
#include "chiton_config.hpp"

//when editing check the static assert for color_map in util.cpp
enum LOG_LEVEL {
    CH_LOG_FATAL = 0,
    CH_LOG_ERROR,//1
    CH_LOG_WARN,//2
    CH_LOG_INFO,//3
    CH_LOG_DEBUG,//4
};


//basic macros
#ifdef DEBUG
#define LDEBUG(str) Util::log_msg(CH_LOG_DEBUG, str)
#else
#define LDEBUG(str)
#endif

#define LINFO(str) Util::log_msg(CH_LOG_INFO, str)
#define LWARN(str) Util::log_msg(CH_LOG_WARN, str)
#define LERROR(str) Util::log_msg(CH_LOG_ERROR, str)
#define LFATAL(str) Util::log_msg(CH_LOG_FATAL, str)



struct VideoDate {
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int min;
    unsigned int sec;
    unsigned int ms;
};

//used for querying log info
struct LogMsg {
    std::string msg;
    LOG_LEVEL lvl;
    struct timeval time;
    LogMsg(const std::string &msg, LOG_LEVEL lvl, struct timeval time) : msg(msg), lvl(lvl), time(time) {};
};

class Util {

public:
    /*
     * Log a message
     */
    static void log_msg(const LOG_LEVEL lvl, const std::string& msg);

    static void get_videotime(struct timeval &time);//write the current time out

    static void get_time_parts(const struct timeval &time, struct VideoDate &date);//write the time out to date format

    //pack and unpack time for database storage, spits out a 64-bit unsigned int
    static unsigned long long int pack_time(const struct timeval &time);
    static void unpack_time(const unsigned long long int packed_time, struct timeval &time);

    static void compute_timestamp(const struct timeval &connect_time, struct timeval &out_time, long pts, AVRational &time_base);

    static void set_log_level(unsigned int level);//set the amount of logging that we do..
    static void set_history_len(int len);
    static bool get_history(int cam, std::vector<LogMsg> &hist);//write thie history for a given camera to hist, return true if messages found

    static bool enable_syslog(void);
    static bool disable_syslog(void);

    static bool enable_color(void);
    static bool disable_color(void);

    static void set_low_priority(void);//reduce the current thread to low priority

    static AVDictionary* get_dict_options(const std::string &fmt);//convert fmt into an AVDict, caller must free

    static void reset_color(void);//clears the color on the CLI
    static void load_colors(Config &cfg);//load all color settings

    static void set_thread_name(int id, Config &cfg);//takes the camera ID and the camera's config
private:
    static std::mutex lock;//lock for actually printing messages
    static unsigned int log_level;//the output logging level
    static bool use_syslog;
    static bool use_color;

    static int color_map[5];
    static_assert(sizeof(color_map)/sizeof(color_map[0]) == (1 + CH_LOG_DEBUG), "Color map must be the same size as enum LOG_LEVEL");

    static std::map<int, std::deque<LogMsg>> log_history;//a log of our messages
    static unsigned int history_len;
    static std::mutex history_lock;

    static std::string get_color_txt(enum LOG_LEVEL ll);//return the string to switch the color on the CLI
    static void add_history(const std::string &msg, LOG_LEVEL lvl);//add the message to the history log
};

#endif
