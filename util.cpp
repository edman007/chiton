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
#include "util.hpp"
#include <sys/time.h>
#include <syslog.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sstream>

thread_local std::string thread_name = "System";
thread_local int thread_cam_id = -1;

std::mutex Util::lock;
#ifdef DEBUG
unsigned int Util::log_level = CH_LOG_DEBUG;/* All Messages */
#else
unsigned int Util::log_level = CH_LOG_INFO;/* WARN and above */
#endif
bool Util::use_syslog = false;

//we store the color stuff so we are not accessing the config so often
//might be premature optimization, but meh
bool Util::use_color = false;

//color codes are mearly suggestions...the terminal decides them
int Util::color_map[5] = {5, 1, 3, 6, 2};

//these are used to track the log messages
std::map<int, std::deque<LogMsg>> Util::log_history;//a log of our messages
unsigned int Util::history_len = 50;
std::mutex Util::history_lock;


void Util::log_msg(const LOG_LEVEL lvl, const std::string& msg){
    if (lvl >= log_level){
        return;//drop any message above our current logging level
    }
    std::string prefix;
    if (thread_name.length() > 0){
        prefix = thread_name + ": ";
    }
    if (use_syslog){
        int priority = LOG_DEBUG;
        switch (lvl){
        case CH_LOG_FATAL:
            priority = LOG_CRIT;
            break;
        case CH_LOG_ERROR:
            priority = LOG_ERR;
            break;
        case CH_LOG_WARN:
            priority = LOG_WARNING;
            break;
        case CH_LOG_INFO:
            priority = LOG_INFO;
            break;
        case CH_LOG_DEBUG:
            priority = LOG_DEBUG;
            break;
        }
        syslog(priority, "%s", (prefix + msg).c_str());

    } else {
        if (lvl == CH_LOG_ERROR || lvl == CH_LOG_FATAL){
            lock.lock();
            std::cerr << get_color_txt(lvl) << prefix << msg << std::endl;
            lock.unlock();
        } else {
            lock.lock();
            std::cout << get_color_txt(lvl) << prefix << msg << std::endl;
            lock.unlock();
        }
    }
    add_history(msg, lvl);
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

bool Util::enable_syslog(void){
    openlog("chiton", LOG_PID, LOG_USER);
    use_syslog = true;
    return use_syslog;
}

bool Util::disable_syslog(void){
    if (use_syslog){
        closelog();
        use_syslog = false;
    }
    return !use_syslog;
}

//reduces the priority of the calling thread
void Util::set_low_priority(void){
#ifdef HAVE_PTHREAD
    sched_param sch;
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &sch);
    //set SCHED_IDLE  and priority of 0
    sch.sched_priority = 0;
    policy = SCHED_IDLE;
    if (pthread_setschedparam(pthread_self(), policy, &sch)){
        LWARN("Could not reduce the scheduling priority");
    }
#endif
#ifdef SYS_ioprio_set
    //this is actually constants from the kernel source...glibc is missing them,
    //we could include them, but I don't know if it's worth making that a dep for this
    //source for this is $LINUX_SRC/include/linux/ioprio.h
    const int IOPRIO_CLASS_SHIFT = 13;
    enum {
        IOPRIO_CLASS_NONE,
        IOPRIO_CLASS_RT,
        IOPRIO_CLASS_BE,
        IOPRIO_CLASS_IDLE,
    };

    enum {
        IOPRIO_WHO_PROCESS = 1,
        IOPRIO_WHO_PGRP,
        IOPRIO_WHO_USER,
    };

    //reduce IO priority
    if (syscall(SYS_ioprio_set, IOPRIO_WHO_PROCESS, 0, IOPRIO_CLASS_IDLE << IOPRIO_CLASS_SHIFT, 0)){
        LWARN("Could not reduce the IO Priority");
    }
#endif
}

AVDictionary* Util::get_dict_options(const std::string &fmt){
    AVDictionary *dict = NULL;
    if (fmt == ""){
        return NULL;
    }
    int error;
    if ((error = av_dict_parse_string(&dict, fmt.c_str(), "=", ":", 0))){
        LERROR("Error Parsing ffmpeg options (" + fmt + "): " + std::string(av_err2str(error)));
        av_dict_free(&dict);
        return NULL;
    }
    return dict;

}

std::string Util::get_color_txt(enum LOG_LEVEL ll){
    if (!use_color){
        return "";
    }
    int color = color_map[ll];
    std::stringstream ss;
    if (color < 8){
        ss << "\u001b[3" << std::to_string(color) << "m";
    } else {
        ss << "\u001b[38;5;" << std::to_string(color) << "m";
    }
    return ss.str();
}

void Util::reset_color(void){
    lock.lock();
    std::cout << "\u001b[0m";
    lock.unlock();
}

void Util::load_colors(Config &cfg){
    color_map[0] = cfg.get_value_int("log-color-fatal");
    color_map[1] = cfg.get_value_int("log-color-error");
    color_map[2] = cfg.get_value_int("log-color-warn");
    color_map[3] = cfg.get_value_int("log-color-info");
    color_map[4] = cfg.get_value_int("log-color-debug");
    for (auto &c : color_map){
        if (c < 1 || c > 255){
            c = 1;
        }
    }
}


bool Util::enable_color(void){
    use_color = true;
    return use_color;
}

bool Util::disable_color(void){
    if (use_color){
        reset_color();
        use_color = false;
    }
    return !use_color;
}

void Util::set_thread_name(int id, Config &cfg){
    std::string name;
    thread_cam_id = id;
    if (id == -1){
        name = "System";
    } else if (cfg.get_value("display-name") == ""){
        name = "Camera " + std::to_string(id);;
    } else {
        name = cfg.get_value("display-name");
    }

    int len = cfg.get_value_int("log-name-length");
    if (len < 0){
        len = 0;
    }
    thread_name = name.substr(0, len);
}

void Util::add_history(const std::string &msg, enum LOG_LEVEL lvl){
    if (history_len == 0){
        return;//no history recording, it's a no-op
    }
    history_lock.lock();
    try {
        auto &queue = log_history.at(thread_cam_id);
        struct timeval time;
        get_videotime(time);
        queue.emplace(queue.end(), msg, lvl, time);
        //delete any excess
        while (queue.size() > history_len){
            queue.pop_front();
        }
    } catch (std::out_of_range &e){
        //add the queue and do it again
        log_history.emplace(std::make_pair(thread_cam_id, std::deque<LogMsg>()));
        history_lock.unlock();
        return add_history(msg, lvl);
    }
    history_lock.unlock();
}

void Util::set_history_len(int len){
    history_len = len;
    if (history_len < 0){
        history_len = 0;
    }
}

bool Util::get_history(int cam, std::vector<LogMsg> &hist){
    history_lock.lock();
	bool ret = false;
    try {
        auto &queue = log_history.at(cam);
        hist.clear();
        hist.reserve(queue.size());
        for (auto &msg : queue){
            hist.emplace_back(msg);
        }
        ret = true;
    }  catch (std::out_of_range &e){
        //nothing, we return false
    }
    history_lock.unlock();
    return ret;
}
