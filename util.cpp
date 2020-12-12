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
#include <syslog.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

std::mutex Util::lock;
#ifdef DEBUG
unsigned int Util::log_level = CH_LOG_DEBUG;/* All Messages */
#else
unsigned int Util::log_level = CH_LOG_INFO;/* WARN and above */
#endif
bool Util::use_syslog = false;

void Util::log_msg(const LOG_LEVEL lvl, const std::string& msg){
    if (lvl >= log_level){
        return;//drop any message above our current logging level
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
        syslog(priority, "%s", msg.c_str());

    } else {
        if (lvl == CH_LOG_ERROR || lvl == CH_LOG_FATAL){
            lock.lock();
            std::cerr << msg << std::endl;
            lock.unlock();
        } else {
            lock.lock();
            std::cout << msg << std::endl;
            lock.unlock();
        }
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
    sched_param sch;
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &sch);
    //set SCHED_IDLE  and priority of 0
    sch.sched_priority = 0;
    policy = SCHED_IDLE;
    if (pthread_setschedparam(pthread_self(), policy, &sch)){
        LWARN("Could not reduce the scheduling priority");
    }

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
